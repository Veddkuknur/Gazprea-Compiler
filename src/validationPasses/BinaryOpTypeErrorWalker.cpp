#include "BinaryOpTypeErrorWalker.h"

#include <iostream>

#include "Symbol.h"


std::any BinaryOpTypeErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    node->global_block->accept(*this);
    return {};
}


std::any BinaryOpTypeErrorWalker::visitIntNode(std::shared_ptr<IntNode> node) {
    PrimitiveType primitive;
    primitive.base_type = Type::BaseType::integer;
    return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}


std::any BinaryOpTypeErrorWalker::visitRealNode(std::shared_ptr<RealNode> node) {
    PrimitiveType primitive;
    primitive.base_type = Type::BaseType::real;
    return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}


std::any BinaryOpTypeErrorWalker::visitCharNode(std::shared_ptr<CharNode> node) {
    PrimitiveType primitive;
    primitive.base_type = Type::BaseType::character;
    return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}


std::any BinaryOpTypeErrorWalker::visitBoolNode(std::shared_ptr<BoolNode> node) {
    PrimitiveType primitive;
    primitive.base_type = Type::BaseType::boolean;
    return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}


std::any BinaryOpTypeErrorWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {
    if (const auto toType = std::dynamic_pointer_cast<PrimitiveType>(node->cast_type)) {
    const auto nodeType = std::any_cast<std::shared_ptr<Type> >(node->expr->accept(*this));
    const auto checkNodeType = std::dynamic_pointer_cast<PrimitiveType>(nodeType);
    const auto checkToType = std::any_cast<std::shared_ptr<Type> >(node->cast_type);
    const auto checkType = std::dynamic_pointer_cast<PrimitiveType>(checkToType);
    if (checkType->base_type == Type::BaseType::boolean && checkNodeType->base_type == Type::BaseType::real)
        throw TypeError(node->line, "Unsupported Cast Type");
    if (checkType->base_type == Type::BaseType::character && checkNodeType->base_type == Type::BaseType::real)
        throw TypeError(node->line, "Unsupported Cast Type");
    }
    return node->cast_type;
}


