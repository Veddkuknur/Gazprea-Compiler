#include "DefineSymbols.h"
#include "iostream"
#include "CompileTimeExceptions.h"

#include <unordered_set>
#include "ScopedSymbol.h"


void DefineSymbols::showSymbolTable() {
    std::cout << symtab->toString() << std::endl;
}


std::shared_ptr<Type> DefineSymbols::resolveType(std::shared_ptr<Type> type) {
    //Enter a type name (only two types) and obtain the type symbol.

    std::string type_id;
    //Find out which type is it
    if (auto primitive_type = std::dynamic_pointer_cast<PrimitiveType>(type)) {
        //This should be replace by a simple type_id = primitive_type->type_id
        switch (primitive_type->base_type) {
            case Type::BaseType::integer:
                type_id = "integer";
            break;
            case Type::BaseType::boolean:
                type_id =  "boolean";
            break;
            case Type::BaseType::character:
                type_id =  "character";
            break;
            case Type::BaseType::real:
                type_id =  "real";
            break;
        }
    }
    /*
        Otherwise if matrix, figure out if matrix or vector has a base_type that is a type def
    Right now doing this makes no sense, but it will make sense once we use type def.
    An else if should go here with vector and matrix types.*/
    const auto result = symtab->currentScope->resolve(type_id);
    type = std::dynamic_pointer_cast<Type>(result->type);
    return type;
}


std::any DefineSymbols::visitRootNode(std::shared_ptr<RootNode> node) {

    auto result = node->global_block->accept(*this);
    node->containing_scope = symtab->globals; //Set scope of node to the current scope
    return {};
}


std::any DefineSymbols::visitDeclNode(std::shared_ptr<DeclNode> node) {

    node->containing_scope = symtab->currentScope;// Set the containing scope as the current scope

    //Resolve the name of the symbol
    auto sym_name = node->id;
    auto sym_mutability = !node->constant;

    //For now we ignore the part where we resolve for a given type, but ideally we should have the exact information about the type being filled.
    std::shared_ptr<Type> sym_type = nullptr;
    if (node->type) {
        sym_type = node->type; //resolveType(node->type);
    }
    //If the variable has already been declared.
    if (symtab->currentScope->resolve(node->id, true)) {
        throw SymbolError(node->line, "symbol '" +node->id +"' is already defined");

    }
    auto var_symbol = std::make_shared<VariableSymbol>(sym_name, sym_type, sym_mutability);

    //Check if we are defining a tuple
    if (auto tuple_type = std::dynamic_pointer_cast<TupleType>(node->type)){
        //Check if we have any named parameters. If so create TupleScope.
        //Set the scopes for the tuple...o
        std::unordered_set<std::string> param_names;
        for (int i = 0; i < static_cast<int>(tuple_type->element_types.size()); i++){
            auto param_name = tuple_type->element_names[i];
            if (not param_name.empty() and param_names.find(param_name) == param_names.end()) {
                param_names.emplace(param_name);
            } else if (not param_name.empty() and param_names.find(param_name) != param_names.end()){
                throw SymbolError(node->line, "Cannot have duplicate element names in a tuple");
            }
        }
        //TODO: Check for names
    }
    //TODO: Implement global error for empty expressions.
    //Declare the variable symbol.

    if (auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type)) {
        if (vec_type->size){
            vec_type->size->accept(*this);

        }
    } else if(auto mat_type = std::dynamic_pointer_cast<MatrixType>(node->type)) {

        if (mat_type->rows) {
            mat_type->rows->accept(*this);
        }
        if (mat_type->cols) {
            mat_type->cols->accept(*this);
        }


    }


    if (node->expr){
        //Visit any expression inside to set the proper scopes.
        node->expr->accept(*this);
    }

    //Define the current scope.
    symtab->currentScope->define(var_symbol);
    return {};
}


std::any DefineSymbols::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {

    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    if (not node->containing_scope->resolve(node->id) ){
        throw SymbolError(node->line, "symbol '" + node->id +"' is not defined.");
    }
    node->expr->accept(*this);
    std::string return_val = "return value from IdAssignNode!";
    return {return_val};
}


std::any DefineSymbols::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {

    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    node->expr_assign_to->accept(*this);
    node->expr_assign_with->accept(*this);
    std::string return_val = "return value from VecMatTupAssignNode!";
    return {};
}


