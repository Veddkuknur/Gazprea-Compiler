#include "TypeInferWalker.h"

#include <iostream>

#include "Symbol.h"


std::any TypeInferWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    auto result = node->global_block->accept(*this);
    return {};
}


std::any TypeInferWalker::visitIntNode(std::shared_ptr<IntNode> node) {
  PrimitiveType primitive;
  primitive.base_type = Type::BaseType::integer;
  return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}


std::any TypeInferWalker::visitRealNode(std::shared_ptr<RealNode> node) {
  PrimitiveType primitive;
  primitive.base_type = Type::BaseType::real;
  return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}


std::any TypeInferWalker::visitCharNode(std::shared_ptr<CharNode> node) {
  PrimitiveType primitive;
  primitive.base_type = Type::BaseType::character;
  return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}


std::any TypeInferWalker::visitBoolNode(std::shared_ptr<BoolNode> node) {
  PrimitiveType primitive;
  primitive.base_type = Type::BaseType::boolean;
  return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}


std::any TypeInferWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {
    //node->type = node->cast_type;

    node->expr->type = node->type; //This gives it the deeclaration type.
    node->expr->accept(*this); // Infer the type of the elements at the expression.
    if (auto vec_type = std::dynamic_pointer_cast<VectorType>(node->cast_type)) {
        vec_type->size->accept(*this); //Infer the type of the size in case there is a complicated expression.
    }
    return node->cast_type;
}