std::any BinaryOpTypeErrorWalker::checkVecType(std::shared_ptr<Type> left_side, std::shared_ptr<Type> right_side, std::string op, int  line) {
    //Check equal not equal operations

    std::shared_ptr<Type> final_type;
    if (op =="||") {
        Type::BaseType base_type = left_side->getBaseType();
        if (base_type == Type::BaseType::integer && left_side->getBaseType() == Type::BaseType::real ) {
            base_type = Type::BaseType::real;
        }
        if (std::dynamic_pointer_cast<VectorType>(left_side) && std::dynamic_pointer_cast<VectorType>(right_side)) {
            auto size = std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<VectorType>(left_side)->size)->val;
            size +=  std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<VectorType>(right_side)->size)->val;
            auto intVecnode = std::make_shared<IntNode>(size,-1,-1);
            return std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, intVecnode));
        }if (std::dynamic_pointer_cast<PrimitiveType>(left_side)&&std::dynamic_pointer_cast<VectorType>(right_side)) {
            auto size = std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<VectorType>(right_side)->size)->val;
            size +=  1;
            auto intVecnode = std::make_shared<IntNode>(size,-1,-1);
            return std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, intVecnode));
        }
        if (std::dynamic_pointer_cast<VectorType>(left_side)&&std::dynamic_pointer_cast<PrimitiveType>(right_side)) {
            auto size = std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<VectorType>(left_side)->size)->val;
            size +=  1;
            auto intVecnode = std::make_shared<IntNode>(size,-1,-1);
            return std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, intVecnode));
        }
        auto intVecnode = std::make_shared<IntNode>(2,-1,-1);
        return std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, intVecnode));
    }
    if (op == "==" || op == "!=") {
        //TODO: Change
        if (left_side->getBaseType() == Type::BaseType::boolean || right_side->getBaseType()  == Type::BaseType::boolean) {
            throw TypeError(line, "Cannot do comparison between vector and boolean");
        }
        auto base_type = Type::BaseType::boolean;
        final_type = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(base_type));
        return final_type;
    }

    if (op == "or" || op == "not" || op == "xor" || op == "and") {
        //If both sides are boolean and can be cast to a vector then do proper casting otherwise throw a type error
        if (left_side->getBaseType() == Type::BaseType::boolean && right_side->getBaseType()  == Type::BaseType::boolean) {
            auto base_type = Type::BaseType::boolean;
            if (std::dynamic_pointer_cast<VectorType>(left_side) && std::dynamic_pointer_cast<VectorType>(right_side)) {
                auto base_type = Type::BaseType::boolean;
                auto size = std::dynamic_pointer_cast<VectorType>(left_side)->size;
                final_type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, size));
                return final_type;
            }
            //return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
        }
        throw TypeError(line, "Logical operation with non-boolean vectors");

    }
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {

        if (left_side->getBaseType() == Type::BaseType::boolean || right_side->getBaseType()  == Type::BaseType::boolean) {
            throw TypeError(line, "Can only compare vector to integer, real, another vector, or matrix");
        }

        auto base_type = Type::BaseType::boolean;
        std::shared_ptr<ASTNode> size;
        if( std::dynamic_pointer_cast<VectorType>(left_side) && std::dynamic_pointer_cast<PrimitiveType>(right_side)) {
            size = std::dynamic_pointer_cast<VectorType>(left_side)->size;
        } else if (std::dynamic_pointer_cast<PrimitiveType>(left_side) && std::dynamic_pointer_cast<VectorType>(right_side)) {
            size = std::dynamic_pointer_cast<VectorType>(right_side)->size;
        } else if (std::dynamic_pointer_cast<VectorType>(left_side) && std::dynamic_pointer_cast<VectorType>(right_side)) {
            size = std::dynamic_pointer_cast<VectorType>(right_side)->size;
        }
        final_type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, size));

        return final_type;

    }
    //This promotion matrix is for arithmetic operations.
    std::unordered_map<Type::BaseType, std::unordered_map<Type::BaseType, Type::BaseType>> prom_matrix = {
    {Type::BaseType::null, {  //In case any element has been deemed as null during type inference this will help catch type errors from binary op as
                {Type::BaseType::null, Type::BaseType::null},
                {Type::BaseType::character, Type::BaseType::null},
                {Type::BaseType::boolean, Type::BaseType::null},
                {Type::BaseType::real, Type::BaseType::null},
                {Type::BaseType::integer, Type::BaseType::null},
          }},

    {Type::BaseType::character,{
                {Type::BaseType::null, Type::BaseType::null},
                {Type::BaseType::character, Type::BaseType::null},
                {Type::BaseType::boolean, Type::BaseType::null},
                {Type::BaseType::integer, Type::BaseType::null},
                {Type::BaseType::real, Type::BaseType::null}
    }},
    {Type::BaseType::boolean,{
        {Type::BaseType::null, Type::BaseType::null},
        {Type::BaseType::character, Type::BaseType::null},
        {Type::BaseType::boolean, Type::BaseType::null},
        {Type::BaseType::integer, Type::BaseType::null},
        {Type::BaseType::real, Type::BaseType::null}
        }},

    {Type::BaseType::integer,{
        {Type::BaseType::null, Type::BaseType::null},
        {Type::BaseType::character, Type::BaseType::null},

        {Type::BaseType::boolean, Type::BaseType::null},
        {Type::BaseType::integer, Type::BaseType::integer},
        {Type::BaseType::real, Type::BaseType::real}
        }},
    {Type::BaseType::real,{
        {Type::BaseType::null, Type::BaseType::null},
        {Type::BaseType::character, Type::BaseType::null},
        {Type::BaseType::boolean, Type::BaseType::null},
        {Type::BaseType::integer, Type::BaseType::real},
        {Type::BaseType::real, Type::BaseType::real}
    }}
    };

                                                                        // From    boolean integer real
                                                                        //To:
                                                                        // boolean  void  void   void
                                                                        // integer void  integer real

    // real    void  reak real
    //Now the last is to check basic binary operators, now we have to promote to it

    auto base_type = prom_matrix[left_side->getBaseType()][right_side->getBaseType()];
    if (base_type == Type::BaseType::null) {
        throw TypeError(line, "Arithmetic between vectors of incompatible types");
    }



    if( std::dynamic_pointer_cast<VectorType>(left_side) && std::dynamic_pointer_cast<PrimitiveType>(right_side)) {
        auto size = std::dynamic_pointer_cast<VectorType>(left_side)->size;
        final_type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, size));
    } else if (std::dynamic_pointer_cast<PrimitiveType>(left_side) && std::dynamic_pointer_cast<VectorType>(right_side)) {
        auto size = std::dynamic_pointer_cast<VectorType>(right_side)->size;
        final_type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, size));
    } else if (std::dynamic_pointer_cast<VectorType>(left_side) && std::dynamic_pointer_cast<VectorType>(right_side)) {
        //We assume both vectors are thes same size
        auto size = std::dynamic_pointer_cast<VectorType>(right_side)->size;

        final_type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, size));
    }

    return final_type;





}


