#include "BackEnd.h"

mlir::Value BackEnd::emitAdd(mlir::Value left_side, mlir::Value right_side) {
    if (isFloatType(right_side) and isFloatType(left_side)){
        return builder->create<mlir::LLVM::FAddOp>(loc, builder->getF32Type(), left_side, right_side);
    } else if (isFloatType(left_side) and isIntegerType(right_side)){
        right_side = castInt(right_side,Type::BaseType::real);
        return builder->create<mlir::LLVM::FAddOp>(loc, builder->getF32Type(), left_side, right_side);
    } else if (isIntegerType(left_side) and isFloatType(right_side)){
        left_side = castInt(left_side,Type::BaseType::real);
        return builder->create<mlir::LLVM::FAddOp>(loc, builder->getF32Type(), left_side, right_side);
    } if (isIntegerType(left_side) and isIntegerType(right_side)){
        return  builder->create<mlir::LLVM::AddOp>(loc, builder->getI32Type(), left_side, right_side);
    }
    throw std::invalid_argument("Arithmetic between incompatible types. Must be an integer or real.");
}
mlir::Value BackEnd::emitExpo(mlir::Value left_side, mlir::Value right_side) {

    mlir::LLVM::LLVMFuncOp float_zero_exp = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("floatZeroExp");

    if (isFloatType(left_side) && isIntegerType(right_side)) {
        right_side = castInt(right_side,Type::BaseType::real);
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_exp, mlir::ValueRange{left_side, right_side});

        return builder->create<mlir::LLVM::PowOp>(loc, left_side, right_side);
    }if (isIntegerType(left_side) && isFloatType(right_side)) {
        left_side = castInt(left_side,Type::BaseType::real);
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_exp, mlir::ValueRange{left_side, right_side});

        return builder->create<mlir::LLVM::PowOp>(loc, left_side, right_side);
    } if (isIntegerType(left_side) && isIntegerType(right_side)) {
        left_side = castInt(left_side,Type::BaseType::real);
        right_side = castInt(right_side,Type::BaseType::real);
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_exp, mlir::ValueRange{left_side, right_side});

        auto result = builder->create<mlir::LLVM::PowOp>(loc, left_side, right_side);
        return castFloat(result,Type::BaseType::integer);
    }if (isFloatType(left_side) && isFloatType(right_side)) {
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_exp, mlir::ValueRange{left_side, right_side});

        return builder->create<mlir::LLVM::PowOp>(loc, left_side, right_side);
    }
    throw std::invalid_argument("Arithmetic between incompatible types. Must be an integer or real.");
}
mlir::Value BackEnd::emitRem(mlir::Value left_side, mlir::Value right_side) {
    mlir::LLVM::LLVMFuncOp int_zero_div = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("intDivZero");
    mlir::LLVM::LLVMFuncOp float_zero_div = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("floatDivZero");

    if (isFloatType(left_side) && isFloatType(right_side)) {
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_div, right_side);

        return builder->create<mlir::LLVM::FRemOp>(loc, left_side, right_side);
    } else if (isFloatType(left_side) && isIntegerType(right_side)) {
        right_side = castInt(right_side,Type::BaseType::real);
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_div, right_side);

        return builder->create<mlir::LLVM::FRemOp>(loc, left_side, right_side);
    } else if (isIntegerType(left_side) && isFloatType(right_side)) {
        left_side = castInt(left_side,Type::BaseType::real);
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_div, right_side);

        return builder->create<mlir::LLVM::FRemOp>(loc, left_side, right_side);
    } if (isIntegerType(left_side) && isIntegerType(right_side)){
        builder->create<mlir::LLVM::CallOp>(loc, int_zero_div, right_side);

        return builder->create<mlir::LLVM::SRemOp>(loc, left_side, right_side);
    }
    throw std::invalid_argument("Arithmetic between incompatible types. Must be an integer or real.");
}
mlir::Value BackEnd::emitDiv(mlir::Value left_side, mlir::Value right_side) {

    //Find functions to

    mlir::LLVM::LLVMFuncOp int_zero_div = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("intDivZero");
    mlir::LLVM::LLVMFuncOp float_zero_div = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("floatDivZero");

    if (isFloatType(left_side) && isFloatType(right_side)) {
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_div, right_side);

        return builder->create<mlir::LLVM::FDivOp>(loc, left_side, right_side);
    } else if (isFloatType(left_side) && isIntegerType(right_side)) {
        right_side = castInt(right_side,Type::BaseType::real);
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_div, right_side);

        return builder->create<mlir::LLVM::FDivOp>(loc, left_side, right_side);
    } else if (isIntegerType(left_side) && isFloatType(right_side)) {
        left_side = castInt(left_side,Type::BaseType::real);
        builder->create<mlir::LLVM::CallOp>(loc, float_zero_div, right_side);

        return builder->create<mlir::LLVM::FDivOp>(loc, left_side, right_side);
    } if (isIntegerType(left_side) && isIntegerType(right_side)){

        builder->create<mlir::LLVM::CallOp>(loc, int_zero_div, right_side);

        return builder->create<mlir::LLVM::SDivOp>(loc, left_side, right_side);
    }
    throw std::invalid_argument("Arithmetic between incompatible types. Must be an integer or real.");
}
mlir::Value BackEnd::emitMul(mlir::Value left_side, mlir::Value right_side) {
    if (isFloatType(right_side) && isFloatType(left_side)){
        return builder->create<mlir::LLVM::FMulOp>(loc, builder->getF32Type(), left_side, right_side);
    } else if (isFloatType(left_side) && isIntegerType(right_side)){
        right_side = castInt(right_side,Type::BaseType::real);
        return builder->create<mlir::LLVM::FMulOp>(loc, builder->getF32Type(), left_side, right_side);
    } else if (isIntegerType(left_side) && isFloatType(right_side)){
        left_side = castInt(left_side,Type::BaseType::real);
        return builder->create<mlir::LLVM::FMulOp>(loc, builder->getF32Type(), left_side, right_side);
    } if (isIntegerType(left_side) && isIntegerType(right_side)){
        return  builder->create<mlir::LLVM::MulOp>(loc, builder->getI32Type(), left_side, right_side);
    }
    throw std::invalid_argument("Arithmetic between incompatible types. Must be an integer or real.");
}
mlir::Value BackEnd::emitSub(mlir::Value left_side, mlir::Value right_side) {
    auto floatType = builder->getF32Type();
    auto intType = builder->getI32Type();
    if (isFloatType(left_side) && isFloatType(right_side)) {
        return builder->create<mlir::LLVM::FSubOp>(loc, floatType, left_side, right_side);
    } if (isFloatType(left_side) && isIntegerType(right_side)) {
        // Convert right_side integer to float
        right_side = builder->create<mlir::LLVM::SIToFPOp>(loc, floatType, right_side);
        return builder->create<mlir::LLVM::FSubOp>(loc, floatType, left_side, right_side);
    } if (isIntegerType(left_side) && isFloatType(right_side)) {
        // Convert left_side integer to float
        left_side = builder->create<mlir::LLVM::SIToFPOp>(loc, floatType, left_side);
        return builder->create<mlir::LLVM::FSubOp>(loc, floatType, left_side, right_side);
    }if (isIntegerType(left_side) && isIntegerType(right_side)) {
        return builder->create<mlir::LLVM::SubOp>(loc, intType, left_side, right_side);
    }
    throw std::invalid_argument("Arithmetic between incompatible types. Must be an integer or real.");
}

