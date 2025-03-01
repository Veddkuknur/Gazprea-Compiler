#include <CodeGenASTWalker.h>

#include "BackEnd.h"
#include "Symbol.h"
#include "Type.h"
#include "CompileTimeExceptions.h"

void BackEnd::emitCreateTuple(const std::shared_ptr<TupleType> &type, const std::shared_ptr<Symbol> &symbol, CodeGenASTWalker &walker) {
    const auto int32Type = builder->getI32Type();
    const auto floatType = builder->getF32Type();
    const auto charType = builder->getI8Type();
    const auto boolType = builder->getI1Type();
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    // Get the type of the tuple
    std::vector<mlir::Type> structVec;
    for (const auto &element_type: type->element_types) {
        if (std::dynamic_pointer_cast<PrimitiveType>(element_type) && element_type->getBaseType() == Type::BaseType::integer) {
            structVec.push_back(int32Type);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(element_type) && element_type->getBaseType() == Type::BaseType::character) {
            structVec.push_back(charType);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(element_type) && element_type->getBaseType() == Type::BaseType::real) {
            structVec.push_back(floatType);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(element_type) && element_type->getBaseType()== Type::BaseType::boolean) {
            structVec.push_back(boolType);
        } else {
            structVec.push_back(ptrTy); //This might need to be replaced with the actual type of the vector.
        }
    }

    const mlir::ArrayRef<mlir::Type> structArrayRef(structVec);

    auto structType = mlir::LLVM::LLVMStructType::getLiteral(&context, structArrayRef);
    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 0));
    auto one = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 1));
    mlir::Value structPtr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy, structType, one);

    for (size_t i = 0; i < type->element_types.size(); ++i) {
        auto constI = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, i));
        if (std::dynamic_pointer_cast<PrimitiveType>(type->element_types[i]) && type->element_types[i]->getBaseType()== Type::BaseType::real) {
            auto realAddr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                               mlir::ValueRange{zero, constI});
            builder->create<mlir::LLVM::StoreOp>(loc, emitReal(0.0f), realAddr);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(type->element_types[i]) && type->element_types[i]->getBaseType() ==
                   Type::BaseType::integer) {
            auto intAddr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                              mlir::ValueRange{zero, constI});
            builder->create<mlir::LLVM::StoreOp>(loc, emitInt(0), intAddr);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(type->element_types[i]) && type->element_types[i]->getBaseType() ==
                   Type::BaseType::boolean) {
            auto boolAddr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                               mlir::ValueRange{zero, constI});
            builder->create<mlir::LLVM::StoreOp>(loc, emitBool(true), boolAddr);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(type->element_types[i]) && type->element_types[i]->getBaseType() ==
                   Type::BaseType::character) {
            auto charAddr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                               mlir::ValueRange{zero, constI});
            builder->create<mlir::LLVM::StoreOp>(loc, emitChar('\0'), charAddr);
        } else if (auto vec_type = std::dynamic_pointer_cast<VectorType>(type->element_types[i])) {
            //Create vector type of the specified size.
            auto vec_addr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                               mlir::ValueRange{zero, constI});
            auto size = std::any_cast<mlir::Value>(vec_type->size->accept(walker));
            auto vector = createVector(size, vec_type);
            builder->create<mlir::LLVM::StoreOp>(loc, vector, vec_addr);
        }if (auto mat_type = std::dynamic_pointer_cast<MatrixType>(type->element_types[i])) {
            //Create vector type of the specified size.
            auto mat_addr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                               mlir::ValueRange{zero, constI});
            auto rowSize = std::any_cast<mlir::Value>(mat_type->rows->accept(walker));
            auto colSize = std::any_cast<mlir::Value>(mat_type->cols->accept(walker));

            auto matrix = emitZeroInitMatrix(rowSize,colSize, type->element_types[i]);

            builder->create<mlir::LLVM::StoreOp>(loc, matrix, mat_addr);
        }
    }
    symbol->value = structPtr;
}