std::any BinaryOpTypeErrorWalker::visitVecMatNode(std::shared_ptr<VecMatNode> node) {
    return node->type;
}


std::any BinaryOpTypeErrorWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
    PrimitiveType primitive;
    primitive.base_type = Type::BaseType::boolean;
    const auto lType = std::any_cast<std::shared_ptr<Type> >(node->l_expr->accept(*this));
    const auto rType = std::any_cast<std::shared_ptr<Type> >(node->r_expr->accept(*this));


    //Do this to not mess up with any of the existing codebase
    if (std::dynamic_pointer_cast<VectorType>(lType) || std::dynamic_pointer_cast<VectorType>(rType)) {
        //The type mismatch error has been handled by type infer walker, its just easier.
        if (node->op == "**") {
            if (node->type->getBaseType() == Type::BaseType::null) {
                throw TypeError(node->line, "Type mismatch");
            }
            return node->type;
        } else if (node->op == "by") {
            if (!(std::dynamic_pointer_cast<VectorType>(lType) && std::dynamic_pointer_cast<PrimitiveType>(rType))) {
                throw TypeError(node->line, "Stride value must be an integer");
            }

        }
        auto final_type =  checkVecType(lType, rType, node->op, node->line);
        return final_type;
    }
    if(std::dynamic_pointer_cast<MatrixType>(lType) && std::dynamic_pointer_cast<MatrixType>(rType)) {
        //Here we assume that the sizes of both operands are equal
        if (node->op == "**") {

            if (std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<MatrixType>(lType)->cols)->val
                != std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<MatrixType>(rType)->rows)->val) {
                throw SizeError(node->line,"");
                }
            auto retType = std::make_shared<MatrixType>(MatrixType(lType->getBaseType(),std::dynamic_pointer_cast<MatrixType>(lType)->rows,std::dynamic_pointer_cast<MatrixType>(rType)->cols));
            auto result_type = std::dynamic_pointer_cast<Type>(retType);
            node->type = result_type;
            return result_type;
        } else if (node->op == "+") {
          auto retType = std::make_shared<MatrixType>(MatrixType(lType->getBaseType(),std::dynamic_pointer_cast<MatrixType>(lType)->rows,std::dynamic_pointer_cast<MatrixType>(rType)->cols));
          auto result_type = std::dynamic_pointer_cast<Type>(retType);
          node->type = result_type;
          return result_type;
        }



    }
    //Check equal not equal operations
    if (node->op == "==" || node->op == "!=") {
        //Check if the left or right are primitive types, if so we assume that the right is a primitive type
        if (auto left_check = std::dynamic_pointer_cast<PrimitiveType>(lType)) {
            auto right_check = std::dynamic_pointer_cast<PrimitiveType>(rType);

            if (left_check->base_type == Type::BaseType::integer || left_check->base_type == Type::BaseType::real) {
                if (right_check->base_type == Type::BaseType::integer || right_check->base_type == Type::BaseType::real) {
                    return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
                }
            }
            if (left_check->base_type != right_check->base_type) {
                throw TypeError(node->line, "Comparison between incompatible types");
            }
        }if (auto left_check = std::dynamic_pointer_cast<TupleType>(lType)) {
            auto right_check = std::dynamic_pointer_cast<TupleType>(rType);
            if (!(left_check && right_check)) {
                throw TypeError(node->line, "Comparison between incompatible types");
            }
        }




        return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
    }


    const auto left_check = std::dynamic_pointer_cast<PrimitiveType>(lType);
    const auto right_check = std::dynamic_pointer_cast<PrimitiveType>(rType);
    if (node->op == "or" || node->op == "not" || node->op == "xor" || node->op == "and") {
        if (right_check->base_type == Type::BaseType::boolean && right_check->base_type == Type::BaseType::boolean) {
            return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
        }
        throw TypeError(node->line, "Comparison with non-boolean sub-expressions");
    }
    primitive.base_type = Type::BaseType::integer;
    std::shared_ptr<Type> finalType = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
    if (node->op == "<" || node->op == ">" || node->op == "<=" || node->op == ">=") {
        primitive.base_type = Type::BaseType::boolean;
        finalType = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
    }



    if (left_check->getBaseType() == Type::BaseType::integer || left_check->getBaseType() == Type::BaseType::real) {
        if (left_check->base_type == Type::BaseType::real && primitive.base_type != Type::BaseType::boolean) {
            primitive.base_type = Type::BaseType::real;
            finalType = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
        }
    } else {
        if (primitive.base_type == Type::BaseType::boolean) {
            throw TypeError(node->line, "Comparison between incompatible types. Must be an integer or real.");
        }
        throw TypeError(node->line, "Arithmetic between incompatible types. Must be an integer or real.");
    }
    if (right_check->getBaseType() == Type::BaseType::integer || right_check->getBaseType() == Type::BaseType::real) {
        if (right_check->base_type == Type::BaseType::real && primitive.base_type != Type::BaseType::boolean) {
            primitive.base_type = Type::BaseType::real;
            finalType = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
        }
    } else {
        if (primitive.base_type == Type::BaseType::boolean) {
            throw TypeError(node->line, "Comparison between incompatible types. Must be an integer or real.");
        }
        throw TypeError(node->line, "Arithmetic between incompatible types. Must be an integer or real.");
    }
    return finalType;
}


std::any BinaryOpTypeErrorWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {

    //Now we do our last bit of type promotion here, if the type is null then

    //When we compare type mismatch what do we want to check

    // #1 that the base types are the same
    std::unordered_map<Type::BaseType, std::unordered_map<Type::BaseType, Type::BaseType>> prom_matrix = {
        {Type::BaseType::null, {  //In case any element has been deemed as null during type inference this will help catch type errors from binary op as
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::boolean, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::null},
            {Type::BaseType::real, Type::BaseType::null},
            {Type::BaseType::integer, Type::BaseType::null},
        }},
        {Type::BaseType::boolean,{
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::null},
            {Type::BaseType::boolean, Type::BaseType::boolean},
            {Type::BaseType::integer, Type::BaseType::null},
            {Type::BaseType::real, Type::BaseType::null}
        }},
        {Type::BaseType::character,{
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::character},
            {Type::BaseType::boolean, Type::BaseType::null},
            {Type::BaseType::integer, Type::BaseType::null},
            {Type::BaseType::real, Type::BaseType::null}
        }},
        {Type::BaseType::integer,{
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::null},
            {Type::BaseType::boolean, Type::BaseType::null},
            {Type::BaseType::integer, Type::BaseType::integer},
            {Type::BaseType::real, Type::BaseType::real}
        }},
        {Type::BaseType::real,{
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::boolean, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::null},
            {Type::BaseType::integer, Type::BaseType::null},
            {Type::BaseType::real, Type::BaseType::real}
        }}
        };

    //#2 that the primitive/vector/string/matrix types are the same.
    if (node->expr && node->type) {

        node->type = node->containing_scope->resolve(node->id)->type;

        auto type = std::any_cast<std::shared_ptr<Type>>(node->expr->accept(*this));
        if (!std::dynamic_pointer_cast<TupleType>(node->type)) {

            //Promote the type of the expression to the type of the declaration
            auto promoted_type = prom_matrix[type->getBaseType()][node->type->getBaseType()];

            type->setBaseType(promoted_type);

            if (!node->type->isEqualType(type)) {
                throw TypeError(node->line, "Type mismatch");
            }
        }
    } else {
        node->containing_scope->resolve(node->id);
    }
    return {};
}