std::any TypeInferWalker::visitVecMatNode(std::shared_ptr<VecMatNode> node) {
    std::unordered_map<Type::BaseType, std::unordered_map<Type::BaseType, Type::BaseType>> prom_matrix = {
          {Type::BaseType::null, {
                {Type::BaseType::null, Type::BaseType::null},
                {Type::BaseType::character, Type::BaseType::null},
                {Type::BaseType::boolean, Type::BaseType::null},
                {Type::BaseType::real, Type::BaseType::null},
                {Type::BaseType::integer, Type::BaseType::null},
          }},
        {Type::BaseType::character, {
                            {Type::BaseType::null, Type::BaseType::null},
                            {Type::BaseType::character, Type::BaseType::character},
                            {Type::BaseType::boolean, Type::BaseType::null},
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
    //If we have the empty vector then we assign it the null type.

    //For now assume we have a vector whose size >= 1
    bool empty_vec;
    int cols = 0;
    bool isMatrix;
    Type::BaseType result_type;
    if (!node->elements.empty()) {
        result_type = std::any_cast<std::shared_ptr<Type>>(node->elements[0]->accept(*this))->getBaseType();
        for (auto i = 0; i < node->elements.size(); i++) {
            auto element = node->elements[i];
            auto el_type = std::any_cast<std::shared_ptr<Type>>(element->accept(*this));
            result_type = prom_matrix[result_type][el_type->getBaseType()];
            if (auto vecType = std::dynamic_pointer_cast<VectorType>(el_type)) {
              auto sNode = std::dynamic_pointer_cast<IntNode>(vecType->size);
              cols = std::max(cols, sNode->val);
              isMatrix = true;
            }
        }
    } else {
        result_type = Type::BaseType::null;
    }

    if (result_type == Type::BaseType::null and !node->elements.empty()) {
        throw TypeError(node->line, "Vector literal with members of incompatible types");
    }
    if (!isMatrix){
      auto size_node = std::make_shared<IntNode>(node->elements.size(), node->line, node->column);
      node->type = std::make_shared<VectorType>(result_type, size_node);
      if (node->is_string) {
        std::dynamic_pointer_cast<VectorType>(node->type)->isString = true;
      }
      return node->type;
      //For every element in the vector
      //Obtain the type of the element
      //If at any point the type cannot be inferred
      //Then we throw an error
    } else {
      auto rowsNode = std::make_shared<IntNode>(node->elements.size(), node->line, node->column);
      auto colsNode = std::make_shared<IntNode>(cols, node->line, node->column);
      node->type = std::make_shared<MatrixType>(result_type, rowsNode, colsNode);
      return  node->type;
    }


}

std::any TypeInferWalker::inferBoolVector(std::shared_ptr<Type> left_side, std::shared_ptr<Type> right_side, std::string op) {

    std::shared_ptr<Type> final_type;

    if (op == "==" || op == "!=") {
        //TODO: Change
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
        //TODO: This is not super robust so might have to change it into something better, I just don't want to call type error in the type infer walker
        final_type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(Type::BaseType::null, nullptr));
        return final_type;

    }
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {

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
}


std::any TypeInferWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {

    if (node->op == "||") {
        const auto left_type = std::any_cast<std::shared_ptr<Type>>(node->l_expr->accept(*this));
        const auto right_type = std::any_cast<std::shared_ptr<Type>>(node->r_expr->accept(*this));
        Type::BaseType base_type = left_type->getBaseType();
        if (base_type == Type::BaseType::integer && right_type->getBaseType() == Type::BaseType::real ) {
            base_type = Type::BaseType::real;
        }
        if (std::dynamic_pointer_cast<VectorType>(left_type) && std::dynamic_pointer_cast<VectorType>(right_type)) {
            auto size = std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<VectorType>(left_type)->size)->val;
            size +=  std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<VectorType>(right_type)->size)->val;
            auto intVecnode = std::make_shared<IntNode>(size,node->line,node->column);
            const auto finalType = std::make_shared<VectorType>(base_type, intVecnode);
            if (std::dynamic_pointer_cast<VectorType>(left_type) ->isString || std::dynamic_pointer_cast<VectorType>(right_type) ->isString) {
                finalType->isString = true;
            }
            node->type = std::dynamic_pointer_cast<Type>(finalType);
            return node->type;
        }if (std::dynamic_pointer_cast<PrimitiveType>(left_type)&&std::dynamic_pointer_cast<VectorType>(right_type)) {
            auto size = std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<VectorType>(right_type)->size)->val;
            size +=  1;
            auto intVecnode = std::make_shared<IntNode>(size,node->line,node->column);
            const auto finalType = std::make_shared<VectorType>(base_type, intVecnode);
            if (std::dynamic_pointer_cast<VectorType>(right_type) ->isString) {
                finalType->isString = true;
            }
            node->type = std::dynamic_pointer_cast<Type>(finalType);

            return node->type;
        }
        if (std::dynamic_pointer_cast<VectorType>(left_type)&&std::dynamic_pointer_cast<PrimitiveType>(right_type)) {
            auto size = std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<VectorType>(left_type)->size)->val;
            size +=  1;
            auto intVecnode = std::make_shared<IntNode>(size,node->line,node->column);
            const auto finalType = std::make_shared<VectorType>(base_type, intVecnode);
            if (std::dynamic_pointer_cast<VectorType>(left_type)->isString) {
                finalType->isString = true;
            }
            node->type = std::dynamic_pointer_cast<Type>(finalType);

            return node->type;
        }
        auto intVecnode = std::make_shared<IntNode>(2,node->line,node->column);
        node->type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, intVecnode));
        return node->type;
    }
    if (node->op == "==" || node->op == "!=" || node->op == "or" ||
      node->op == "not" || node->op == "xor" || node->op == "and" ||
      node->op == "<" || node->op == ">" || node->op == "<=" || node->op == ">=") {

        const auto left_type = std::any_cast<std::shared_ptr<Type>>(node->l_expr->accept(*this));
        const auto right_type = std::any_cast<std::shared_ptr<Type>>(node->r_expr->accept(*this));
        if (std::dynamic_pointer_cast<VectorType>(left_type) || std::dynamic_pointer_cast<VectorType>(right_type)) {
            auto result_type = inferBoolVector(left_type, right_type, node->op);
            node->type = std::any_cast<std::shared_ptr<Type>>(result_type);
            return result_type;
        }
        PrimitiveType primitive;
        primitive.base_type = Type::BaseType::boolean;
        node->type = std::make_shared<PrimitiveType>(primitive);
        return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));

      }
    PrimitiveType primitive;
    const auto left_type = std::any_cast<std::shared_ptr<Type>>(node->l_expr->accept(*this));
    const auto right_type = std::any_cast<std::shared_ptr<Type>>(node->r_expr->accept(*this));

    Type::BaseType base_type;
    if (left_type->getBaseType() == Type::BaseType::real || right_type->getBaseType() == Type::BaseType::real) {
        base_type = Type::BaseType::real;
    } else {
        base_type = Type::BaseType::integer;
    }



    if (left_type->getBaseType() == Type::BaseType::null && right_type->getBaseType() == Type::BaseType::null) {
        throw TypeError(node->line, "Operation between ambiguous types");
    }
    std::shared_ptr<Type> result_type;
    //Determine what will the type be of the function
    //TODO: Make into a look up table i promise this time ill do it
    if (std::dynamic_pointer_cast<VectorType>(left_type) && std::dynamic_pointer_cast<PrimitiveType>(right_type)) {
        if (node->op == "**") {
            result_type = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(base_type));
        } else {
            auto vec_size = std::dynamic_pointer_cast<VectorType>(left_type)->size;

            if (node->op == "by") {
                vec_size = nullptr;
            }
            result_type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, vec_size));
        }


        node->type = result_type;
        return result_type;

    } else if (std::dynamic_pointer_cast<PrimitiveType>(left_type) && std::dynamic_pointer_cast<VectorType>(right_type)) {
        auto vec_size = std::dynamic_pointer_cast<VectorType>(right_type)->size;
        if (node->op == "**") {
            result_type = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(base_type));
        } else {
            if (node->op == "by") {
                vec_size = nullptr;
            }

            result_type =std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, vec_size));
        }
        node->type = result_type;
        return result_type;


    } else if (std::dynamic_pointer_cast<PrimitiveType>(left_type) && std::dynamic_pointer_cast<PrimitiveType>(right_type)) {
        result_type = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(base_type));
        if (node->op == "**") {
            throw TypeError(node->line, "Type mismatch, cannot do dot product between integers");
        }

        node->type = result_type;
        return result_type;

    } else if(std::dynamic_pointer_cast<VectorType>(left_type) && std::dynamic_pointer_cast<VectorType>(right_type)) {
        //Here we assume that the sizes of both operands are equal
        auto vec_size = std::dynamic_pointer_cast<VectorType>(left_type)->size;
        if (node->op == "**") {
            result_type = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(base_type));
        } else {

            if (node->op == "by") {
                vec_size = nullptr;
            }
            result_type = std::dynamic_pointer_cast<Type>(std::make_shared<VectorType>(base_type, vec_size));
        }

        node->type = result_type;
        return result_type;

    }if(std::dynamic_pointer_cast<MatrixType>(left_type) && std::dynamic_pointer_cast<MatrixType>(right_type)) {
        //Here we assume that the sizes of both operands are equal
        if (node->op == "**" ) {

            if (std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<MatrixType>(left_type)->cols)->val
                != std::dynamic_pointer_cast<IntNode>(std::dynamic_pointer_cast<MatrixType>(right_type)->rows)->val) {
                throw SizeError(node->line,"");
            }
            auto retType = std::make_shared<MatrixType>(MatrixType(left_type->getBaseType(),std::dynamic_pointer_cast<MatrixType>(left_type)->rows,std::dynamic_pointer_cast<MatrixType>(right_type)->cols));
            result_type = std::dynamic_pointer_cast<Type>(retType);
        } if(node->op == "+") {
          auto retType = std::make_shared<MatrixType>(MatrixType(left_type->getBaseType(),std::dynamic_pointer_cast<MatrixType>(left_type)->rows,std::dynamic_pointer_cast<MatrixType>(right_type)->cols));
          result_type = std::dynamic_pointer_cast<Type>(retType);
        }
        node->type = result_type;
        return result_type;

    }
}