mlir::Value BackEnd::emitCreateTuple(const std::shared_ptr<TupleType> &type,
                                     const std::vector<mlir::Value> &tupleVec) {
    // Types
    const auto int32Type = builder->getI32Type();
    const auto floatType = builder->getF32Type();
    const auto charType = builder->getI8Type();
    const auto boolType = builder->getI1Type();
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);

    // Get the type of the tuple
    std::vector<mlir::Type> structVec;
    for (const auto &element_type: type->element_types) {
        if (std::dynamic_pointer_cast<PrimitiveType>(element_type) && element_type->getBaseType() == Type::BaseType::integer) {
            structVec.push_back(int32Type);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(element_type) && element_type->getBaseType() == Type::BaseType::character) {
            structVec.push_back(charType);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(element_type) && element_type->getBaseType() == Type::BaseType::real) {
            structVec.push_back(floatType);
        } else if (std::dynamic_pointer_cast<PrimitiveType>(element_type) && element_type->getBaseType()== Type::BaseType::boolean) {
            structVec.push_back(boolType);
        } else {
            structVec.push_back(ptrTy); //This might need to be replaced with the actual type of the vector.
        }
    }

    const mlir::ArrayRef<mlir::Type> structArrayRef(structVec);

    auto structType = mlir::LLVM::LLVMStructType::getLiteral(&context, structArrayRef);
    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 0));
    auto one = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 1));
    mlir::Value structPtr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy, structType, one);

    // Set the values
    for (size_t i = 0; i < type->element_types.size(); ++i) {
        auto constI = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, i));
        auto realAddr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                           mlir::ValueRange{zero, constI});
        mlir::Value val = tupleVec[i];
        if (std::dynamic_pointer_cast<PrimitiveType>(type->element_types[i]) && type->element_types[i]->getBaseType() == Type::BaseType::real) {
            val = typeCast(tupleVec[i], Type::BaseType::real);
        } else if (std::dynamic_pointer_cast<VectorType>(type->element_types[i])) {
            if (isStructType(tupleVec[i])) {

            }


        }


        builder->create<mlir::LLVM::StoreOp>(loc, val, realAddr);
    }
    return structPtr;
}

mlir::Value BackEnd::emitCastTuple(const std::shared_ptr<TupleType> &toType, const std::shared_ptr<TupleType> &fromType,
                                   mlir::Value fromStructPtr, CodeGenASTWalker &walker) {
    const auto fromValVec = tupleValues(fromStructPtr,fromType);
    std::vector<mlir::Value> castedValVec;
    // Set the values
    castedValVec.reserve(fromValVec.size());
    for (int i =0; i < fromValVec.size(); i++) {
        if (std::dynamic_pointer_cast<PrimitiveType>(toType->element_types[i])) {
            castedValVec.push_back(typeCast(fromValVec[i],std::dynamic_pointer_cast<PrimitiveType>(toType->element_types[i])->base_type));
        } else if (std::dynamic_pointer_cast<VectorType>(toType->element_types[i])) {

            castedValVec.push_back(emitCastVector(fromValVec[i], fromType->element_types[i],
                toType->element_types[i], walker));


        }
    }
    return emitCreateTuple(toType,castedValVec);
}




mlir::Value BackEnd::emitCastVector(mlir::Value vector_addr,  std::shared_ptr<Type> &from_type,
     std::shared_ptr<Type> &to_type, CodeGenASTWalker &walker) {


    auto toType = std::dynamic_pointer_cast<VectorType>(to_type);
    if (std::dynamic_pointer_cast<PrimitiveType>(from_type)) {
        auto size = std::any_cast<mlir::Value>(toType->size->accept(walker));
        vector_addr = scalarToVector(vector_addr, size, to_type);
        return vector_addr;
    } else if (std::dynamic_pointer_cast<VectorType>(from_type)) {
        if (toType->size) {
            auto size = std::any_cast<mlir::Value>(toType->size->accept(walker));
            vector_addr = promoteVectorTo(vector_addr, from_type, to_type);
            auto dest_vec = createVector(size, toType);
            transferVecElements(vector_addr, dest_vec, to_type);
            return dest_vec;
        } else {
            vector_addr = promoteVectorTo(vector_addr, from_type, to_type);
            return vector_addr;
        }
    }

}