std::any BinaryOpTypeErrorWalker::visitTupleNode(std::shared_ptr<TupleNode> node) {
    auto tup_type = std::make_shared<TupleType>();
    for (auto tup_el: node->tuple) {
        auto result = std::any_cast<std::shared_ptr<Type>>(tup_el->accept(*this));
        tup_type->element_types.push_back(result);
        tup_type->element_names.emplace_back("");
    }
    return std::dynamic_pointer_cast<Type>(tup_type);
}

std::any BinaryOpTypeErrorWalker::visitFilterNode(std::shared_ptr<FilterNode> node) {

    node->id->accept(*this);
    node->vec_expr->accept(*this);
    for (auto pred_node : node->pred_exprs) {
        pred_node->accept(*this);
    }
    return node->type;

}




std::any BinaryOpTypeErrorWalker::visitGeneratorNode(std::shared_ptr<GeneratorNode> node) {
    if (node->vec_expr1) {
        node->vec_expr1->accept(*this);
    }
    if (node->vec_expr2) {
        node->vec_expr2->accept(*this);
    }

    if (node->id1) {
        node->id1->accept(*this);
    }
    if (node->id2) {
        node->id2->accept(*this);
    }

    node->rhs_expr->accept(*this);
    return node->type;
}


std::any BinaryOpTypeErrorWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {
    const auto symbol = node->containing_scope->resolve(node->tuple_id->id);
    const auto tupleTypesVec = std::dynamic_pointer_cast<TupleType>(symbol->type)->element_types;
    // Get the pos of the element
    int pos = 0;
    if (const auto intNode = std::dynamic_pointer_cast<IntNode>(node->element)){
        pos = intNode->val - 1;
    } else if (const auto idNode = std::dynamic_pointer_cast<IdNode>(node->element)) {
        std::vector<std::string> elemNameVec = std::dynamic_pointer_cast<TupleType>(symbol->type)->element_names;
        const auto elemPos =  find(elemNameVec.begin(),elemNameVec.end(),idNode->id);
        if (elemPos != elemNameVec.end()) {
            // Calculate the index and cast to int
            pos = static_cast<int>(elemPos - elemNameVec.begin());
        }
    }
    return std::dynamic_pointer_cast<Type>(tupleTypesVec[pos]);
}


std::any BinaryOpTypeErrorWalker::visitIdNode(std::shared_ptr<IdNode> node) {
    const auto symbol = std::dynamic_pointer_cast<VariableSymbol>(node->containing_scope->resolve(node->id));
    return symbol->type;
}


std::any BinaryOpTypeErrorWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {
    std::unordered_map<Type::BaseType, std::unordered_map<Type::BaseType, Type::BaseType>> prom_matrix = {
        {Type::BaseType::null, {  //In case any element has been deemed as null during type inference this will help catch type errors from binary op as
                {Type::BaseType::null, Type::BaseType::null},
                {Type::BaseType::boolean, Type::BaseType::null},
                {Type::BaseType::character, Type::BaseType::null},
                {Type::BaseType::real, Type::BaseType::null},
                {Type::BaseType::integer, Type::BaseType::null},
            }},
            {Type::BaseType::boolean,{
                {Type::BaseType::null, Type::BaseType::null},
                {Type::BaseType::character, Type::BaseType::null},
                {Type::BaseType::boolean, Type::BaseType::boolean},
                {Type::BaseType::integer, Type::BaseType::null},
                {Type::BaseType::real, Type::BaseType::null}
            }},
            {Type::BaseType::character,{
                {Type::BaseType::null, Type::BaseType::null},
                {Type::BaseType::character, Type::BaseType::character},
                {Type::BaseType::boolean, Type::BaseType::null},
                {Type::BaseType::integer, Type::BaseType::null},
                {Type::BaseType::real, Type::BaseType::null}
            }},
            {Type::BaseType::integer,{
                {Type::BaseType::null, Type::BaseType::null},
                {Type::BaseType::character, Type::BaseType::null},
                {Type::BaseType::boolean, Type::BaseType::null},
                {Type::BaseType::integer, Type::BaseType::integer},
                {Type::BaseType::real, Type::BaseType::real}
            }},
            {Type::BaseType::real,{
                {Type::BaseType::null, Type::BaseType::null},
                {Type::BaseType::boolean, Type::BaseType::null},
                {Type::BaseType::character, Type::BaseType::null},
                {Type::BaseType::integer, Type::BaseType::real},
                {Type::BaseType::real, Type::BaseType::real}
            }}
    };


    if ( auto index_node = std::dynamic_pointer_cast<IndexNode>(node->expr_assign_to)) {
        if (std::dynamic_pointer_cast<VectorType>(index_node->collection->type)) {
            mlir::Value array;
            std::shared_ptr<Type> type;
            if (auto id_node = std::dynamic_pointer_cast<IdNode>(index_node->collection)) {
                auto array_sym = node->containing_scope->resolve(id_node->id);
                type = array_sym->type;

                auto result_type = std::any_cast<std::shared_ptr<Type>>(node->expr_assign_with->accept(*this));
                if (std::dynamic_pointer_cast<PrimitiveType>(result_type)) {
                    auto test_type = prom_matrix[node->expr_assign_with->type->getBaseType()][ node->expr_assign_to->type->getBaseType()];
                    if (test_type == Type::BaseType::null) {
                        throw TypeError(node->line, "Type mismatch");
                    }

                    return {};
                } else {
                    throw TypeError(node->line, "Cannot assign vector to vector of scalars");
                }



            }
        }
    }
    return {};
}


std::any BinaryOpTypeErrorWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {
    // visit the i-values to unpack to
    for (const auto &ivalue: node->ivalues) {
        ivalue->accept(*this);
    }
    // visit the tuple
    node->tuple->accept(*this);
    return {};
}


std::any BinaryOpTypeErrorWalker::visitTypeDefNode(std::shared_ptr<TypeDefNode> node) {
    return {};
}


std::any BinaryOpTypeErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
    for (const auto &stat: node->stat_nodes) {
        stat->accept(*this);
    }
    return {};
}


std::any BinaryOpTypeErrorWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
    auto result = std::any_cast<std::shared_ptr<Type> >(node->expr->accept(*this));
    if (node->op == "not") {
        if (result->getBaseType() == Type::BaseType::boolean) {
            return result;
        }
        throw TypeError(node->line, "unary NOT can only be performed on boolean.");
    }
    if (result->getBaseType() == Type::BaseType::integer || result->getBaseType() == Type::BaseType::real) {
        return result;
    }
    throw TypeError(node->line, "unary arithmetic can only be performed on integers or reals.");
}


std::any BinaryOpTypeErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
    // visit the loop block
    node->loop_block->accept(*this);
    return {};
}


std::any BinaryOpTypeErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
    // visit the control expression
    node->contr_expr->accept(*this);

    // visit the loop block
    node->loop_block->accept(*this);
    return {};
}


std::any BinaryOpTypeErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
    // visit the domain expression
    node->dom_expr->accept(*this);

    // visit the loop block
    node->loop_block->accept(*this);
    return {};
}


std::any BinaryOpTypeErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
    // visit control expression
    node->contr_expr->accept(*this);

    // visit "if" block
    node->if_block->accept(*this);

    // visit "else" block (if it exists)
    if (node->else_block != nullptr) {
        node->else_block->accept(*this);
    }
    return {};
}