std::any DefineSymbols::visitTypeDefNode(std::shared_ptr<TypeDefNode> node) {
    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    return {};
}


std::any DefineSymbols::visitBlockNode(std::shared_ptr<BlockNode> node) {

    if (symtab == nullptr) {
        symtab = std::make_shared<SymbolTable>();
        node->containing_scope = symtab->currentScope;
    } else{
        symtab->push_scope(std::make_shared<LocalScope>());
        node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    }
    // if we are in an iterator loop, define the domain variable symbol
    if (domain_variable) {
        symtab->currentScope->define(domain_variable);
        domain_variable = nullptr;
    }

    //Visit every statement in the block
    for (auto stat : node->stat_nodes) {
        auto result = stat->accept(*this);
    }
    symtab->pop_scope();
    return {};
}


std::any DefineSymbols::visitBoolNode(std::shared_ptr<BoolNode> node) {
    node->containing_scope = symtab->currentScope;
    return {};
}


std::any DefineSymbols::visitCharNode(std::shared_ptr<CharNode> node) {
    node->containing_scope = symtab->currentScope;
    return {};
}


std::any DefineSymbols::visitIntNode(std::shared_ptr<IntNode> node) {
    node->containing_scope = symtab->currentScope;
    return {};
}


std::any DefineSymbols::visitRealNode(std::shared_ptr<RealNode> node) {
    node->containing_scope = symtab->currentScope;
    return {};
}


std::any DefineSymbols::visitTupleNode(std::shared_ptr<TupleNode> node) {

    node->containing_scope = symtab->currentScope;

    for (auto element : node->tuple) {
        element->accept(*this);
    }
    //When we declare a new tuple, visit the tuple node
    return {};
}


std::any DefineSymbols::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {

    node->containing_scope = symtab->currentScope;
    node->expr->accept(*this);
    return {};
}


std::any DefineSymbols::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {

    node->containing_scope = symtab->currentScope;

    //Visit left side
    node->l_expr->accept(*this);

    //Visit right side
    node->r_expr->accept(*this);
    return {};
}


std::any DefineSymbols::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {

    node->containing_scope = symtab->currentScope;
    auto result = node->expr->accept(*this);
    if (auto vec_type = std::dynamic_pointer_cast<VectorType>(node->cast_type)) {
        vec_type->size->accept(*this);
    } else if (auto mat_type = std::dynamic_pointer_cast<MatrixType>(node->cast_type)) {
        mat_type->rows->accept(*this);
        mat_type->cols->accept(*this);
    }
    return {};
}


std::any DefineSymbols::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {

    auto scope = std::make_shared<LoopScope>();
    symtab->push_scope(scope);
    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope

     ; //Set scope of node to the current scope
    //TODO: Implement scoping
    auto result = node->loop_block->accept(*this);
    symtab->pop_scope();
    return {};
}


std::any DefineSymbols::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

    //TODO: Implement scoping
    //Push the loop scope, this scope will serve to get continue and header blocks for continue and break
    symtab->push_scope(std::make_shared<LoopScope>());
    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope

    node->contr_expr->accept(*this);
    // visit the loop block
    node->loop_block->accept(*this);
    symtab->pop_scope();
    return {};
}



std::any DefineSymbols::visitGeneratorNode(std::shared_ptr<GeneratorNode> node) {
    symtab->push_scope(std::make_shared<LocalScope>());




    if (node->vec_expr1) {
        node->vec_expr1->accept(*this);
    }
    if (node->vec_expr2) {
        node->vec_expr2->accept(*this);
    }
    //visit(node->domain_node); //Visit the domain node, check if there are any variable or id definitions

    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope

    if (auto id1_node = std::dynamic_pointer_cast<IdNode>(node->id1)) {
        auto sym_type = std::make_shared<PrimitiveType>(Type::BaseType::null);
        auto sym_mutability = true;
        auto id1_sym = std::make_shared<VariableSymbol>(id1_node->id, sym_type, sym_mutability);
        symtab->currentScope->define(id1_sym);
        id1_node->containing_scope = symtab->currentScope;
        id1_node->type = sym_type;
    }

    if (auto id2_node = std::dynamic_pointer_cast<IdNode>(node->id2)) {
        auto sym_type = std::make_shared<PrimitiveType>(Type::BaseType::null);
        auto sym_mutability = true;
        auto id2_sym = std::make_shared<VariableSymbol>(id2_node->id, sym_type, sym_mutability);
        symtab->currentScope->define(id2_sym);
        id2_node->containing_scope = symtab->currentScope;
        id2_node->type = sym_type;

    }

    //Visit expression on the right hand side
    node->rhs_expr->accept(*this);

    symtab->pop_scope();

    return {};
}