mlir::Value BackEnd::emitTupleAccess(const std::shared_ptr<Symbol> &symbol, int pos) {
    auto structPtr = std::any_cast<mlir::Value>(symbol->value);
    if (symbol->scope->getScopeName() == "global") {
        structPtr = emitLoadID(symbol,symbol->name);
    }
    // Types
    const auto int32Type = builder->getI32Type();
    const auto floatType = builder->getF32Type();
    const auto charType = builder->getI8Type();
    const auto boolType = builder->getI1Type();
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);

    // Get the type of the tuple
    std::vector<mlir::Type> structVec;

    auto tupleType = std::dynamic_pointer_cast<TupleType>(symbol->type);
    auto clone_vec = false;
    std::shared_ptr<Type> cloned_type;
    for (const auto &var: tupleType->element_types) {
        auto primType = std::dynamic_pointer_cast<PrimitiveType>(var);
        if (primType && primType->getBaseType() == Type::BaseType::integer) {
            structVec.push_back(int32Type);
        } else if (primType && primType->getBaseType() == Type::BaseType::character) {
            structVec.push_back(charType);
        } else if (primType && primType->getBaseType() == Type::BaseType::real) {
            structVec.push_back(floatType);
        } else if (primType && primType->getBaseType() == Type::BaseType::boolean) {
            structVec.push_back(boolType);
        } else if (std::dynamic_pointer_cast<VectorType>(var)) {
            structVec.push_back(ptrTy);
            clone_vec = true;
            cloned_type = var;
        }else if (std::dynamic_pointer_cast<MatrixType>(var)) {
            structVec.push_back(ptrTy);
        }
    }

    mlir::ArrayRef<mlir::Type> structArrayRef(structVec);
    auto structType = mlir::LLVM::LLVMStructType::getLiteral(&context, structArrayRef);

    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 0));
    auto constPos = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, pos));

    auto loadedElementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                               mlir::ValueRange{zero, constPos});
    auto loadedValue = builder->create<mlir::LLVM::LoadOp>(loc, structVec[pos], loadedElementPtr);
    if (clone_vec) {
        auto vec_val = cloneVector(loadedValue, cloned_type);
        return vec_val;
    }
    // Return the loaded value
    return loadedValue;
}


void BackEnd::emitTupAssign(const std::shared_ptr<Symbol> &symbol, const int pos, mlir::Value value) {
    const auto int32Type = builder->getI32Type();
    const auto floatType = builder->getF32Type();
    const auto charType = builder->getI8Type();
    const auto boolType = builder->getI1Type();
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);

    auto structPtr = std::any_cast<mlir::Value>(symbol->value);

    // Get the type of the tuple
    std::vector<mlir::Type> structVec;

    auto tupleType = std::dynamic_pointer_cast<TupleType>(symbol->type);

    for (const auto &var: tupleType->element_types) {
        if( auto primType = std::dynamic_pointer_cast<PrimitiveType>(var)) {
        if (primType->base_type == Type::BaseType::integer) {
            structVec.push_back(int32Type);
        } else if (primType->base_type == Type::BaseType::character) {
            structVec.push_back(charType);
        } else if (primType->base_type == Type::BaseType::real) {
            structVec.push_back(floatType);
        } else if (primType->base_type == Type::BaseType::boolean) {
            structVec.push_back(boolType);
        }
        }else {
            structVec.push_back(ptrTy);
        }
    }

    mlir::ArrayRef<mlir::Type> structArrayRef(structVec);
    auto structType = mlir::LLVM::LLVMStructType::getLiteral(&context, structArrayRef);

    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 0));
    auto constPos = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, pos));

    auto loadedElementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                               mlir::ValueRange{zero, constPos});
    builder->create<mlir::LLVM::StoreOp>(loc, value, loadedElementPtr);
}