std::any TypeInferWalker::visitIndexNode(std::shared_ptr<IndexNode> node) {

    if (node->index1) {
        node->index1->accept(*this);
    }
    if (node->index2) {
        node->index2->accept(*this);

    }

    auto collection_type = std::any_cast<std::shared_ptr<Type>>(node->collection->accept(*this));
    std::shared_ptr<PrimitiveType> result_type = std::make_shared<PrimitiveType>(collection_type->getBaseType());
    node->type = result_type;
    return std::dynamic_pointer_cast<Type>(result_type);
}


std::any TypeInferWalker::visitGeneratorNode(std::shared_ptr<GeneratorNode> node) {
    std::shared_ptr<Type> domain1_type;
    std::shared_ptr<Type> domain2_type;
    if (node->vec_expr1) {
        domain1_type = std::any_cast<std::shared_ptr<Type>>(node->vec_expr1->accept(*this));
    }
    if (node->vec_expr2) {
        domain2_type = std::any_cast<std::shared_ptr<Type>>(node->vec_expr2->accept(*this));
    }


    if (auto id1_node = std::dynamic_pointer_cast<IdNode>(node->id1)){

        id1_node->type->setBaseType(domain1_type->getBaseType());

    }
    if (auto id2_node = std::dynamic_pointer_cast<IdNode>(node->id2)) {
        id2_node->type->setBaseType(domain2_type->getBaseType());

    }

    auto rhs_expr = std::any_cast<std::shared_ptr<Type>>(node->rhs_expr->accept(*this));
    if (node->vec_expr1 && !node->vec_expr2) {
        auto vec1_type = std::dynamic_pointer_cast<VectorType>(domain1_type);
        auto result_type = std::make_shared<VectorType>(rhs_expr->getBaseType(), vec1_type->size);
        node->type = result_type;
        return std::dynamic_pointer_cast<Type>(result_type);
    }

    if (node->vec_expr2 && node->vec_expr1) {
        auto vec1_type = std::dynamic_pointer_cast<VectorType>(domain1_type);
        auto vec2_type = std::dynamic_pointer_cast<VectorType>(domain2_type);

        auto result_type = std::make_shared<MatrixType>(rhs_expr->getBaseType(), vec1_type->size, vec2_type->size);
        node->type = result_type;
        return std::dynamic_pointer_cast<Type>(result_type);

    }
    throw SyntaxError(node->line, "Invalid syntax for generator");
}