std::any BinaryOpTypeErrorWalker::visitOutStreamNode(std::shared_ptr<OutStreamNode> node) {
    // visit the expression to be printed


    auto result = std::any_cast<std::shared_ptr<Type>>(node->expr->accept(*this));
    if (std::dynamic_pointer_cast<TupleType>(result)) {
        throw TypeError(node->line, "Tuple cannot be printed.");
    }
    return {};
}


std::any BinaryOpTypeErrorWalker::visitInStreamNode(std::shared_ptr<InStreamNode> node) {
    if(const auto idNode = std::dynamic_pointer_cast<IdNode>(node->l_value)){
        return {};
    }
    if (const auto idNode = std::dynamic_pointer_cast<TupleAccessNode>(node->l_value)) {
        return {};
    }
    if (const auto idNode = std::dynamic_pointer_cast<VecMatTupAssignNode>(node->l_value)) {
        return {};
    }
    if (const auto idNode = std::dynamic_pointer_cast<IndexNode>(node->l_value)) {
        return {};
    }
    throw GlobalError(node->line, "invalid l-value" );
    return {};
}


std::any BinaryOpTypeErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
    funcProcBlockRtnType = node->return_type;
    if (node->func_block) {
        node->func_block->accept(*this);
    }
    return {};
}


std::any BinaryOpTypeErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    funcProcBlockRtnType = node->return_type;
    if (node->proc_block) {
        node->proc_block->accept(*this);
    }
    return {};
}


std::any BinaryOpTypeErrorWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {
    return node->containing_scope->resolve(node->id)->type;
}


std::any BinaryOpTypeErrorWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {
    if (node->expr) {
        const auto result = std::any_cast<std::shared_ptr<Type> >(node->expr->accept(*this));
        if (const auto rtnCheck = std::dynamic_pointer_cast<PrimitiveType>(result)) {
            if (rtnCheck->base_type != std::dynamic_pointer_cast<PrimitiveType>(funcProcBlockRtnType)->base_type) {
                if (!(rtnCheck->base_type == Type::BaseType::integer && std::dynamic_pointer_cast<PrimitiveType>(funcProcBlockRtnType)->base_type == Type::BaseType::real)) {
                    throw TypeError(
                        node->line,
                        "Cannot return type " + rtnCheck->getTypeAsString() + " in a subroutine that returns " +
                        funcProcBlockRtnType->getTypeAsString());
                }
            }
        } else if (auto rtnCheck = std::dynamic_pointer_cast<TupleType>(result)) {
            if (!rtnCheck->isEqualType(std::dynamic_pointer_cast<TupleType>(funcProcBlockRtnType))) {
                throw TypeError(
                    node->line,
                    "Cannot return type " + rtnCheck->getTypeAsString() + " in a subroutine that returns " +
                    funcProcBlockRtnType->getTypeAsString());
            }
        }
    }
    return {};
}


std::any BinaryOpTypeErrorWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {



    //Now we do our last bit of type promotion here, if the type is null then

    //When we compare type mismatch what do we want to check

    // #1 that the base types are the same
    std::unordered_map<Type::BaseType, std::unordered_map<Type::BaseType, Type::BaseType>> prom_matrix = {
        {Type::BaseType::null, {  //In case any element has been deemed as null during type inference this will help catch type errors from binary op as
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::boolean, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::null},
            {Type::BaseType::real, Type::BaseType::null},
            {Type::BaseType::integer, Type::BaseType::null},
        }},
        {Type::BaseType::boolean,{
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::null},
            {Type::BaseType::boolean, Type::BaseType::boolean},
            {Type::BaseType::integer, Type::BaseType::null},
            {Type::BaseType::real, Type::BaseType::null}
        }},
        {Type::BaseType::character,{
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::character},
            {Type::BaseType::boolean, Type::BaseType::null},
            {Type::BaseType::integer, Type::BaseType::null},
            {Type::BaseType::real, Type::BaseType::null}
        }},
        {Type::BaseType::integer,{
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::null},
            {Type::BaseType::boolean, Type::BaseType::null},
            {Type::BaseType::integer, Type::BaseType::integer},
            {Type::BaseType::real, Type::BaseType::real}
        }},
        {Type::BaseType::real,{
            {Type::BaseType::null, Type::BaseType::null},
            {Type::BaseType::boolean, Type::BaseType::null},
            {Type::BaseType::character, Type::BaseType::null},
            {Type::BaseType::integer, Type::BaseType::real},
            {Type::BaseType::real, Type::BaseType::real}
        }}
        };

    //#2 that the primitive/vector/string/matrix types are the same.


    auto symbol = node->containing_scope->resolve(node->id);

    if (node->expr) {

        //symbol->type = node->containing_scope->resolve(node->id)->type;

        auto type = std::any_cast<std::shared_ptr<Type>>(node->expr->accept(*this));
        if (!std::dynamic_pointer_cast<TupleType>(symbol->type)) {


            auto promoted_type = prom_matrix[type->getBaseType()][symbol->type->getBaseType()];

            type->setBaseType(promoted_type);

            if (!symbol->type->isEqualType(type)) {
                throw TypeError(node->line, "Type mismatch");
            }
        }
    } else {
        node->containing_scope->resolve(node->id);
    }


    return {};
}