mlir::Value BackEnd::emitTupleEqNeq(mlir::Value leftStructPtr, const std::shared_ptr<TupleType> &leftTupleType,
                                    mlir::Value rightStructPtr, const std::shared_ptr<TupleType> &rightTupleType,
                                    const std::string &op) {

    // Types
    const auto int32Type = builder->getI32Type();
    const auto floatType = builder->getF32Type();
    const auto charType = builder->getI8Type();
    const auto boolType = builder->getI1Type();
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);

    // Get the type of the tuple
    std::vector<mlir::Type> structVec;

    for (const auto &var: leftTupleType->element_types) {

        if (auto primType = std::dynamic_pointer_cast<PrimitiveType>(var)){
            if (primType->base_type == Type::BaseType::integer) {
                structVec.push_back(int32Type);
            } else if (primType->base_type == Type::BaseType::character) {
                structVec.push_back(charType);
            } else if (primType->base_type == Type::BaseType::real) {
                structVec.push_back(floatType);
            } else if (primType->base_type == Type::BaseType::boolean) {
                structVec.push_back(boolType);
            }
        }else {
            structVec.push_back(ptrTy);
        }
    }

    mlir::ArrayRef<mlir::Type> structArrayRef(structVec);
    auto leftStructType = mlir::LLVM::LLVMStructType::getLiteral(&context, structArrayRef);


    // Get the type of the tuple
    std::vector<mlir::Type> rightStructVec;

    for (const auto &var: rightTupleType->element_types) {
        if (auto primType = std::dynamic_pointer_cast<PrimitiveType>(var)){
            if (primType->base_type == Type::BaseType::integer) {
                rightStructVec.push_back(int32Type);
            } else if (primType->base_type == Type::BaseType::character) {
                rightStructVec.push_back(charType);
            } else if (primType->base_type == Type::BaseType::real) {
                rightStructVec.push_back(floatType);
            } else if (primType->base_type == Type::BaseType::boolean) {
                rightStructVec.push_back(boolType);
            }
        }else {
            rightStructVec.push_back(ptrTy);
        }
    }

    mlir::ArrayRef<mlir::Type> rightStructArrayRef(rightStructVec);
    auto rightStructType = mlir::LLVM::LLVMStructType::getLiteral(&context, rightStructArrayRef);

    // Loop over the elements store results
    std::vector<mlir::Value> results;

    if (structVec.size() != rightStructVec.size()) {
        // TODO: Change this to be handled in some walker
        throw TypeError(-1, "Unsupported comparison between tuples");
    }

    for (size_t i = 0; i < structVec.size(); i++) {
        auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 0));
        auto constPos = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type,
                                                                builder->getIntegerAttr(
                                                                    int32Type, static_cast<int>(i)));
        auto leftLoadedElementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, leftStructType, leftStructPtr,
                                                                       mlir::ValueRange{zero, constPos});
        auto leftLoadedValue = builder->create<mlir::LLVM::LoadOp>(loc, structVec[i], leftLoadedElementPtr);
        auto rightLoadedElementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, rightStructType, rightStructPtr,
                                                                        mlir::ValueRange{zero, constPos});
        auto rightLoadedValue = builder->create<mlir::LLVM::LoadOp>(loc, rightStructVec[i], rightLoadedElementPtr);
        if (op == "==") {
            if (std::dynamic_pointer_cast<VectorType>(leftTupleType->element_types[i])) {
                results.push_back(emitVectorEqualTo(leftLoadedValue, rightLoadedValue,leftTupleType->element_types[i]));
            }else {
                results.push_back(emitEqualTo(leftLoadedValue, rightLoadedValue));
            }
        } else {
            if (std::dynamic_pointer_cast<VectorType>(leftTupleType->element_types[i])) {
                results.push_back(emitVectorNotEqualTo(leftLoadedValue, rightLoadedValue,leftTupleType->element_types[i]));
            }else {
                results.push_back(emitNotEqual(leftLoadedValue, rightLoadedValue));
            }
        }
    }

    auto result = results[0];
    for (size_t i = 1; i < results.size(); ++i) {
        if (op == "==") {
            result = builder->create<mlir::LLVM::AndOp>(loc, result, results[i]);
        } else {
            result = builder->create<mlir::LLVM::OrOp>(loc, result, results[i]);
        }
    }

    return result;
}

std::vector<mlir::Value> BackEnd::tupleValues(const std::shared_ptr<Symbol> &symbol) {
    auto structPtr = std::any_cast<mlir::Value>(symbol->value);
    if (symbol->scope->getScopeName() == "global") {
        structPtr = emitLoadID(symbol,symbol->name);
    }
    // Types
    const auto int32Type = builder->getI32Type();
    const auto floatType = builder->getF32Type();
    const auto charType = builder->getI8Type();
    const auto boolType = builder->getI1Type();
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    // Get the type of the tuple
    std::vector<mlir::Type> structVec;
    std::vector<mlir::Value> tupleValues;
    auto tupleType = std::dynamic_pointer_cast<TupleType>(symbol->type);
    for (const auto &var: tupleType->element_types) {
        if(auto primType = std::dynamic_pointer_cast<PrimitiveType>(var)) {
            if (primType->base_type == Type::BaseType::integer) {
                structVec.push_back(int32Type);
            } else if (primType->base_type == Type::BaseType::character) {
                structVec.push_back(charType);
            } else if (primType->base_type == Type::BaseType::real) {
                structVec.push_back(floatType);
            } else if (primType->base_type == Type::BaseType::boolean) {
                structVec.push_back(boolType);
            }
        }else{
            structVec.push_back(ptrTy);
        }
    }
    mlir::ArrayRef<mlir::Type> structArrayRef(structVec);
    auto structType = mlir::LLVM::LLVMStructType::getLiteral(&context, structArrayRef);
    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 0));
    for (size_t i = 0; i < structVec.size(); ++i) {
        auto constPos = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, i));
        auto loadedElementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                                   mlir::ValueRange{zero, constPos});
        auto loaded_val = builder->create<mlir::LLVM::LoadOp>(loc, structVec[i], loadedElementPtr);


        tupleValues.push_back(loaded_val);
    }
    // Return the loaded value
    return tupleValues;
}