mlir::Value BackEnd::emitLessThan(mlir::Value left_side, mlir::Value right_side) {



    if (isFloatType(right_side) and isFloatType(left_side)){
        return realLessThan(left_side, right_side);
    } else if (isFloatType(left_side) and isIntegerType(right_side)){
        right_side = castInt(right_side,Type::BaseType::real);
        return realLessThan(left_side, right_side);
    } else if (isIntegerType(left_side) and isFloatType(right_side)){
        left_side = castInt(left_side,Type::BaseType::real);
        return realLessThan(left_side, right_side);
    } if (isIntegerType(left_side) and isIntegerType(right_side)){
        return intLessThan(left_side, right_side);
    }
    throw std::invalid_argument("Comparison between incompatible types. Must be an integer or real.");
}

mlir::Value BackEnd::emitUnaryMinus(mlir::Value value) {
    if (isFloatType(value)) {
        mlir::Value constF32_negativeOne = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getF32Type(), builder->getFloatAttr(builder->getF32Type(), -1.0f));
        return builder->create<mlir::LLVM::FMulOp>(loc, value, constF32_negativeOne);
    }
    mlir::Value constI32_negativeOne =  builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), -1);
    return builder->create<mlir::LLVM::MulOp>(loc, builder->getI32Type(), constI32_negativeOne, value);
}
mlir::Value BackEnd::intLessThan(mlir::Value left_side, mlir::Value right_side){
    auto result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, left_side, right_side);
    return result;

}
mlir::Value BackEnd::realLessThan(mlir::Value left_side, mlir::Value right_side){
    auto result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::olt, left_side, right_side);
    return result;

}


