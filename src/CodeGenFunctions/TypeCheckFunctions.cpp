#include "BackEnd.h"
#include <iostream>

bool BackEnd::isSameType(const mlir::Value value1,const mlir::Value value2) {
    if (isIntegerType(value1)&& isIntegerType(value2)) {
        return true;
    }if (isBoolType(value1)&& isBoolType(value2)) {
        return true;
    }if (isCharType(value1)&& isCharType(value2)) {
        return true;
    }if (isFloatType(value1)&& isFloatType(value2)) {
        return true;
    }if (isStructType(value1) && isStructType(value2)) {
        return true;
    }
    return false;
}

bool BackEnd::isSameType(const mlir::Value value1, const std::shared_ptr<Type> &type) {
    if (auto primType = std::dynamic_pointer_cast<PrimitiveType>(type)){
        if (primType->base_type == Type::BaseType::boolean && isBoolType(value1)){
            return true;
        }if (primType->base_type == Type::BaseType::character && isCharType(value1)) {
            return true;
        }if (primType->base_type == Type::BaseType::real && isFloatType(value1))  {
            return true;
        }if (primType->base_type == Type::BaseType::integer && isIntegerType(value1))  {
            return true;
        }
    }else {
        if (isStructType(value1)) {
            return true;
        }
    }
    return false;
}

bool BackEnd::isIntegerType(const mlir::Value value) {
    return value.getType().isInteger(32);
}


bool BackEnd::isFloatType(const mlir::Value value) {
    return value.getType().isa<mlir::Float32Type>();
}


bool BackEnd::isBoolType(const mlir::Value value) {
    return value.getType().isInteger(1);
}




bool BackEnd::isCharType(const mlir::Value value) {
    return value.getType().isInteger(8);
}


bool BackEnd::isStructType(mlir::Value value) {
  mlir::Type type = value.getType();
  if (type.isa<mlir::LLVM::LLVMPointerType>()) {
    // TODO: change this when doing vectors and mats
    return true;
  }
  return false;
}


mlir::Type BackEnd::getMLIRTypeFromTypeObject(const std::shared_ptr<Type> &type) {
    const mlir::Type int32_type = builder->getI32Type();
    const mlir::Type float32_type = builder->getF32Type();
    const mlir::Type bool_type = builder->getI1Type();
    const mlir::Type char_type = builder->getI8Type();
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);

    // no type
    if (type == nullptr)
        return {};

    // PRIMITIVES
    if (auto primitive = std::dynamic_pointer_cast<PrimitiveType>(type)) {
        // boolean
        if (primitive->base_type == Type::BaseType::boolean)
            return bool_type;
        // character
        if (primitive->base_type == Type::BaseType::character)
            return char_type;
        // integer
        if (primitive->base_type == Type::BaseType::integer)
            return int32_type;
        // real
        if (primitive->base_type == Type::BaseType::real)
            return float32_type;
    }
    return ptr_type;
}


mlir::Type BackEnd::getMLIRTypeFromBaseType(const std::shared_ptr<Type> &type) {
    const mlir::Type int32_type = builder->getI32Type();
    const mlir::Type float32_type = builder->getF32Type();
    const mlir::Type bool_type = builder->getI1Type();
    const mlir::Type char_type = builder->getI8Type();
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);

    // no type
    if (type == nullptr)
        return {};

    // PRIMITIVES
    if (auto primitive = std::dynamic_pointer_cast<PrimitiveType>(type)) {
        // boolean
        if (primitive->base_type == Type::BaseType::boolean)
            return bool_type;
        // character
        if (primitive->base_type == Type::BaseType::character)
            return char_type;
        // integer
        if (primitive->base_type == Type::BaseType::integer)
            return int32_type;
        // real
        if (primitive->base_type == Type::BaseType::real)
            return float32_type;
    } else if (auto vec_type = std::dynamic_pointer_cast<VectorType>(type)){
        // boolean
        if (vec_type->getBaseType() == Type::BaseType::boolean)
            return bool_type;
        // character
        if (vec_type->getBaseType() == Type::BaseType::character)
            return char_type;
        // integer
        if (vec_type->getBaseType() == Type::BaseType::integer)
            return int32_type;
        // real
        if (vec_type->getBaseType() == Type::BaseType::real)
            return float32_type;

    }
    return ptr_type;
}