Type::BaseType TypeInferWalker::declPromMatrix(Type::BaseType from_type, Type::BaseType to_type) {
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

    return prom_matrix[from_type][to_type];
}


void TypeInferWalker::inferDeclVectorWithoutSize(std::shared_ptr<DeclNode> &source_node) {
          //We assume that whatever vector expression that goes in here has the same size.
    auto vec_type = std::dynamic_pointer_cast<VectorType>(source_node->type);

    if (source_node->expr) {

        //We have the source node expression, and the source decl node, what do we want to do?
        auto result_type = std::any_cast<std::shared_ptr<Type>>(source_node->expr->accept(*this));

        //Infer the type and the size.
        auto test_type = std::any_cast<std::shared_ptr<Type>>(result_type);
      //Size is automatically assigned when we visit both vector expressions.
        if (auto expr_type = std::dynamic_pointer_cast<VectorType>(test_type)) {

            //Assign the type to the orginal node, to the symbol, and to the vector.
            auto symbol_node = std::dynamic_pointer_cast<VariableSymbol>(source_node->containing_scope->resolve(source_node->id));
            symbol_node->type = vec_type;
            vec_type->size = expr_type->size;

            if (auto size = std::dynamic_pointer_cast<IntNode>(vec_type->size)) {
                if (size->val == 0) {
                    source_node->expr = nullptr;
                    symbol_node->type = source_node->type;
                }

            }
            auto result_base_type =  declPromMatrix(result_type->getBaseType(), source_node->type->getBaseType());//prom_matrix[result_type->getBaseType()][source_node->type->getBaseType()];
            if (result_base_type == Type::BaseType::null && source_node->expr) {
                throw TypeError(source_node->line, "Type mismatch");
            }

            if (source_node->expr) {
                vec_type->setBaseType(result_base_type);
                source_node->expr->type = vec_type;
            }
        //If the result of the variable sized vector ends up being a primitive type then we throw a size error
        } else if (auto prim_type = std::dynamic_pointer_cast<PrimitiveType>(test_type)) {
            throw SizeError(source_node->line, "Cannot initialize a variable sized vector with a scalar");
        }
    } else {
      //TODO: Maybe make this into its own walker?
      std::string err_msg = "Cannot declare an implicit size vector without also assigning a vector literal";
      throw SizeError(source_node->line, err_msg);
  }

}