mlir::Value BackEnd::emitLessEq(mlir::Value left_side, mlir::Value right_side) {
    if (isFloatType(right_side) and isFloatType(left_side)){
        return realLessEq(left_side, right_side);
    } else if (isFloatType(left_side) and isIntegerType(right_side)){
        right_side = castInt(right_side,Type::BaseType::real);
        return realLessEq(left_side, right_side);
    } else if (isIntegerType(left_side) and isFloatType(right_side)){
        left_side = castInt(left_side,Type::BaseType::real);
        return realLessEq(left_side, right_side);
    }if (isIntegerType(left_side) and isIntegerType(right_side)){
        return intLessEq(left_side, right_side);
    }
    throw std::invalid_argument("Comparison between incompatible types. Must be an integer or real.");
}

mlir::Value BackEnd::intLessEq(mlir::Value left_side, mlir::Value right_side){
    auto result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sle, left_side, right_side);
    return result;

}
mlir::Value BackEnd::realLessEq(mlir::Value left_side, mlir::Value right_side){
    auto result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::ole, left_side, right_side);
    return result;

}

mlir::Value BackEnd::emitGreaterThan(mlir::Value left_side, mlir::Value right_side) {
    if (isFloatType(right_side) and isFloatType(left_side)){
        return realGreaterThan(left_side, right_side);
    } else if (isFloatType(left_side) and isIntegerType(right_side)){
        right_side = castInt(right_side,Type::BaseType::real);
        return realGreaterThan(left_side, right_side);
    } else if (isIntegerType(left_side) and isFloatType(right_side)){
        left_side = castInt(left_side,Type::BaseType::real);
        return realGreaterThan(left_side, right_side);
    }if (isCharType(left_side) || isCharType(right_side)) {
        throw std::runtime_error("Comparison between incompatible types. Must be an integer or real.");
    }
    return intGreaterThan(left_side, right_side);
}

mlir::Value BackEnd::intGreaterThan(mlir::Value left_side, mlir::Value right_side){
    auto result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sgt, left_side, right_side);
    return result;

}
mlir::Value BackEnd::realGreaterThan(mlir::Value left_side, mlir::Value right_side){
    auto result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::ogt, left_side, right_side);
    return result;

}

mlir::Value BackEnd::emitGreaterEq(mlir::Value left_side, mlir::Value right_side) {
    if (isFloatType(right_side) and isFloatType(left_side)){
        return realGreaterEq(left_side, right_side);
    } else if (isFloatType(left_side) and isIntegerType(right_side)){
        right_side = builder->create<mlir::LLVM::SIToFPOp>(loc,builder->getF32Type(), right_side);
        return realGreaterEq(left_side, right_side);
    } else if (isIntegerType(left_side) and isFloatType(right_side)){
        left_side = castInt(left_side,Type::BaseType::real);
        return realGreaterEq(left_side, right_side);
    } if (isCharType(left_side) || isCharType(right_side)) {
        throw std::runtime_error("Comparison between incompatible types. Must be an integer or real.");
    }
    return intGreaterEq(left_side, right_side);

}



mlir::Value BackEnd::intGreaterEq(mlir::Value left_side, mlir::Value right_side){
    auto result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::sge, left_side, right_side);
    return result;

}
mlir::Value BackEnd::realGreaterEq(mlir::Value left_side, mlir::Value right_side){
    auto result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::oge, left_side, right_side);
    return result;

}