std::any DefineSymbols::visitFilterNode(std::shared_ptr<FilterNode> node) {

    symtab->push_scope(std::make_shared<LocalScope>());


    node->vec_expr->accept(*this);

    //visit(node->domain_node); //Visit the domain node, check if there are any variable or id definitions

    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope

    if (auto id_node = std::dynamic_pointer_cast<IdNode>(node->id)) {
        auto sym_type = std::make_shared<PrimitiveType>(Type::BaseType::null);
        auto sym_mutability = true;
        auto id1_sym = std::make_shared<VariableSymbol>(id_node->id, sym_type, sym_mutability);
        symtab->currentScope->define(id1_sym);
        id_node->containing_scope = symtab->currentScope;
        id_node->type = sym_type;
        for (auto element : node->pred_exprs) {
            element->accept(*this); //Apply a proper scope to all the filters.
        }
    } else {
        throw SyntaxError(node->line, "Filter does not have an id");
    }


    symtab->pop_scope();

    return {};

}




std::any DefineSymbols::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

    //TODO: Implement scoping

    symtab->push_scope(std::make_shared<LoopScope>());
    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    node->dom_expr->accept(*this);

    // add the domain variable to the scope table
    auto sym_name = node->dom_id;
    auto sym_mutability = true;
    auto sym_type = node->dom_expr->type;
    domain_variable = std::make_shared<VariableSymbol>(sym_name, sym_type, sym_mutability);

    // visit the loop block
    node->loop_block->accept(*this);

    symtab->pop_scope();
    return {};
}


std::any DefineSymbols::visitCondNode(std::shared_ptr<CondNode> node) {

    //TODO: Implement scoping
    auto enclosing_scope = symtab->currentScope->getEnclosingScope(); //Set scope of node to the current scope
    //Set new scope as current scope

    // visit control expression, any variable here will be resolved to the outside scope of the conditional
    node->contr_expr->accept(*this);

    // visit "if" block, that will take care of the scoping
    node->if_block->accept(*this);

    // visit "else" block (if it exists)
    if (node->else_block != nullptr) {
        node->else_block->accept(*this);
    }
    return {};
}


std::any DefineSymbols::visitOutStreamNode(std::shared_ptr<OutStreamNode> node) {

    node->containing_scope = symtab->currentScope; //This line might be wrong, we will see...
    auto result = node->expr->accept(*this);
    return {};
}


// TODO: this is a temp fix from me (Garrett) to account for grammar and AST change to InStream nodes
//       Juan may need to look at this??
std::any DefineSymbols::visitInStreamNode(std::shared_ptr<InStreamNode> node) {

    node->containing_scope = symtab->currentScope; //This line might be wrong, we will see...

    // new logic: just visit the expression we are writing to
    node->l_value->accept(*this);

    // logic before fix:
    // if (not node->containing_scope->resolve(node->id) ){
    //     throw SymbolError(node->line, "symbol '" + node->id +"' is not defined.");
    // }
    // //Check whether if it has this node.
    // if (node->tupleAccessDot) {
    //     node->tupleAccessDot->accept(*this);
    // }
    return {};
}


std::any DefineSymbols::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {
    node->containing_scope = symtab->currentScope; //This line might be wrong, we will see...

    // visit the id node
    node->tuple_id->accept(*this);
    //node->element->accept(*this) // Check for symbol error her

    return {};
}