void TypeInferWalker::inferDeclVectorWithSize(std::shared_ptr<DeclNode> &source_node) {
    auto symbol_node = std::dynamic_pointer_cast<VariableSymbol>(source_node->containing_scope->resolve(source_node->id));
    std::shared_ptr<Type> result_type;

    //Check if node has expression if so check the type
    if (source_node->expr) {
        result_type = std::any_cast<std::shared_ptr<Type>>(source_node->expr->accept(*this));
    }

    if (auto res_vec = std::dynamic_pointer_cast<VectorType>(result_type)){
        //Check if the size of the vector is 0
        if (auto size = std::dynamic_pointer_cast<IntNode>(res_vec->size)) {
            if (size->val == 0) {
                //Set the type to the node to the type of the original expression

                if (source_node->expr->type->getBaseType() == Type::BaseType::null) {
                    source_node->expr = nullptr; //If the expression between the vector is null then that means that it did not get promoted or the promotion is in valid
                }

            }
        }

            //If the size of the vector is not zero, or if the type is null but has an expression then we must have hit an invalid type
        auto result_base_type = declPromMatrix(result_type->getBaseType(), source_node->type->getBaseType());//prom_matrix[result_type->getBaseType()][node->type->getBaseType()];
        if (result_base_type == Type::BaseType::null && source_node->expr) {
            throw TypeError(source_node->line, "Type mismatch");
        }
            //Else we must be dealing with an empty vector
        if (source_node->expr) {

            //source_node->expr->type = source_node->type;
            //res_vec->size = std::dynamic_pointer_cast<VectorType>(source_node->type)->size;
            source_node->expr->type->setBaseType(result_base_type);
        }

    } else if (auto res_vec = std::dynamic_pointer_cast<PrimitiveType>(result_type)) { //Otherwise if the result is a primitive then
        //In case the result of the vector is an integer.
        //Set the result type of the expression as the type of the source node
        source_node->expr->type = source_node->type;
        //Cast it, if the type is null then we must have hitt an incompatible type
        auto result_base_type = declPromMatrix(result_type->getBaseType(), source_node->type->getBaseType());//prom_matrix[result_type->getBaseType()][node->type->getBaseType()];
        if (result_base_type == Type::BaseType::null) {
            throw TypeError(source_node->line, "Type mismatch");
        }
        //Otherwise set the type to the promoted type.
        source_node->expr->type->setBaseType(result_base_type);

    }
}
void TypeInferWalker::inferVectorTypeAtDecl(std::shared_ptr<DeclNode> &source_node) {

    auto vec_type = std::dynamic_pointer_cast<VectorType>(source_node->type);
    std::shared_ptr<Type> result_type;
    if (source_node->expr) {
        result_type = std::any_cast<std::shared_ptr<Type>>(source_node->expr->accept(*this));
    }
    if (!vec_type->size) { //If the node does not have a size
        inferDeclVectorWithoutSize(source_node);
    } else { //Otherwise if the node has a size
        inferDeclVectorWithSize(source_node);
    }
    //If the result type is null but the vector has not hit any error, that means tthat the vector must be empty,
    //therefore we set the base type of the vector as the type of the result

    if (source_node->expr && source_node->expr->type->getBaseType() == Type::BaseType::null) {
        result_type->setBaseType(source_node->type->getBaseType());
    }
}



void TypeInferWalker::inferVectorTypeAtTupleWithoutSize(std::shared_ptr<ASTNode> &expr_node, std::shared_ptr<Type> &source_type) {

    if (expr_node) {
        //We have the source node expression, and the source decl node, what do we want to do?
        auto result_type = std::any_cast<std::shared_ptr<Type>>(expr_node->accept(*this));
      //Size is automatically assigned when we visit both vector expressions.
        if (auto expr_type = std::dynamic_pointer_cast<VectorType>(result_type)) {
            auto vec_source_type = std::dynamic_pointer_cast<VectorType>(source_type);

            //Assign the type to the orginal node, to the symbol, and to the vector.
            vec_source_type->size = expr_type->size;
            //vec_source_type->setBaseType(result_type->getBaseType());
            auto zero_size = false;
            if (auto size = std::dynamic_pointer_cast<IntNode>(vec_source_type->size)) {
                if (size->val == 0) {
                    zero_size = true;
                    result_type = vec_source_type;
                }
            }
            auto result_base_type =  declPromMatrix(result_type->getBaseType(), source_type->getBaseType());//prom_matrix[result_type->getBaseType()][expr_node->type->getBaseType()];
            if (result_base_type == Type::BaseType::null && expr_node && !zero_size) {
                throw TypeError(expr_node->line, "Type mismatch");
            }

            if (expr_node) {
                vec_source_type->setBaseType(result_base_type);
                expr_node->type = vec_source_type;
            }
        //If the result of the variable sized vector ends up being a primitive type then we throw a size error
        } else if (auto prim_type = std::dynamic_pointer_cast<PrimitiveType>(result_type)) {
            throw SizeError(expr_node->line, "Cannot initialize a variable sized vector with a scalar");
        }
    } else {
      //TODO: Maybe make this into its own walker?
      std::string err_msg = "Cannot declare an implicit size vector without also assigning a vector literal";
      throw SizeError(expr_node->line, err_msg);
    }
}