mlir::Value BackEnd::emitEqualTo(mlir::Value left_side, mlir::Value right_side) {
    if (isFloatType(right_side) and isFloatType(left_side)){
        return realEqualTo(left_side, right_side);
    } else if (isFloatType(left_side) and isIntegerType(right_side)){
        right_side = castInt(right_side,Type::BaseType::real);
        return realEqualTo(left_side, right_side);
    } else if (isIntegerType(left_side) and isFloatType(right_side)){
        left_side = castInt(left_side,Type::BaseType::real);
        return realEqualTo(left_side, right_side);
    }
    return intEqualTo(left_side, right_side);
}

mlir::Value BackEnd::emitEqualTo(const std::vector<mlir::Value> &left_side, const std::vector<mlir::Value> &right_side,
    const std::shared_ptr<TupleType> &leftTupleType,const std::shared_ptr<TupleType> &rightTupleType) {
    std::cout<<"reached here"<<std::endl;
    if (left_side.size()!=right_side.size()) {
        return emitBool(false);
    }
    std::vector<mlir::Value> results;
    results.reserve(left_side.size());
    for (unsigned i = 0; i < left_side.size(); i++) {
        if (!isSameType(left_side[i], right_side[i])) {
            return emitBool(false);
        }

        if (std::dynamic_pointer_cast<VectorType>(leftTupleType->element_types[i])) {
            results.push_back(emitVectorEqualTo(left_side[i], right_side[i], leftTupleType->element_types[i]));
        }else {
            results.push_back(emitEqualTo(left_side[i], right_side[i]));
        }

    }
    auto result = results[0];
    for (size_t i = 1; i < results.size(); ++i) {
        result = builder->create<mlir::LLVM::AndOp>(loc, result, results[i]);
    }
    return result;
}

mlir::Value BackEnd::intEqualTo(mlir::Value left_side, mlir::Value right_side) {
    auto result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::eq, left_side, right_side);
    return result;

}

mlir::Value BackEnd::realEqualTo(mlir::Value left_side, mlir::Value right_side) {
    auto result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::oeq, left_side, right_side);
    return result;
}


mlir::Value BackEnd::emitNotEqual(mlir::Value left_side, mlir::Value right_side) {
    if (isFloatType(right_side) and isFloatType(left_side)){
        return realNotEqual(left_side, right_side);
    } else if (isFloatType(left_side) and isIntegerType(right_side)){
        right_side = castInt(right_side,Type::BaseType::real);
        return realNotEqual(left_side, right_side);
    } else if (isIntegerType(left_side) and isFloatType(right_side)){
        left_side = castInt(left_side,Type::BaseType::real);
        return realNotEqual(left_side, right_side);
    }
    return intNotEqual(left_side, right_side);

}
mlir::Value BackEnd::emitNotEqual(const std::vector<mlir::Value> &left_side, const std::vector<mlir::Value> &right_side) {
    if (left_side.size()!=right_side.size()) {
        return emitBool(true);
    }
    std::vector<mlir::Value> results;
    results.reserve(left_side.size());
    for (unsigned i = 0; i < left_side.size(); i++) {
        if (!isSameType(left_side[i], right_side[i])) {
            return emitBool(true);
        }
        results.push_back(emitNotEqual(left_side[i], right_side[i]));
    }
    auto result = results[0];
    for (size_t i = 1; i < results.size(); ++i) {
        result = builder->create<mlir::LLVM::OrOp>(loc, result, results[i]);
    }
    return result;
}
mlir::Value BackEnd::intNotEqual(mlir::Value left_side, mlir::Value right_side) {
    auto result = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, left_side, right_side);
    return result;

}
mlir::Value BackEnd::realNotEqual(mlir::Value left_side, mlir::Value right_side) {
    auto result = builder->create<mlir::LLVM::FCmpOp>(loc, mlir::LLVM::FCmpPredicate::one, left_side, right_side);
    return result;
}