std::any DefineSymbols::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {

    //Try to check whether if this function has been declared before
    node->containing_scope = symtab->currentScope; //This line might be wrong, we will see...

    if (!node->func_block){
        // ?
    }
    auto function_sym = std::static_pointer_cast<FunctionSymbol>(symtab->currentScope->resolve(node->func_id));
    //If function is already defined, and we have the prototypes of the elements.
    //How do we change the variable names of each prototype.
    if (!function_sym) {
        function_sym = std::make_shared<FunctionSymbol>(node->func_id, node->return_type, symtab->currentScope);
        //Not efficient but hopefully it works.
        symtab->currentScope->define(function_sym); //DO NOT SWITCH THIS LINE  TO AFTER PUSH_SCOPE OR YOU WILL ETERNAL RECURSSION.

        symtab->push_scope(function_sym);
        //Define symbol in the current scope.
        if (!node->func_block){
            function_sym->prototype = true;
        }
        for (auto param: node->params) {
            //Get params
            auto sym_type = std::get<0>(param); // resolveType(std::get<0>(param));
            auto sym_name = std::get<1>(param);
            auto sym_mutability = false;
            //Define each parameter in the function...
            auto var_symbol = std::make_shared<VariableSymbol>(sym_name, sym_type, sym_mutability);
            function_sym->define(var_symbol);
        }
    } else if (function_sym and function_sym->prototype) {
        symtab->currentScope = function_sym;
        function_sym->prototype = false;
        //Update parameter names...
        function_sym->orderedArgs.clear(); //Clear the symbol names, however, the implementation of this might need to change depending
        // on whether what we should do for checking functions whose types don't match the prototypes.

        for (auto param : node->params) {
            //Get params
            auto sym_type =std::get<0>(param); // resolveType(std::get<0>(param));
            auto sym_name = std::get<1>(param);
            auto sym_mutability = false;

            //Define each parameter in the function...
            auto var_symbol = std::make_shared<VariableSymbol>(sym_name, sym_type, sym_mutability);
            function_sym->define(var_symbol);
            //If no prototype define the list of parameters.
        }

    } else if (not function_sym->prototype) { // If function is not a prototype, and a symbol already exists ( it has been defined) then go to false.
        throw SymbolError(node->line, "function symbol'" + node->func_id +"' has already been defined in a current or parent scope.");
    }
    //Visit every statement inside the function
    if (node->func_block) {
        auto result = node->func_block->accept(*this);
    }
    //Pop the scope
    symtab->pop_scope();

    return {};
}


std::any DefineSymbols::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

    node->containing_scope = symtab->currentScope; //This line might be wrong, we will see...
    auto proc_sym = std::static_pointer_cast<ProcedureSymbol>(symtab->currentScope->resolve(node->proc_id));

    //How do we change the variable names of each prototype.
    if (!proc_sym) {
        proc_sym = std::make_shared<ProcedureSymbol>(node->proc_id, node->return_type, symtab->currentScope);
        //Not efficient but hopefully it works.
        symtab->currentScope->define(proc_sym); //DO NOT SWITCH THIS LINE  TO AFTER PUSH_SCOPE OR YOU WILL ETERNAL RECURSSION.
        if (!node->proc_block){
            proc_sym->prototype = true;
        }
        symtab->push_scope(proc_sym);
        //Define symbol in the current scope.

        for (auto param: node->params) {
            //Get params
            auto sym_mutability = not std::get<0>(param);
            auto sym_type = std::get<1>(param); //resolveType(std::get<1>(param));
            auto sym_name = std::get<2>(param);

            //Define each parameter in the function...
            auto var_symbol = std::make_shared<VariableSymbol>(sym_name, sym_type, sym_mutability);
            proc_sym->define(var_symbol);
        }
    } else if (proc_sym and proc_sym->prototype) {
        symtab->currentScope = proc_sym;
        proc_sym->prototype = false;
        //Update parameter names...
        proc_sym->orderedArgs.clear(); //Clear the symbol names, however, the implementation of this might need to change depending
        // on whether what we should do for checking functions whose types don't match the prototypes.

        bool no_prototype = (proc_sym->param_types.size() == 0);
        for (auto param : node->params) {
            //Get params
            auto sym_mutability = not std::get<0>(param);
            auto sym_type = std::get<1>(param); //resolveType(std::get<1>(param));
            auto sym_name = std::get<2>(param);

            //Define each parameter in the function...
            auto var_symbol = std::make_shared<VariableSymbol>(sym_name, sym_type, sym_mutability);
            proc_sym->define(var_symbol);
            //If no prototype define the list of parameters.
            if (no_prototype){
                proc_sym->param_types.push_back(sym_type); //Only adding this in case it makes someones life easier later on.
            }
        }
    } else if (!proc_sym->prototype) {
        throw SymbolError(node->line, "procedure symbol'" + node->proc_id +"' has already been defined in a current or parent scope.");
    }
    if (node->proc_block) {
        auto result = node->proc_block->accept(*this);
    }
    symtab->pop_scope();
    return {};
}