void TypeInferWalker::inferVectorTypeAtTupleWithSize(std::shared_ptr<ASTNode> &expr_node, std::shared_ptr<Type> &source_type) {
    //auto symbol_node = std::dynamic_pointer_cast<VariableSymbol>(source_node->containing_scope->resolve(source_node->id));
    std::shared_ptr<Type> result_type;

    //Check if node has expression if so check the type
    if (expr_node) {
        result_type = std::any_cast<std::shared_ptr<Type>>(expr_node->accept(*this));
    }

    if (auto res_vec = std::dynamic_pointer_cast<VectorType>(result_type)){
        //Check if the size of the vector is 0
        auto zero_size = false;
        if (auto size = std::dynamic_pointer_cast<IntNode>(res_vec->size)) {
            if (size->val == 0) {
                //Set the type to the node to the type of the original expression

                if (expr_node->type->getBaseType() == Type::BaseType::null) {
                    zero_size = true; //If the expression between the vector is null then that means that it did not get promoted or the promotion is in valid
                    result_type = source_type; //Change the result type to the source tpye as the vector is empty.
                }

            }
        }

            //If the size of the vector is not zero, or if the type is null but has an expression then we must have hit an invalid type
        auto result_base_type = declPromMatrix(result_type->getBaseType(), source_type->getBaseType());//prom_matrix[result_type->getBaseType()][node->type->getBaseType()];
        if (result_base_type == Type::BaseType::null && expr_node && !zero_size) {
            throw TypeError(expr_node->line, "Type mismatch");
        }
            //Else we must be dealing with an empty vector
        if (expr_node) {
            expr_node->type = source_type;
            res_vec->size = std::dynamic_pointer_cast<VectorType>(expr_node->type)->size;
            expr_node->type->setBaseType(result_base_type);
        }

    } else if (auto res_vec = std::dynamic_pointer_cast<PrimitiveType>(result_type)) { //Otherwise if the result is a primitive then
        //In case the result of the vector is an integer.
        //Set the result type of the expression as the type of the source node
        expr_node->type = source_type;
        //Cast it, if the type is null then we must have hitt an incompatible type
        auto result_base_type = declPromMatrix(result_type->getBaseType(), source_type->getBaseType());//prom_matrix[result_type->getBaseType()][node->type->getBaseType()];
        if (result_base_type == Type::BaseType::null) {
            throw TypeError(expr_node->line, "Type mismatch");
        }
        //Otherwise set the type to the promoted type.
        expr_node->type->setBaseType(result_base_type);

    }
}

void TypeInferWalker::inferVectorTypeAtTupl(std::shared_ptr<ASTNode> &expr_node, std::shared_ptr<Type> &source_type) {

    auto vec_type = std::dynamic_pointer_cast<VectorType>(source_type);
    std::shared_ptr<Type> result_type;
    if (expr_node) {
        result_type = std::any_cast<std::shared_ptr<Type>>(expr_node->accept(*this));
    }
    if (!vec_type->size) { //If the node does not have a size
        inferVectorTypeAtTupleWithoutSize(expr_node, source_type);
    } else { //Otherwise if the node has a size
        inferVectorTypeAtTupleWithSize(expr_node, source_type);
    }
    //If the result type is null but the vector has not hit any error, that means tthat the vector must be empty,
    //therefore we set the base type of the vector as the type of the result

    if (expr_node && source_type->getBaseType() == Type::BaseType::null) {
        result_type->setBaseType(source_type->getBaseType());
    }



}






std::any TypeInferWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {


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

    std::shared_ptr<Type> result_type;
    if (node->expr && ! std::dynamic_pointer_cast<TupleType>(node->type)) {
        result_type = std::any_cast<std::shared_ptr<Type>>(node->expr->accept(*this));

    }
    Type::BaseType result_base_type;
    if (not node->type ){
        auto symbol_node = std::dynamic_pointer_cast<VariableSymbol>(node->containing_scope->resolve(node->id));
        symbol_node->type = result_type;
        node->type = result_type;
    } else if (node->expr && !std::dynamic_pointer_cast<TupleType>(node->type)) {
        result_base_type = prom_matrix[result_type->getBaseType()][node->type->getBaseType()]; //Can't put the line here because it fucks up tuples i am tired man pls let me go
    }

    if (std::dynamic_pointer_cast<TupleType>(node->type)) {
        if (std::dynamic_pointer_cast<TupleNode>(node->expr)) {
            //Set the original types to the type of the caster
            std::dynamic_pointer_cast<TupleNode>(node->expr)->type = std::dynamic_pointer_cast<TupleType>(node->type);
            //Fix any errors
            node->expr->accept(*this);
        } else {
            //Visit the types so type gets propagated to all the nodes
            if (node->expr) {
                node->expr->type = node->type;//If its initiated in any other way it means its a filter
                node->expr->accept(*this);
                //However assert your dominance as an alpha chad sigma r-value and share your type anyways.

            }

        }

    }

    else if (auto vec_type = std::dynamic_pointer_cast<VectorType>(node->type)) {
        inferVectorTypeAtDecl(node);
    } else {
        if (node->expr) {
            node->expr->accept(*this);
        }
    }

  return {};
}