mlir::Value BackEnd::emitAnd(mlir::Value left_side, mlir::Value right_side) {
    if (isBoolType(right_side) and isBoolType(left_side)) {
        return builder->create<mlir::LLVM::AndOp>(loc, left_side, right_side);
    }
    throw std::runtime_error("comparison between non-boolean types");
}

mlir::Value BackEnd::emitOr(mlir::Value left_side, mlir::Value right_side) {
    if (isBoolType(right_side) and isBoolType(left_side)) {
        return builder->create<mlir::LLVM::OrOp>(loc, left_side, right_side);
    }
    throw std::runtime_error("comparison between non-boolean types");
}
mlir::Value BackEnd::emitXor(mlir::Value left_side, mlir::Value right_side) {
    if (isBoolType(right_side) and isBoolType(left_side)) {
        return builder->create<mlir::LLVM::XOrOp>(loc, left_side, right_side);
    }
    throw std::runtime_error("comparison between non-boolean types");

}
mlir::Value BackEnd::emitUnaryNot(mlir::Value value) {
    if (isBoolType(value)) {
        auto one_const = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI1Type(),  builder->getIntegerAttr(builder->getI1Type(), 1));
        return emitXor(value, one_const);
    }
    throw std::runtime_error("");
}


mlir::Value BackEnd::emitVectorAdd(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitAdd(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();

    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}



mlir::Value BackEnd::emitVectorSub(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays


    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitSub(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}



mlir::Value BackEnd::emitVectorEqualTo(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {

    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result = emitBool(true);

    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one, mlir::ValueRange{result});

    //Start with both elements.


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();
    //Start with the previous value on top, we set that to true, then we continually reassign it
    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);
    auto result_arg = add_loop.getRegionIterArgs().front(); // Access the first loop argument
    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto el_result = emitEqualTo(left_val, right_val);
    result = emitAnd(result_arg, el_result);

    //This makes sure the argument gets carried to the next iteration
    builder->create<mlir::scf::YieldOp>(loc, mlir::ValueRange{result});
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return add_loop->getResult(0);
}


mlir::Value BackEnd::emitVectorNotEqualTo(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {


    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result = emitVectorEqualTo(left_side, right_side, type);
    result = emitUnaryNot(result);
    return result;
}




mlir::Value BackEnd::emitVectorGreaterThan(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitGreaterThan(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}



mlir::Value BackEnd::emitVectorGreaterEq(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitGreaterEq(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}






mlir::Value BackEnd::emitVectorLessThan(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitLessThan(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}





mlir::Value BackEnd::emitVectorLessEq(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitLessEq(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}



mlir::Value BackEnd::emitVectorMul(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays


    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitMul(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}



mlir::Value BackEnd::emitVectorDiv(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays


    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitDiv(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}




mlir::Value BackEnd::emitVectorDot(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {


    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result = emitInt(0);
    result = typeCast(result, type->getBaseType()); //Make sure the result is the appropiate type.
    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one, mlir::ValueRange{result});

    //Start with both elements.


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();
    //Start with the previous value on top, we set that to true, then we continually reassign it
    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);
    auto result_arg = add_loop.getRegionIterArgs().front(); // Access the first loop argument
    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto el_result = emitMul(left_val, right_val);
    result = emitAdd(result_arg, el_result);

    //This makes sure the argument gets carried to the next iteration
    builder->create<mlir::scf::YieldOp>(loc, mlir::ValueRange{result});
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return add_loop->getResult(0);
}



mlir::Value BackEnd::emitVectorExp(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitExpo(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}



mlir::Value BackEnd::emitVectorRem(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitRem(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}


mlir::Value BackEnd::emitVectorNot(mlir::Value value, std::shared_ptr<Type> &type) {


    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    auto arr_size = getArrSize(value);

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto result_val = loadArrVal(value, index, type);
    auto result = emitUnaryNot(result_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(value);

    return result_vector;


}


mlir::Value BackEnd::emitVectorUnaryMinus(mlir::Value value, std::shared_ptr<Type> &type) {

    auto size = getArrSize(value);
    auto scalar = emitInt(-1);
    auto scalar_vec = scalarToVector(scalar,size, type);
    auto result = emitVectorMul(value, scalar_vec, type); //The value of the result gets freed inside this op so no need to free

    return result;


}


mlir::Value BackEnd::emitVectorAnd(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitAnd(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}


mlir::Value BackEnd::emitVectorStride(mlir::Value vector_addr, mlir::Value stride, std::shared_ptr<Type> &type) {




    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    /*
    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size}); */
    auto arr_size = getArrSize(vector_addr);
    auto result_vec = createVector(arr_size, type);


    auto result_index = emitInt(0);


    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, stride, mlir::ValueRange{result_index});

    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();
    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);
    result_index = add_loop.getRegionIterArg(0);
    auto val = loadArrVal(vector_addr, index, type);
    storeArrVal(result_vec, result_index, val, type);
    result_index = emitAdd(result_index, emitInt(1));
    builder->create<mlir::scf::YieldOp>(loc, result_index);
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());


    auto final_index = add_loop->getResult(0);
    auto result = resizeVectorWithInt(result_vec, final_index, type);
    ipStack.pop();


    return result;
    //Now how should a for loop work
    //Store insertion point in the stack
    /*ipStack.push(builder->saveInsertionPoint());

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitOr(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side); */

    //While i < vec_size

    //Obtain index i*stride

    //





}



mlir::Value BackEnd::emitVectorOr(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitOr(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}


mlir::Value BackEnd::emitVectorXor(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);

    //Obtain the array with the maximum size, obtain the array with the minimum size

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    // Cannot add different sized arrays.
    //TODO: Check for error on the size of the arrays

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorSizeCheck");

    auto left_arr_size = getArrSize(left_side);
    auto right_arr_size = getArrSize(right_side);
    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{left_arr_size, right_arr_size});




    auto arr_size = left_arr_size;

    auto result_vector = createVector(arr_size, type);



    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);

    auto left_val = loadArrVal(left_side, index, type);
    auto right_val = loadArrVal(right_side, index, type);
    auto result = emitXor(left_val, right_val);
    storeArrVal(result_vector, index, result, type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();


    //Free both left and right side operands

    freeVector(left_side);

    freeVector(right_side);
    return result_vector;
}
mlir::Value BackEnd::emitVectorConcat(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type) {
    // Keep track of insertion points for nested operations
    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    // Constants for loop increments and initial indices
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(), builder->getIntegerAttr(builder->getI32Type(), 0)
    );
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(), builder->getIntegerAttr(builder->getI32Type(), 1)
    );

    // Obtain sizes of the left and right arrays
    auto arr_size_left = getArrSize(left_side);
    auto arr_size_right = getArrSize(right_side);

    // Total size of the resulting concatenated vector
    const auto arr_size = emitAdd(arr_size_left, arr_size_right);
    // Create the resultant vector
    const auto result_vector = createVector(arr_size, type);

    // First loop: copy elements from the left_side into result_vector
    {
        auto left_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size_left, const32_one
        );
        // Save the current insertion point and set the insertion point to the loop body
        ipStack.push(builder->saveInsertionPoint());
        mlir::Block *left_body = left_loop.getBody();
        builder->setInsertionPointToStart(left_body);

        const mlir::Value left_index = left_loop.getInductionVar();
        const mlir::Value left_val = loadArrVal(left_side, left_index, type);

        storeArrVal(result_vector, left_index, left_val, type);

        // Restore the insertion point
        builder->restoreInsertionPoint(ipStack.top());
        ipStack.pop();
    }

    // Second loop: copy elements from the right_side into result_vector
    {
        auto right_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size_right, const32_one
        );
        ipStack.push(builder->saveInsertionPoint());
        mlir::Block *right_body = right_loop.getBody();
        builder->setInsertionPointToStart(right_body);
        mlir::Value right_index = right_loop.getInductionVar();
        // Compute offset so that right-side elements follow left-side elements
        const auto offset_index = emitAdd(right_index, arr_size_left);
        const mlir::Value right_val = loadArrVal(right_side, right_index, type);
        storeArrVal(result_vector, offset_index, right_val, type);

        builder->restoreInsertionPoint(ipStack.top());
        ipStack.pop();
    }

    // Free both input vectors
    freeVector(left_side);
    freeVector(right_side);

    return result_vector;
}