std::any BinaryOpTypeErrorWalker::visitLengthCallNode(std::shared_ptr<LengthCallNode> node) {
    PrimitiveType primitive;
    primitive.base_type = Type::BaseType::integer;
    return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}

std::any BinaryOpTypeErrorWalker::visitRowsCallNode(std::shared_ptr<RowsCallNode> node) {
  PrimitiveType primitive;
  primitive.base_type = Type::BaseType::integer;
  return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}

std::any BinaryOpTypeErrorWalker::visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node) {
  PrimitiveType primitive;
  primitive.base_type = Type::BaseType::integer;
  return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}

std::any BinaryOpTypeErrorWalker::visitIndexNode(std::shared_ptr<IndexNode> node) {

    if (node->index1) {
        node->index1->accept(*this);
        if (!std::dynamic_pointer_cast<PrimitiveType>(node->index1->type) && node->index1->type->getBaseType() != Type::BaseType::integer) {
            throw TypeError(node->line, "Index is not an integer");
        }
    }
    if (node->index2) {
        node->index2->accept(*this);
        if (!std::dynamic_pointer_cast<PrimitiveType>(node->index1->type) && node->index1->type->getBaseType() != Type::BaseType::integer) {
            throw TypeError(node->line, "Index is not an integer");
        }

    }

    node->collection->accept(*this);

    return node->type;
}
std::any BinaryOpTypeErrorWalker::visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) {
    return node->type;
}

std::any BinaryOpTypeErrorWalker::visitRangeNode(std::shared_ptr<RangeNode> node) {
    node->lower_bound->accept(*this);
    node->upper_bound->accept(*this);
    return node->type;
}
std::any BinaryOpTypeErrorWalker::visitFormatCallNode(std::shared_ptr<FormatCallNode> node) {
    const auto result = std::any_cast<std::shared_ptr<Type>>(node->scalar->accept(*this));
    auto resultBaseType = result->getBaseType();
    if (resultBaseType == Type::BaseType::character||resultBaseType ==Type::BaseType::boolean) {
        auto size_node = std::make_shared<IntNode>(1, node->line, node->column);
        auto rtnType = std::make_shared<VectorType>(Type::BaseType::character, size_node);
        rtnType->isString = true;
        return std::dynamic_pointer_cast<Type>(rtnType);
    }if(resultBaseType == Type::BaseType::integer) {
        auto size_node = std::make_shared<IntNode>(11, node->line, node->column);
        auto rtnType = std::make_shared<VectorType>(Type::BaseType::character, size_node);
        rtnType->isString = true;
        return std::dynamic_pointer_cast<Type>(rtnType);
    }if(resultBaseType == Type::BaseType::real) {
        auto size_node = std::make_shared<IntNode>(11, node->line, node->column);
        auto rtnType = std::make_shared<VectorType>(Type::BaseType::character, size_node);
        rtnType->isString = true;
        return std::dynamic_pointer_cast<Type>(rtnType);
    }
    return {};
}