std::any  TypeInferWalker::visitTupleNode(std::shared_ptr<TupleNode> node) {

    auto tup_type = std::make_shared<TupleType>();
    std::shared_ptr<Type> result;
    for (auto i = 0; i < node->tuple.size(); i++) {
        auto tup_el = node->tuple[i];
        std::shared_ptr<ASTNode> old_size = nullptr;
        if (! std::dynamic_pointer_cast<TupleType>(node->type)->element_types.empty()) {
            auto node_type = std::dynamic_pointer_cast<TupleType>( node->type);
            auto el_type = node_type->element_types[i];

            if (auto og_type = std::dynamic_pointer_cast<VectorType>(node_type->element_types[i])) {
                if (og_type->size) {
                    old_size = og_type->size;
                }
            }

           result = std::any_cast<std::shared_ptr<Type>>(tup_el->accept(*this));

            if (auto vec_type =std::dynamic_pointer_cast<VectorType>(result)) {
                //Here we assume we have a size
                inferVectorTypeAtTupl(tup_el, el_type);

                if (auto v_type = std::dynamic_pointer_cast<VectorType>(el_type)) {
                    if (old_size) {
                        v_type->size = old_size;
                    }
                }

                result = el_type;
            }
        } else {
            result = std::any_cast<std::shared_ptr<Type>>(tup_el->accept(*this));
        }
        tup_type->element_types.push_back(result);
    }

    return std::dynamic_pointer_cast<Type>(tup_type);
}


std::any TypeInferWalker::visitIdNode(std::shared_ptr<IdNode> node) {
    auto symbol = std::dynamic_pointer_cast<VariableSymbol>(node->containing_scope->resolve(node->id));
    node->type = symbol->type;
    return symbol->type;
}


std::any TypeInferWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {
    node->expr_assign_to->accept(*this);
    node->expr_assign_with->accept(*this);
    return {};
}

std::any TypeInferWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {
    auto symbol = node->containing_scope->resolve(node->id);
    if (std::dynamic_pointer_cast<TupleType>(symbol->type)) {
        node->expr->type = symbol->type;
    }
    node->expr->accept(*this);
    return {};
}




std::any TypeInferWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {

    // visit the i-values to unpack to
    for (auto ivalue : node->ivalues) {
        auto result = ivalue->accept(*this);
    }
    // visit the tuple
    auto result = node->tuple->accept(*this);
    return {};
}


std::any TypeInferWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
    for (auto stat : node->stat_nodes) {
        auto result = stat->accept(*this);
    }
    return {};
}


std::any TypeInferWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {

    auto result = node->expr->accept(*this);
    node->type = std::any_cast<std::shared_ptr<Type>>(result);
    return result;
}


std::any TypeInferWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {

    // visit the loop block
    auto result = node->loop_block->accept(*this);
    return {};
}


std::any TypeInferWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

    // visit the control expression
    auto result = node->contr_expr->accept(*this);

    // visit the loop block
    result = node->loop_block->accept(*this);
    return {};
}


std::any TypeInferWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

    // visit the domain expression
    auto vec_base_type = std::any_cast<std::shared_ptr<Type>>(node->dom_expr->accept(*this))->getBaseType();

    // assign a type to the domain variable
    auto dom_var_sym = std::dynamic_pointer_cast<VariableSymbol>(
        node->loop_block->containing_scope->resolve(node->dom_id));
    dom_var_sym->type = std::make_shared<PrimitiveType>(vec_base_type);

    // visit the loop block
    node->loop_block->accept(*this);
    return {};
}


std::any TypeInferWalker::visitCondNode(std::shared_ptr<CondNode> node) {

    // visit control expression
    auto result = node->contr_expr->accept(*this);

    // visit "if" block
    result = node->if_block->accept(*this);

    // visit "else" block (if it exists)
    if (node->else_block != nullptr) {
        result = node->else_block->accept(*this);
    }
    return {};
}


std::any TypeInferWalker::visitOutStreamNode(std::shared_ptr<OutStreamNode> node) {
    // visit the expression to be printed
    auto result = node->expr->accept(*this);
    //Set the type of the expression to the current node
    auto type = std::any_cast<std::shared_ptr<Type>>(result);
    node->type = type;
    return {};
}