std::any DefineSymbols::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {

    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    if (not node->containing_scope->resolve(node->id)) {
        throw SymbolError(node->line, "function symbol '" + node->id+"'"+ " is not defined in current or parent scope.");
    }
    for (auto param : node->params) {
        //Visit the parameter
        param->accept(*this);
            // Use param here
    }
    return {};
}


std::any DefineSymbols::visitReturnNode(std::shared_ptr<ReturnNode> node) {

    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    node->free_node->accept(*this);
    if (node->expr) {
        auto result = node->expr->accept(*this);
    }
    return {};
}


std::any DefineSymbols::visitIdNode(std::shared_ptr<IdNode> node) {

    //When visiting an id node we need to determine whether if the ID has been defined beforehand
    auto symbol_id = symtab->currentScope->resolve(node->id);

    if (symbol_id) {
        node->containing_scope =symbol_id->scope;
    } else {
        //Call symbol exception as symbol has not been declared
        throw SymbolError(node->line, "symbol '" + node->id+"'"+ " is not defined in current or parent scope.");
        //node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    }
    if (std::dynamic_pointer_cast<ProcedureSymbol>(symbol_id)) {
        throw SymbolError(node->line, "symbol '" + node->id+"'"+ " is not defined in current or parent scope.");

    } else if (std::dynamic_pointer_cast<FunctionSymbol>(symbol_id)) {
        throw SymbolError(node->line, "symbol '" + node->id+"'"+ " is not defined in current or parent scope.");

    }
    return {};
}


std::any DefineSymbols::visitBreakNode(std::shared_ptr<BreakNode> node) {
    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    return {};
}


std::any DefineSymbols::visitContinueNode(std::shared_ptr<ContinueNode> node) {

    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope
    return {};
}


std::any DefineSymbols::visitUnpackNode(std::shared_ptr<UnpackNode> node) {
    node->containing_scope = symtab->currentScope; //Set scope of node to the current scope

    // visit the i-values to unpack to
    for (auto ivalue : node->ivalues) {
        auto result = ivalue->accept(*this);
    }
    // visit the tuple
    auto result = node->tuple->accept(*this);
    return {};
}


std::any DefineSymbols::visitVecMatNode(std::shared_ptr<VecMatNode> node) {
    node->containing_scope = symtab->currentScope;
    for (auto val : node->elements) {
        val->accept(*this);
    }
    return {};
}
std::any DefineSymbols::visitIndexNode(std::shared_ptr<IndexNode> node) {
    node->containing_scope = symtab->currentScope;

    if (node->index1) {
        node->index1->accept(*this);
    }
    if (node->index2) {
        node->index2->accept(*this);
    }

    node->collection->accept(*this);
    return {};
}


std::any DefineSymbols::visitRangeNode(std::shared_ptr<RangeNode> node) {
    node->containing_scope = symtab->currentScope;
    node->lower_bound->accept(*this);
    node->upper_bound->accept(*this);
    return {};
}


    std::any DefineSymbols::visitFreeNode(std::shared_ptr<FreeNode> node) {
    node->containing_scope = symtab->currentScope;

    return {};
}



std::any DefineSymbols::visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) {
    node->containing_scope = symtab->currentScope;
    auto result = node->vector->accept(*this);
    return {};
}

std::any DefineSymbols::visitRowsCallNode(std::shared_ptr<RowsCallNode> node){
  node->containing_scope = symtab->currentScope;
  auto result = node->matrix->accept(*this);
  return {};
}

std::any DefineSymbols::visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node){
  node->containing_scope = symtab->currentScope;
  auto result = node->matrix->accept(*this);
  return {};
}

std::any DefineSymbols::visitLengthCallNode(std::shared_ptr<LengthCallNode> node) {
    node->containing_scope = symtab->currentScope;
    auto result = node->vector->accept(*this);
    return {};
}

std::any DefineSymbols::visitFormatCallNode(std::shared_ptr<FormatCallNode> node) {
    node->containing_scope = symtab->currentScope;
    auto result = node->scalar->accept(*this);
    return {};
}