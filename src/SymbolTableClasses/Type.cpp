#include "Type.h"

#include "AST.h"
#include <iostream>
#include <ostream>
#include <sstream>
#include <llvm/ADT/STLExtras.h>
#include <mlir/Dialect/LLVMIR/LLVMOpsAttrDefs.h.inc>


bool PrimitiveType::isEqualType(std::shared_ptr<Type> type) {

    // try a dynamic_ptr_cast to a Primitive Type, if successful, check if the base type is equal
    if (auto type_to_compare = std::dynamic_pointer_cast<PrimitiveType>(type)) {
        return this->base_type == type_to_compare->base_type;
    }
    return false;
}


bool VectorType::isEqualType(std::shared_ptr<Type> type) {
    //As we can do upcasting from primitives to vectors then we can safely say that if both vectors have the same type or they are
    //primitive then they are equal
    // check if the other type is a vector

    //If my type is a primitive type then check if both base types are equal, if so return.
    if (auto type_to_compare = std::dynamic_pointer_cast<PrimitiveType>(type)) {
        return this->base_type == type_to_compare->base_type;

    }
    auto type_to_compare = std::dynamic_pointer_cast<VectorType>(type);

    if (!type_to_compare)
        return false;
    // check if the base type is the same
    if (this->base_type != type_to_compare->base_type)
        return false;

    return true;
}


bool MatrixType::isEqualType(std::shared_ptr<Type> type) {

  if (auto type_to_compare = std::dynamic_pointer_cast<PrimitiveType>(type)) {
    return this->base_type == type_to_compare->base_type;

  }

  if (auto type_to_compare = std::dynamic_pointer_cast<VectorType>(type)){
    return this->base_type == type_to_compare->base_type;
  }

  // check if the other type is a matrix
  auto type_to_compare = std::dynamic_pointer_cast<MatrixType>(type);
  if (!type_to_compare)
        return false;

  // check if the base type is the same
  if (this->base_type != type_to_compare->base_type)
    return false;

  return true;
}


bool TupleType::isEqualType(std::shared_ptr<Type> type) {

    // check if the other type is a tuple
    auto type_to_compare = std::dynamic_pointer_cast<TupleType>(type);
    if (!type_to_compare)
        return false;

    // check if both tuples are the same size
    if (this->element_types.size() != type_to_compare->element_types.size())
        return false;

    // compare each element in both tuples
    for (int i = 0; i < static_cast<int>(this->element_types.size()) ; i++) {
        if (not this->element_types[i]->isEqualType(type_to_compare->element_types[i]))
            return false;
    }
    return true;
}


std::string VectorType::getTypeAsString() {
    std::string base_str;
    std::stringstream ss;

    switch (base_type) {
        case Type::BaseType::integer:
            base_str = "integer";
        break;
        case Type::BaseType::boolean:
            base_str =  "boolean";
        break;
        case Type::BaseType::character:
            base_str =  "character";
        break;
        case Type::BaseType::real:
            base_str =  "real";
        break;
        default:
            base_str = "null";  // Handle unexpected base_type values
        break;
    }
    std::string size_str;
    if (size == nullptr)
        size_str = "inferred size";
    else if (auto int_node = std::dynamic_pointer_cast<IntNode>(size))
        size_str = std::to_string(int_node->val);
    else
        size_str = "undetermined";

    ss << base_str << '[' << size_str << ']';
    return ss.str();
}


std::string MatrixType::getTypeAsString() {
    std::string base_str;
    std::stringstream ss;

    switch (base_type) {
        case Type::BaseType::integer:
            base_str = "integer";
        break;
        case Type::BaseType::boolean:
            base_str =  "boolean";
        break;
        case Type::BaseType::character:
            base_str =  "character";
        break;
        case Type::BaseType::real:
            base_str =  "real";
        break;
        default:
            base_str = "null";
        break;
    }
    std::string row_str;
    std::string col_str;

    if (rows == nullptr)
        row_str = "inferred size";
    else if (auto int_node = std::dynamic_pointer_cast<IntNode>(rows))
        row_str = std::to_string(int_node->val);
    else
        row_str = "undetermined";

    if (cols == nullptr)
        col_str = "inferred size";
    else if (auto int_node = std::dynamic_pointer_cast<IntNode>(cols))
        col_str = std::to_string(int_node->val);
    else
        col_str = "undetermined";

    ss << base_str << '[' << row_str << ',' << col_str << ']';
    return ss.str();
}


std::string TupleType::getTypeAsString() {
    std::stringstream ss;

    ss << "tuple(" << this->element_types[0]->getTypeAsString();
    for (int i = 1; i < static_cast<int>(this->element_types.size()); i++) {
        ss << ',' << this->element_types[1]->getTypeAsString();
    }
    ss << ')';
    return ss.str();
}


std::string PrimitiveType::getTypeAsString() {
    std::string base_str;
    switch (base_type) {
        case Type::BaseType::integer:
            base_str = "integer";
            break;
        case Type::BaseType::boolean:
            base_str =  "boolean";
            break;
        case Type::BaseType::character:
            base_str =  "character";
            break;
        case Type::BaseType::real:
            base_str =  "real";
            break;
        default:
            base_str = "null";
            break;
    }
    return base_str;
}





Type::BaseType PrimitiveType::getBaseType() {
    return this->base_type;
}

void PrimitiveType::setBaseType(BaseType type) {
    this->base_type = type;
}

Type::BaseType VectorType::getBaseType() {
    return this->base_type;
}
void VectorType::setBaseType(BaseType type) {
    this->base_type = type;
}


Type::BaseType MatrixType::getBaseType() {
    return this->base_type;
}

void MatrixType::setBaseType(BaseType type) {
    this->base_type = type;
}
Type::BaseType TupleType::getBaseType() {
    return BaseType::null;
}