std::any TypeInferWalker::visitInStreamNode(std::shared_ptr<InStreamNode> node) {
    auto result = node->l_value->accept(*this);
    //Set the type of the expression to the current node
    auto type = std::any_cast<std::shared_ptr<Type>>(result);
    node->type = type;
    return {};
}


std::any TypeInferWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
    if (node->func_block) {
        auto result = node->func_block->accept(*this);
    }
    return {};
}


std::any TypeInferWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    if (node->proc_block) {
        auto result = node->proc_block->accept(*this);
    }
    return {};
}

std::any TypeInferWalker::visitFilterNode(std::shared_ptr<FilterNode> node) {
    auto tup_type = std::make_shared<TupleType>();

    auto vec_type = std::any_cast<std::shared_ptr<Type>>(node->vec_expr->accept(*this));
    node->id->type = std::make_shared<PrimitiveType>(vec_type->getBaseType()); //Set the type of the id for boolean comparisons.
    auto id_node = std::dynamic_pointer_cast<IdNode>(node->id);

    auto symbol = node->containing_scope->resolve(id_node->id);//Update the symbol as well.
    symbol->type = node->id->type;


    for (auto element : node->pred_exprs) {
        element->accept(*this);
        if (element->type->getBaseType() != Type::BaseType::boolean) {
            throw TypeError(node->line, "Filter with non boolean predicate");
        }
        //element->type = std::make_shared<PrimitiveType>(Type::BaseType::boolean); //Set the result type of the predicate which will be booolean.
        tup_type->element_types.push_back(std::make_shared<VectorType>(vec_type->getBaseType(), nullptr)); //Make the predicates be the same type as the vector expression
    }
    tup_type->element_types.push_back(std::make_shared<VectorType>(vec_type->getBaseType(), nullptr)); //Add one more vector array for the leftovers
    node->type = tup_type;
    return node->type;
}





std::any TypeInferWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {
    return node->containing_scope->resolve(node->id)->type;
}


std::any TypeInferWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {
    if (node->expr) {
        auto result = node->expr->accept(*this);
    }
    return {};
}


std::any TypeInferWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {

    // visit the id node
    //auto result = node->tuple_id->accept(*this);
    auto tup_symbol = node->containing_scope->resolve(node->tuple_id->id);
    auto tup_type = std::dynamic_pointer_cast<TupleType>(tup_symbol->type);


    // visit the tuple element node
    if (auto int_node = std::dynamic_pointer_cast<IntNode>(node->element)){
        if (int_node->val < 1 || int_node->val > tup_type->element_types.size()) {
            throw SymbolError(node->line, "Tuple index out of bounds");
        }
        node->type = tup_type->element_types[int_node->val - 1];
        return node->type;//node->element->accept(*this);
    }
    if (auto id_node = std::dynamic_pointer_cast<IdNode>(node->element)){
         for (int i = 0; i < static_cast<int>(tup_type->element_types.size()); i++ ){
             if (tup_type->element_names[i] == id_node->id){
                 node->type = tup_type->element_types[i];
                 return node->type;
             }
         }
    }

    //Otherwise just return null

    std::shared_ptr<PrimitiveType> null_type = std::make_shared<PrimitiveType>(Type::BaseType::null);
    return std::dynamic_pointer_cast<Type>(null_type);
}
std::any TypeInferWalker::visitLengthCallNode(std::shared_ptr<LengthCallNode> node) {
    PrimitiveType primitive;
    primitive.base_type = Type::BaseType::integer;
    return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}

std::any TypeInferWalker::visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) {
    auto result = std::any_cast<std::shared_ptr<Type>>(node->vector->accept(*this));
    node->type = result;
    return result;
}

std::any TypeInferWalker::visitRowsCallNode(std::shared_ptr<RowsCallNode> node) {
  PrimitiveType primitive;
  primitive.base_type = Type::BaseType::integer;
  return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}

std::any TypeInferWalker::visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node) {
  PrimitiveType primitive;
  primitive.base_type = Type::BaseType::integer;
  return std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
}

std::any TypeInferWalker::visitRangeNode(std::shared_ptr<RangeNode> node) {
    auto range_type = std::make_shared<VectorType>(Type::BaseType::integer, nullptr);
    node->type = range_type;
    return node->type;
}
std::any TypeInferWalker::visitFormatCallNode(std::shared_ptr<FormatCallNode> node) {
    const auto result = std::any_cast<std::shared_ptr<Type>>(node->scalar->accept(*this));
    const auto resultBaseType = result->getBaseType();
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