std::vector<mlir::Value> BackEnd::tupleValues(mlir::Value structPtr, const std::shared_ptr<TupleType> &tupleType) {
    // Types
    const auto int32Type = builder->getI32Type();
    const auto floatType = builder->getF32Type();
    const auto charType = builder->getI8Type();
    const auto boolType = builder->getI1Type();
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);

    // Get the type of the tuple
    std::vector<mlir::Type> structVec;
    std::vector<mlir::Value> tupleValues;
    for (const auto &var: tupleType->element_types) {
        if(auto primType = std::dynamic_pointer_cast<PrimitiveType>(var)) {
            if (primType->base_type == Type::BaseType::integer) {
                structVec.push_back(int32Type);
            } else if (primType->base_type == Type::BaseType::character) {
                structVec.push_back(charType);
            } else if (primType->base_type == Type::BaseType::real) {
                structVec.push_back(floatType);
            } else if (primType->base_type == Type::BaseType::boolean) {
                structVec.push_back(boolType);
            }
        }else{
            structVec.push_back(ptrTy);
        }
    }
    mlir::ArrayRef<mlir::Type> structArrayRef(structVec);
    auto structType = mlir::LLVM::LLVMStructType::getLiteral(&context, structArrayRef);
    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 0));
    for (size_t i = 0; i < structVec.size(); ++i) {
        auto constPos = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, i));
        auto loadedElementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                                   mlir::ValueRange{zero, constPos});
        tupleValues.push_back(builder->create<mlir::LLVM::LoadOp>(loc, structVec[i], loadedElementPtr));
    }
    // Return the loaded value
    return tupleValues;
}


mlir::Value BackEnd::getTupleElemPointer(const std::shared_ptr<Symbol> &symbol, int pos) {

    auto structPtr = std::any_cast<mlir::Value>(symbol->value);
    if (symbol->scope->getScopeName() == "global") {
        structPtr = emitLoadID(symbol,symbol->name);
    }
    // Types
    const auto int32Type = builder->getI32Type();
    const auto floatType = builder->getF32Type();
    const auto charType = builder->getI8Type();
    const auto boolType = builder->getI1Type();
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);

    // Get the type of the tuple
    std::vector<mlir::Type> structVec;

    auto tupleType = std::dynamic_pointer_cast<TupleType>(symbol->type);
    std::shared_ptr<Type> cloned_type;
    for (const auto &var: tupleType->element_types) {
        auto primType = std::dynamic_pointer_cast<PrimitiveType>(var);
        if (primType && primType->getBaseType() == Type::BaseType::integer) {
            structVec.push_back(int32Type);
        } else if (primType && primType->getBaseType() == Type::BaseType::character) {
            structVec.push_back(charType);
        } else if (primType && primType->getBaseType() == Type::BaseType::real) {
            structVec.push_back(floatType);
        } else if (primType && primType->getBaseType() == Type::BaseType::boolean) {
            structVec.push_back(boolType);
        } else if (std::dynamic_pointer_cast<VectorType>(var)) {
            structVec.push_back(ptrTy);
        }
    }
    mlir::ArrayRef<mlir::Type> structArrayRef(structVec);
    auto structType = mlir::LLVM::LLVMStructType::getLiteral(&context, structArrayRef);

    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, 0));
    auto constPos = builder->create<mlir::LLVM::ConstantOp>(loc, int32Type, builder->getIntegerAttr(int32Type, pos));

    auto loadedElementPtr = builder->create<mlir::LLVM::GEPOp>(loc, ptrTy, structType, structPtr,
                                                               mlir::ValueRange{zero, constPos});
    // Return the pointer to the value
    return loadedElementPtr;
}
