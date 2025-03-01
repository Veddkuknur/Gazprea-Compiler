
#include "BackEnd.h"
#include "Type.h"


mlir::Value BackEnd::typeCast(const mlir::Value value, const Type::BaseType toType) const {
    if (isIntegerType(value)) {
        return castInt(value,toType);
    }if (isCharType(value)) {
        return castChar(value,toType);
    }if (isFloatType(value)) {
        return castFloat(value,toType);
    }if (isBoolType(value)) {
        return castBool(value,toType);
    }
    throw std::logic_error("Not supported");
}



mlir::Value BackEnd::scalarToVector(mlir::Value scalar, mlir::Value arr_size, std::shared_ptr<Type> &type) {


    std::stack<mlir::OpBuilder::InsertPoint> ipStack;



    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);
    scalar = typeCast(scalar, type->getBaseType());

    auto result_vec = createVector(arr_size, type);
    //How do we make sure we are not putting a space in the last element?
    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);
    storeArrVal(result_vec, index, scalar, type);



    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();

    return result_vec;
}


Type::BaseType BackEnd::promoteBaseType(std::shared_ptr<Type> &left_type, std::shared_ptr<Type> &right_type) {



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



    return prom_matrix[left_type->getBaseType()][right_type->getBaseType()];




}


mlir::Value BackEnd::promoteVectorTo(mlir::Value vector_addr, std::shared_ptr<Type> &old_type, std::shared_ptr<Type> &new_type) {


    if (old_type->getBaseType() == new_type->getBaseType()) {
        return vector_addr;
    }
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //We promote the vector to the correct type and free the original address
    auto size = getArrSize(vector_addr);
    auto result_vec =createVector(size, new_type);

    //While i < arr size
    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    auto old_point = builder->saveInsertionPoint();
    builder->setInsertionPointToStart(for_body);
    //Obtain the element for the old vector and cast it onto the new vector.

    auto old_val = loadArrVal(vector_addr, index, old_type);

    auto casted_val = typeCast(old_val, new_type->getBaseType());
    storeArrVal(result_vec, index, casted_val, new_type);



    builder->restoreInsertionPoint(old_point);
    freeVector(vector_addr);
    return result_vec;

}




mlir::Value BackEnd::castInt(mlir::Value value, const Type::BaseType toType) const {
    if (toType == Type::BaseType::boolean) {
        mlir::Value constI32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
        mlir::Value constI1_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI1Type(), 1);
        mlir::Value constI1_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI1Type(), 0);
        mlir::Value isZero = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, value, constI32_zero);
        return builder->create<mlir::LLVM::SelectOp>(loc, isZero, constI1_zero, constI1_one);
    }
    if (toType == Type::BaseType::real) {
        return builder->create<mlir::LLVM::SIToFPOp>(loc, builder->getF32Type(), value);
    }
    if (toType == Type::BaseType::character) {
        mlir::Value constI32_255 = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), builder->getIntegerAttr(builder->getI32Type(), 0xFF));
        mlir::Value mod256Result = builder->create<mlir::LLVM::AndOp>(loc, value, constI32_255);
        return builder->create<mlir::LLVM::TruncOp>(loc, builder->getI8Type(), mod256Result);
    }
    if (toType == Type::BaseType::integer) {
        return value;
    }
    throw std::logic_error("Not supported");
}


mlir::Value BackEnd::castChar(mlir::Value value, const Type::BaseType toType) const {
    if (toType == Type::BaseType::boolean) {
        mlir::Value constI8_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI8Type(), static_cast<int8_t>(0));
        mlir::Value constI1_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI1Type(), 1);
        mlir::Value constI1_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI1Type(), 0);
        mlir::Value isZero = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, value, constI8_zero);
        return builder->create<mlir::LLVM::SelectOp>(loc, isZero, constI1_zero, constI1_one);
    }
    if (toType == Type::BaseType::real) {
        auto castIntTemp = builder->create<mlir::LLVM::ZExtOp>(loc, builder->getI32Type(), value);
        return builder->create<mlir::LLVM::SIToFPOp>(loc, builder->getF32Type(), castIntTemp);
    }
    if (toType == Type::BaseType::integer) {
        return builder->create<mlir::LLVM::ZExtOp>(loc, builder->getI32Type(), value);
    }
    if (toType == Type::BaseType::character) {
        return value;
    }
    throw std::logic_error("Not supported");
}


mlir::Value BackEnd::castFloat(mlir::Value value, const Type::BaseType toType) const {
    if (toType == Type::BaseType::integer) {
        return builder->create<mlir::LLVM::FPToSIOp>(loc, builder->getI32Type(), value);
    }
    if (toType == Type::BaseType::real) {
        return value;
    }
    throw std::logic_error("Not supported");
}


mlir::Value BackEnd::castBool(mlir::Value value, const Type::BaseType toType) const {
    if (toType == Type::BaseType::character) {
        mlir::Type char_type = builder->getI8Type();
        mlir::Value constI8_one = builder->create<mlir::LLVM::ConstantOp>(loc,  char_type, static_cast<int8_t>(1));
        mlir::Value constI8_zero = builder->create<mlir::LLVM::ConstantOp>(loc, char_type,  static_cast<int8_t>(0));
        return builder->create<mlir::LLVM::SelectOp>(loc, value, constI8_one,constI8_zero);
    }
    if (toType == Type::BaseType::real) {
        auto castIntTemp = builder->create<mlir::LLVM::ZExtOp>(loc, builder->getI32Type(), value);
        return builder->create<mlir::LLVM::SIToFPOp>(loc, builder->getF32Type(), castIntTemp);
    }
    if (toType == Type::BaseType::integer) {
        return builder->create<mlir::LLVM::ZExtOp>(loc, builder->getI32Type(), value);
    }
    if (toType == Type::BaseType::boolean) {
        return value;
    }
    throw std::logic_error("Not supported");
}
