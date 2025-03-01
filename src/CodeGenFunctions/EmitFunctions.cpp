#include <CompileTimeExceptions.h>
#include <iostream>
#include <mlir/Dialect/LLVMIR/FunctionCallUtils.h>

#include "Symbol.h"
#include "ScopedSymbol.h"
#include "BackEnd.h"



mlir::Value BackEnd::emitVectorDec(mlir::Value vector) {
    mlir::Type int32_type = builder->getI32Type();
    mlir::Value const32_four = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 4);


    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);

    mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, ptr_type, const32_four);
    // allocate and store on stack
    builder->create<mlir::LLVM::StoreOp>(loc, vector, alloca_op);

    return alloca_op;
}



void BackEnd::checkDeclSizeError(mlir::Value decl_size, mlir::Value vec_size) {

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("declVectorSizeCheck");

    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{decl_size, vec_size});


}


void BackEnd::emitDec(const std::shared_ptr<Symbol>& symbol) {
    mlir::Type ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    // declaring an integer

    if (std::dynamic_pointer_cast<VectorType>(symbol->type)) {
        auto vector_struct = std::any_cast<mlir::Value>(symbol->value);
        auto id_val = emitVectorDec(vector_struct);
        symbol->value = id_val;
    } else if (const auto typeCheck = std::dynamic_pointer_cast<PrimitiveType>(symbol->type); typeCheck->base_type == Type::BaseType::integer) {
        mlir::Type int32_type = builder->getI32Type();
        mlir::Value constI32_zero = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(), 0);
        mlir::Value constI32_four = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(), 4);
        // allocated space for id value (int32)
        mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, int32_type, constI32_four);
        // allocate and store on stack
        builder->create<mlir::LLVM::StoreOp>(loc, constI32_zero, alloca_op);
        // store the mlir value in the scope tree
        symbol->value = alloca_op;
    } else if (typeCheck->base_type == Type::BaseType::real) {
        mlir::Type float32_type = builder->getF32Type();
        mlir::Value constI32_four = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(), 4);
        auto floatVal = mlir::FloatAttr::get(builder->getF32Type(), 0.0f);
        auto constF32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, float32_type, floatVal);
        // allocated space for id value (float32)
        mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, float32_type, constI32_four);
        // allocate and store on stack
        builder->create<mlir::LLVM::StoreOp>(loc, constF32_zero, alloca_op);
        // store the mlir value in the scope tree
        symbol->value = alloca_op;
    } else if (typeCheck->base_type == Type::BaseType::boolean) {
        mlir::Type bool_type = builder->getI1Type();
        mlir::Value constI32_one = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(), 1);
        mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, bool_type, constI32_one);
        mlir::Value constBool_false = builder->create<mlir::LLVM::ConstantOp>(loc, bool_type, builder->getBoolAttr(false));
        builder->create<mlir::LLVM::StoreOp>(loc, constBool_false, alloca_op);
        // store the mlir value in the scope tree
        symbol->value = alloca_op;
    }else if (typeCheck->base_type == Type::BaseType::character) {
        mlir::Type char_type = builder->getI8Type();
        mlir::Value constI32_one = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(), 1);
        mlir::Value constI8_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI8Type(),  static_cast<int8_t>(0));
        // allocated space for character value (int8)
        mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, char_type, constI32_one);
        // allocate and store on stack
        builder->create<mlir::LLVM::StoreOp>(loc,constI8_zero, alloca_op);
        // store the mlir value in the scope tree
        symbol->value = alloca_op;
    }

}
void BackEnd::emitDec(const std::shared_ptr<Symbol>& symbol, mlir::Value val) {
    mlir::Type ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    mlir::Value constI32_four = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 4);
    if (std::dynamic_pointer_cast<VectorType>(symbol->type)) {
        auto vec_alloca_op = emitVectorDec(val);
        symbol->value = vec_alloca_op;

    } else if (std::dynamic_pointer_cast<MatrixType>(symbol->type)){
      symbol->value = val;
    } else if (isStructType(val)){
      symbol->value = val;
    } else if (const auto typeCheck = std::dynamic_pointer_cast<PrimitiveType>(symbol->type); typeCheck->base_type == Type::BaseType::integer) {
        mlir::Type int32_type = builder->getI32Type();
        // allocated space for id value (int32)
        mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, int32_type, constI32_four);
        // allocate and store on stack
        builder->create<mlir::LLVM::StoreOp>(loc, val, alloca_op);
        // store the mlir value in the scope tree
        symbol->value = alloca_op;
    }else if (typeCheck->base_type == Type::BaseType::real) {
        mlir::Type float32_type = builder->getF32Type();
        if (isIntegerType(val)) {
            val = castInt(val,Type::BaseType::real);
        }
        // allocated space for id value (float32)
        mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, float32_type, constI32_four);
        // allocate and store on stack
        builder->create<mlir::LLVM::StoreOp>(loc, val, alloca_op);
        // store the mlir value in the scope tree
        symbol->value = alloca_op;
    } else if (typeCheck->base_type == Type::BaseType::boolean) {
        mlir::Value constI32_one = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 1);
        mlir::Type bool_type = builder->getI1Type();
        mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, bool_type, constI32_one);
        builder->create<mlir::LLVM::StoreOp>(loc, val, alloca_op);
        // store the mlir value in the scope tree
        symbol->value = alloca_op;
    }else if (typeCheck->base_type == Type::BaseType::character) {
        mlir::Value constI32_one = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 1);
        mlir::Type char_type = builder->getI8Type();
        // allocated space for character value (int8)
        mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, char_type, constI32_one);
        // allocate and store on stack
        builder->create<mlir::LLVM::StoreOp>(loc, val, alloca_op);
        // store the mlir value in the scope tree
        symbol->value = alloca_op;
    }
}


void BackEnd::emitDecGlobal(const std::shared_ptr<Symbol> &symbol, mlir::Value global_addr, mlir::Value val) {
    builder->create<mlir::LLVM::StoreOp>(loc, val, global_addr);
}


void BackEnd::insertGlobalDeclBlock() {
    auto main_proc = module.lookupSymbol<mlir::func::FuncOp>("main");
    auto& main_proc_body = main_proc.getBody().front();
    builder->setInsertionPointToStart(&main_proc_body);
    while (!global_decl_block->empty()) {
        mlir::Operation &op = global_decl_block->front();
        op.remove();
        main_proc_body.getOperations().insert(getInsertionPoint(), &op);
    }
}


void BackEnd::insertGlobalFreeBlock() {
    auto main_proc = module.lookupSymbol<mlir::func::FuncOp>("main");
    auto& main_proc_body = main_proc.getBody().back();

}


void BackEnd::setInsertionPointToModuleEnd() {
    builder->setInsertionPointToEnd(module.getBody());
}






void BackEnd::transferVecElements(mlir::Value from_vector, mlir::Value to_vector, std::shared_ptr<Type> &to_type) {


    //Load the element from the vector variable//Load the vector
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);


    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);
    //Only store as many elements as there are from t he from vector
    auto from_size = getArrSize(from_vector);
    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, from_size, const32_one);

    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();
    //Store insertion point in the stack
    auto old_insert_point = builder->saveInsertionPoint();
    builder->setInsertionPointToStart(for_body);
    auto from_val = loadArrVal(from_vector, index, to_type);
    storeArrVal(to_vector, index, from_val, to_type);
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(old_insert_point);
    //Free the vector...
    freeVector(from_vector);
}

void BackEnd::emitAssignVector( std::shared_ptr<Symbol>& symbol, mlir::Value value) {


    //Load the element from the vector variable
    auto to_alloca = std::any_cast<mlir::Value>(symbol->value);
    //Load the vector
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);

    auto to_vec = builder->create<mlir::LLVM::LoadOp>(loc, ptr_type, to_alloca);
    auto from_vec = value;

    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);



    auto to_size = getArrSize(to_vec);
    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, to_size, const32_one);

    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();
    //Store insertion point in the stack
    auto old_insert_point = builder->saveInsertionPoint();
    builder->setInsertionPointToStart(for_body);
    auto from_val = loadArrVal(from_vec, index, symbol->type);
    storeArrVal(to_vec, index, from_val, symbol->type);
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(old_insert_point);
    //Free the vector...
    freeVector(value);


    //Obtain the size of the vector



}






void BackEnd::emitAssignId( std::shared_ptr<Symbol>& symbol, mlir::Value value, int line)  {
    if (std::dynamic_pointer_cast<PrimitiveType>(symbol->type)) {
        if (isIntegerType(value) && symbol->type->getBaseType() == Type::BaseType::real) {
            value = castInt(std::any_cast<mlir::Value>(value),Type::BaseType::real);
        }
        if (!isSameType(value,symbol->type)) {
            throw TypeError(line,"incompatible expression type");
        }
        auto alloca_op = std::any_cast<mlir::Value>(symbol->value); // pointer to variable on stack
        builder->create<mlir::LLVM::StoreOp>(loc, value, alloca_op);
    } else if (std::dynamic_pointer_cast<VectorType>(symbol->type)) {
        //We are safe to say that the result of a vector will be a vector, so we are safe to know that the left and right side operand are vectors

        //First we compare the size of each operand, then we restart assigning, we call size error if the size of right operand
        //Copy every value from vector A to vector B - This is a general solution.
        // is larger than the left operand.
        //If we reassign the vector to another vector then we must copy all the contents of the left side vector to the right side vector
        emitAssignVector(symbol, value);

    }
    else {
        symbol->value = value;
    }
}


mlir::Value BackEnd::emitInt(const int val) const {
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), val);
}


mlir::Value BackEnd::emitReal(const float val) const {
    auto floatVal = mlir::FloatAttr::get(builder->getF32Type(), val);
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getF32Type(), floatVal);
}


mlir::Value BackEnd::emitBool(const bool val) const {
    auto boolValue = builder->getBoolAttr(val);
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI1Type(), boolValue);
}


mlir::Value BackEnd::emitChar(const char val) const {
    auto asciiValue = static_cast<int8_t>(val);
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI8Type(), asciiValue);
}


mlir::Value BackEnd::emitIterLoopIndex() {

    mlir::Type int32_type = builder->getI32Type();
    mlir::Type ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    auto zero = emitInt(0);
    auto four = emitInt(4);
    mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, int32_type, four);
    builder->create<mlir::LLVM::StoreOp>(loc, zero, alloca_op);
    return alloca_op;
}


mlir::Value BackEnd::emitDomainVariable(mlir::Type type) {
    auto four = emitInt(4);
    mlir::Type ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, type, four);
    return alloca_op;
}


void BackEnd::emitStoreDomainVariable(mlir::Value dv_pointer, mlir::Value new_value) {
    builder->create<mlir::LLVM::StoreOp>(loc, new_value, dv_pointer);
}


mlir::Value BackEnd::emitIncrementIndex(mlir::Value index) {
    auto one = emitInt(1);
    return emitAdd(index, one);
}


mlir::Value BackEnd::emitLoadIndex(mlir::Value index_ptr) {
    mlir::Type int32_type = builder->getI32Type();
    return builder->create<mlir::LLVM::LoadOp>(loc, int32_type, index_ptr);
}


void BackEnd::emitStoreIndex(mlir::Value index_ptr, mlir::Value new_value) {
    builder->create<mlir::LLVM::StoreOp>(loc, new_value, index_ptr);
}


void BackEnd::emitUnreachableOp() {
    builder->create<mlir::LLVM::UnreachableOp>(loc);
}


mlir::Value BackEnd::emitLoadID(const std::shared_ptr<Symbol>& symbol) {
    auto ptr = std::any_cast<mlir::Value>(symbol->value);
    if (const auto typeCheck = std::dynamic_pointer_cast<PrimitiveType>(symbol->type)) {
        if(typeCheck->base_type == Type::BaseType::integer) {
        mlir::Type int32_type = builder->getI32Type();
        return builder->create<mlir::LLVM::LoadOp>(loc, int32_type, ptr);
        } else if (typeCheck->base_type == Type::BaseType::real) {
            mlir::Type float32_type = builder->getF32Type();
            return builder->create<mlir::LLVM::LoadOp>(loc, float32_type, ptr);
        } else if (typeCheck->base_type == Type::BaseType::boolean) {
            mlir::Type bool_type = builder->getI1Type();
            return builder->create<mlir::LLVM::LoadOp>(loc, bool_type, ptr);
        } else if (typeCheck->base_type == Type::BaseType::character) {
            mlir::Type char_type = builder->getI8Type();
            return  builder->create<mlir::LLVM::LoadOp>(loc, char_type, ptr);
        }
    } else if (std::dynamic_pointer_cast<VectorType>(symbol->type)) {
        return loadVectorId(symbol);
    } else if (std::dynamic_pointer_cast<MatrixType>(symbol->type)){
      return loadMatrixId(symbol);
    }
    return  ptr;
}


mlir::Value BackEnd::loadVectorId(const std::shared_ptr<Symbol>& symbol, const std::string& id) {
    const std::string globalName = "g_" + id;

    //Create a function to clone your vector, here we pass it the
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    auto addressOfOp = builder->create<mlir::LLVM::AddressOfOp>(loc, ptrTy, globalName);
    //Load vector struct
    auto original_vec = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy, addressOfOp)->getResult(0);
    auto cloned_vec = cloneVector(original_vec, symbol->type);
    return cloned_vec;
}



mlir::Value BackEnd::loadVectorId(const std::shared_ptr<Symbol>& symbol) {

    //Create a function to clone your vector, here we pass it the
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    //Load vector struct
    auto val = std::any_cast<mlir::Value>(symbol->value);
    auto original_vec = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy, val)->getResult(0);
    auto cloned_vec = cloneVector(original_vec, symbol->type);
    return cloned_vec;
}





mlir::Value BackEnd::cloneVector(mlir::Value vector_addr, std::shared_ptr<Type>& type) {


    auto from_vec = vector_addr;

    auto size = getArrSize(from_vec);

    auto to_vec = createVector(size, type);
    auto const32_zero =  builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_one =  builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);


    auto add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, size, const32_one);

    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();
    //Store insertion point in the stack
    auto old_insert_point = builder->saveInsertionPoint();
    builder->setInsertionPointToStart(for_body);
    auto from_val = loadArrVal(from_vec, index, type);
    storeArrVal(to_vec, index, from_val, type);
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(old_insert_point);


    //Now create for loop that copies the vector
    return to_vec;

}





mlir::Value BackEnd::emitLoadID(const std::shared_ptr<Symbol>& symbol, const std::string& id) {
    const std::string globalName = "g_" + id;
    if (const auto typeCheck = std::dynamic_pointer_cast<PrimitiveType>(symbol->type)){
        if(typeCheck->base_type == Type::BaseType::integer) {
            mlir::IntegerType intType = builder->getI32Type();
            auto pointerType = mlir::LLVM::LLVMPointerType::get(intType.getContext());
            auto addressOfOp = builder->create<mlir::LLVM::AddressOfOp>(loc, pointerType, globalName);
            auto loadedValue = builder->create<mlir::LLVM::LoadOp>(loc, intType, addressOfOp);
            return loadedValue;
        }else if (typeCheck->base_type == Type::BaseType::real) {
            mlir::Type intType = builder->getF32Type();
            auto pointerType = mlir::LLVM::LLVMPointerType::get(intType.getContext());
            auto addressOfOp = builder->create<mlir::LLVM::AddressOfOp>(loc, pointerType, globalName);
            auto loadedValue = builder->create<mlir::LLVM::LoadOp>(loc, intType, addressOfOp);
            return loadedValue;
        }else if (typeCheck->base_type == Type::BaseType::boolean) {
            mlir::IntegerType intType = builder->getI1Type();
            auto pointerType = mlir::LLVM::LLVMPointerType::get(intType.getContext());
            auto addressOfOp = builder->create<mlir::LLVM::AddressOfOp>(loc, pointerType, globalName);
            auto loadedValue = builder->create<mlir::LLVM::LoadOp>(loc, intType, addressOfOp);
            return loadedValue;
        }
        else if (typeCheck->base_type == Type::BaseType::character) {
            mlir::IntegerType intType = builder->getI8Type();
            auto pointerType = mlir::LLVM::LLVMPointerType::get(intType.getContext());
            auto addressOfOp = builder->create<mlir::LLVM::AddressOfOp>(loc, pointerType, globalName);
            auto loadedValue = builder->create<mlir::LLVM::LoadOp>(loc, intType, addressOfOp);
            return loadedValue;
        }
    } else if (std::dynamic_pointer_cast<VectorType>(symbol->type)) {
        //Returned a cloned version of the vector.
        return loadVectorId(symbol, id);
    }

    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    auto addressOfOp = builder->create<mlir::LLVM::AddressOfOp>(loc, ptrTy, globalName);
    return  builder->create<mlir::LLVM::LoadOp>(loc, ptrTy, addressOfOp);


}


void BackEnd::emitFuncDefStart(const std::shared_ptr<Symbol>& symbol, bool is_prototype) {

    auto func_symbol = std::dynamic_pointer_cast<FunctionSymbol>(symbol);
    std::vector<mlir::Type> param_types_vector;

    // we have to create a vector of MLIR Types to pass to the funcOp
    for (const auto& param : func_symbol->orderedArgs)
        param_types_vector.push_back(getMLIRTypeFromTypeObject(param->type));

    // create the funcOp (with or without return value)
    mlir::FunctionType func_type;
    if (func_symbol->type)
        func_type = builder->getFunctionType(param_types_vector, {getMLIRTypeFromTypeObject(func_symbol->type)});
    else
        func_type = builder->getFunctionType(param_types_vector, {});

    std::string func_name = func_symbol->name;
    if (func_name != "main")
        func_name = '_' + func_name;

    // check if the function has an existing forward declaration
    auto function = module.lookupSymbol<mlir::func::FuncOp>(func_name);
    if (!function) {
        // no forward declaration exists, create the function
        function = builder->create<mlir::func::FuncOp>(loc, func_name, func_type);
        if (is_prototype)
            return; // the function is a prototype, leave the declaration for later
    }
    // set the entry block
    auto func_block = function.addEntryBlock();
    builder->setInsertionPointToStart(func_block);

    // emit the parameters in the main function block
    for (int i = 0; i < int(func_block->getNumArguments()); i++) {
        mlir::Value param = func_block->getArgument(i);
        mlir::Value allocOp = builder->create<mlir::LLVM::AllocaOp>(
            loc, mlir::LLVM::LLVMPointerType::get(&context), param.getType(), builder->create<mlir::LLVM::ConstantOp>(
                loc, mlir::IntegerType::get(&context, 32),
                builder->getI32IntegerAttr(4)));

        builder->create<mlir::LLVM::StoreOp>(loc, param, allocOp);
        func_symbol->orderedArgs[i]->value = allocOp;
    }
}


// duplicate code :( can't be bothered to handle two different symbol types in the same method
void BackEnd::emitProcDefStart(const std::shared_ptr<Symbol>& symbol, bool is_prototype) {

    auto proc_symbol = std::dynamic_pointer_cast<ProcedureSymbol>(symbol);
    std::vector<mlir::Type> param_types_vector;

    // we have to create a vector of MLIR Types to pass to the funcOp
    for (const auto& param : proc_symbol->orderedArgs) {

        // pass by reference
        if (param->mutability == true)
            param_types_vector.push_back(mlir::LLVM::LLVMPointerType::get(&context));

        // pass by value
        else
            param_types_vector.push_back(getMLIRTypeFromTypeObject(param->type));
    }
    // create the funcOp (with or without return value)
    mlir::FunctionType proc_type;
    if (proc_symbol->type)
        proc_type = builder->getFunctionType(param_types_vector, {getMLIRTypeFromTypeObject(proc_symbol->type)});
    else
        proc_type = builder->getFunctionType(param_types_vector, {});

    std::string proc_name = proc_symbol->name;
    if (proc_name != "main")
        proc_name = '_' + proc_name;

    // check if the procedure has an existing forward declaration
    auto procedure = module.lookupSymbol<mlir::func::FuncOp>(proc_name);
    if (!procedure) {
        // no forward declaration exists, create the procedure
        procedure = builder->create<mlir::func::FuncOp>(loc, proc_name, proc_type);
        if (is_prototype)
            return; // the procedure is a prototype, leave the declaration for later
    }
    // set the entry block
    auto proc_block = procedure.addEntryBlock();
    builder->setInsertionPointToStart(proc_block);

    // if the proc is main, init the buffer pointer
    if (proc_name == "main"){
      initInStreamBufferInMain();
    }

    // emit the parameters in the main procedure block
    for (int i = 0; i < int(proc_block->getNumArguments()); i++) {
        mlir::Value param = proc_block->getArgument(i);
        auto param_type = param.getType();

        // param is not a pointer
        if (not param_type.isa<mlir::LLVM::LLVMPointerType>()) {
            mlir::Value allocOp = builder->create<mlir::LLVM::AllocaOp>(
                loc, mlir::LLVM::LLVMPointerType::get(&context), param.getType(),
                builder->create<mlir::LLVM::ConstantOp>( loc, mlir::IntegerType::get(&context, 32),
                    builder->getI32IntegerAttr(4)));
            builder->create<mlir::LLVM::StoreOp>(loc, param, allocOp);
            proc_symbol->orderedArgs[i]->value = allocOp;
        }

        // param is a pointer
        else
            proc_symbol->orderedArgs[i]->value = param;
    }
}


void BackEnd::emitFuncDefEnd() {
    builder->setInsertionPointToEnd(module.getBody());
}


void BackEnd::emitProcDefEnd() {
    builder->setInsertionPointToEnd(module.getBody());
}


mlir::func::ReturnOp BackEnd::emitReturn(mlir::Value return_val) {

    mlir::func::ReturnOp return_op;
    if (return_val) { // return something
        return builder->create<mlir::func::ReturnOp>(loc, return_val);
    }
    else { // return nothing
        return builder->create<mlir::func::ReturnOp>(loc);
    }
}


mlir::Value BackEnd::emitFuncProcCall(std::shared_ptr<Symbol> &symbol, std::vector<mlir::Value> &evaluated_args) {

    // function case
    if (auto func_sym = std::dynamic_pointer_cast<FunctionSymbol>(symbol)) {

        std::string func_name = func_sym->name;
        if (func_name != "main")
            func_name = '_' + func_name;
        mlir::Type return_type = getMLIRTypeFromTypeObject(func_sym->type);
        std::vector<mlir::Value> args;
        for (int i = 0; i < static_cast<int>(func_sym->orderedArgs.size()); i++) {
            mlir::Value tempVal = evaluated_args[i];
            if (auto prim_type = std::dynamic_pointer_cast<PrimitiveType>(func_sym->orderedArgs[i]->type)) {
                if (prim_type->base_type == Type::BaseType::real)
                    tempVal = typeCast(evaluated_args[i],Type::BaseType::real);
            }
            args.push_back(tempVal);
        }
        const auto call_op = builder->create<mlir::func::CallOp>(loc, func_name, return_type, args);
        return call_op->getResult(0);
    }
    // procedure case
    if (auto proc_sym = std::dynamic_pointer_cast<ProcedureSymbol>(symbol)){
        mlir::Type return_type = getMLIRTypeFromTypeObject(proc_sym->type);

        std::string proc_name = proc_sym->name;
        if (proc_name != "main")
            proc_name = '_' + proc_name;
        std::vector<mlir::Value> args;
        for (int i = 0; i < static_cast<int>(proc_sym->orderedArgs.size()); i++) {
            mlir::Value tempVal = evaluated_args[i];
            if (auto prim_type = std::dynamic_pointer_cast<PrimitiveType>(proc_sym->orderedArgs[i]->type)) {
                if (prim_type->base_type == Type::BaseType::real)
                    tempVal = typeCast(evaluated_args[i],Type::BaseType::real);
            }
            args.push_back(tempVal);
        }
        if (!return_type) {
            builder->create<mlir::func::CallOp>(loc, proc_name, llvm::ArrayRef<mlir::Type>{}, args);
            return {};
        }
        auto call_op = builder->create<mlir::func::CallOp>(loc, proc_name, return_type, args);
        if (proc_sym->type)
            return call_op->getResult(0);
    }
    return {};
}


mlir::OpBuilder::InsertPoint BackEnd::saveInsertionPoint() {
    return builder->saveInsertionPoint();
}


void BackEnd::restoreInsertionPoint(mlir::OpBuilder::InsertPoint insert_point) {
    builder->restoreInsertionPoint(insert_point);
}


mlir::Block::iterator BackEnd::getInsertionPoint(){
    return builder->getInsertionPoint();
}


void BackEnd::setInsertionPointToStart(mlir::Block *block) {
    builder->setInsertionPointToStart(block);
}


void BackEnd::setInsertionPointToEnd(mlir::Block *block) {
    builder->setInsertionPointToEnd(block);
}

void BackEnd::setInsertionPointBeforeOp(mlir::Operation* op) {
    builder->setInsertionPoint(op);
}


mlir::Block * BackEnd::getInsertionBlock() {
    return builder->getInsertionBlock();
}


bool BackEnd::hasTerminator(mlir::Block *block) {

    // does this block have a terminal op or guaranteed return conditional?
    if (block->empty()) {
        return false;
    }
    return block->back().mightHaveTrait<mlir::OpTrait::IsTerminator>();
}

bool BackEnd::isTerminator(mlir::Operation* op) {
    return op->mightHaveTrait<mlir::OpTrait::IsTerminator>();
}


void BackEnd::emitCondBranch(mlir::Value condition, mlir::Block *if_block, mlir::Block *else_block) {

    //Create 3 different branches.

    //The condition is created, now we must create an insertion point before the header point.
    builder->create<mlir::LLVM::CondBrOp>(loc, condition, if_block, else_block);
}


void BackEnd::emitBranch(mlir::Block * block) {
    builder->create<mlir::LLVM::BrOp>(loc, block);
}


mlir::Block * BackEnd::createBlock(){
    return builder->createBlock(builder->getInsertionBlock()->getParent());
}


mlir::Value BackEnd::declareGlobal(const std::shared_ptr<Symbol> &symbol, std::string& id) {
    if (const auto typeCheck = std::dynamic_pointer_cast<PrimitiveType>(symbol->type)) {
        if (typeCheck->base_type == Type::BaseType::integer) {
            auto constI32_zero = builder->create<mlir::LLVM::ConstantOp>(
            loc, builder->getI32Type(), 0);
            return createGlobalInteger(id, builder->getI32Type(), constI32_zero);
        } else if (typeCheck->base_type == Type::BaseType::real) {
            return createGlobalFloat(id);
        } else if (typeCheck->base_type == Type::BaseType::boolean) {
            auto constBool_false = builder->create<mlir::LLVM::ConstantOp>(
                loc, builder->getI1Type(), builder->getBoolAttr(false));
            return createGlobalInteger(id, builder->getI1Type(), constBool_false);
        }else if (typeCheck->base_type == Type::BaseType::character) {
            auto constI8_zero = builder->create<mlir::LLVM::ConstantOp>(
                loc, builder->getI8Type(), static_cast<int8_t>(0));
            return createGlobalInteger(id, builder->getI1Type(), constI8_zero);
        }
    }if (const auto typeCheck = std::dynamic_pointer_cast<TupleType>(symbol->type)) {
        return createGlobalptr(id);
    } else if (auto vec_type = std::dynamic_pointer_cast<VectorType>(symbol->type)) {
        return createGlobalptr(id);
    } else if (auto mat_type = std::dynamic_pointer_cast<MatrixType>(symbol->type)) {
        return createGlobalptr(id);
    }
    return {};
}


mlir::Value BackEnd::emitCurrentStackPointer() {
    auto stack_save_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("llvm.stacksave.p0");
    auto saved_stack = builder->create<mlir::LLVM::CallOp>(
    module.getLoc(), stack_save_func, mlir::ValueRange());
    return saved_stack.getResult();
}


void BackEnd::emitRestoreStackPointer(mlir::Value stack_pointer) {
    auto restored_stack_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("llvm.stackrestore.p0");
    builder->create<mlir::LLVM::CallOp>(module.getLoc(), restored_stack_func, stack_pointer);
}


mlir::Value BackEnd::emitAndStoreInteger(mlir::Value integer) {
    mlir::Type int32_type = builder->getI32Type();
    mlir::Type ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    auto four = emitInt(4);
    mlir::Value alloca_op = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, int32_type, four);
    builder->create<mlir::LLVM::StoreOp>(loc, integer, alloca_op);
    return alloca_op;
}


mlir::Value BackEnd::emitLoadInteger(mlir::Value integer_ptr) {
    mlir::Type int32_type = builder->getI32Type();
    return builder->create<mlir::LLVM::LoadOp>(loc, int32_type, integer_ptr);
}

void BackEnd::storeIntegerAtPointer(mlir::Value integer_val, mlir::Value integer_ptr) {
    builder->create<mlir::LLVM::StoreOp>(loc, integer_val, integer_ptr);
}
mlir::Value BackEnd::emitFormatScalar(mlir::Value value) {
    PrimitiveType primitive;
    primitive.base_type = Type::BaseType::character;
    auto charType = std::dynamic_pointer_cast<Type>(std::make_shared<PrimitiveType>(primitive));
    auto vec_addr = createVector(emitInt(20),charType);
    auto i64type = builder->getI64Type();
    auto i32type = builder->getI32Type();
    auto i8type = builder->getI8Type();
    auto int32_type = builder->getI32Type();

    if (isCharType(value)) {
        storeArrVal(vec_addr,emitInt(0),value,charType);
        return vec_addr;
    } if (isBoolType(value)) {
        auto boolStr = builder->create<mlir::LLVM::SelectOp>(loc, value, emitChar('T'), emitChar('F'));
        storeArrVal(vec_addr,emitInt(0),boolStr,charType);
        return vec_addr;
    } if (isIntegerType(value)){
        auto c11 = builder->create<mlir::LLVM::ConstantOp>(loc, i64type, 11);
        auto c0i32 = emitInt(0);
        auto c11i32 = emitInt(11);
        auto c1i32 = emitInt(1);
        auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
        mlir::LLVM::LLVMFuncOp mallocFn = mlir::LLVM::lookupOrCreateMallocFn(module, i64type);
        const mlir::Value bufferPtr = builder->create<mlir::LLVM::CallOp>(
            loc, mallocFn, mlir::ValueRange{c11}).getResult();
        auto formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("intFormat");
        const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
        mlir::ValueRange args = {bufferPtr,formatStringPtr,value};
        auto sprintfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("sprintf");
        builder->create<mlir::LLVM::CallOp>(loc, sprintfFunc, args);
        auto strlen = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("strlen");
        mlir::ValueRange argsStrLen = {bufferPtr};
        auto vecSize = builder->create<mlir::LLVM::CallOp>(loc, strlen,argsStrLen).getResult();
        vec_addr = createVector(vecSize,charType);
        std::stack<mlir::OpBuilder::InsertPoint> ipStack;
        auto colsFor = builder->create<mlir::scf::ForOp>(
             loc, c0i32, vecSize, c1i32);
        mlir::Value index = colsFor.getInductionVar();
        mlir::Block *for_body = colsFor.getBody();
        ipStack.push(builder->saveInsertionPoint());
        builder->setInsertionPointToStart(for_body);
        mlir::Value element_ptr = builder->create<mlir::LLVM::GEPOp>(
                   loc, ptr_type, i8type, bufferPtr, mlir::ValueRange{index});
        const mlir::Value charVal = builder->create<mlir::LLVM::LoadOp>(loc, i8type, element_ptr);
        storeArrVal(vec_addr, index, charVal, charType);
        builder->restoreInsertionPoint(ipStack.top());
        ipStack.pop();
        return vec_addr;
    }if (isFloatType(value)){
        auto c11 = builder->create<mlir::LLVM::ConstantOp>(loc, i64type, 50);
        auto c0i32 = emitInt(0);
        auto c1i32 = emitInt(1);
        auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
        mlir::LLVM::LLVMFuncOp mallocFn = mlir::LLVM::lookupOrCreateMallocFn(module, i64type);
        const mlir::Value bufferPtr = builder->create<mlir::LLVM::CallOp>(
            loc, mallocFn, mlir::ValueRange{c11}).getResult();
        auto formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("floatFormat");
        const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
        const mlir::Value doubleVal = builder->create<mlir::LLVM::FPExtOp>(loc, builder->getF64Type(), value);
        mlir::ValueRange args = {bufferPtr,formatStringPtr,doubleVal};
        auto sprintfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("sprintf");
        builder->create<mlir::LLVM::CallOp>(loc, sprintfFunc, args);
        auto strlen = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("strlen");
        mlir::ValueRange argsStrLen = {bufferPtr};
        auto vecSize = builder->create<mlir::LLVM::CallOp>(loc, strlen,argsStrLen).getResult();
        vec_addr = createVector(vecSize,charType);
        std::stack<mlir::OpBuilder::InsertPoint> ipStack;
        auto colsFor = builder->create<mlir::scf::ForOp>(
             loc, c0i32, vecSize, c1i32);
        mlir::Value index = colsFor.getInductionVar();
        mlir::Block *for_body = colsFor.getBody();
        ipStack.push(builder->saveInsertionPoint());
        builder->setInsertionPointToStart(for_body);
        mlir::Value element_ptr = builder->create<mlir::LLVM::GEPOp>(
                   loc, ptr_type, i8type, bufferPtr, mlir::ValueRange{index});
        const mlir::Value charVal = builder->create<mlir::LLVM::LoadOp>(loc, i8type, element_ptr);
        storeArrVal(vec_addr, index, charVal, charType);
        builder->restoreInsertionPoint(ipStack.top());
        ipStack.pop();
        return vec_addr;
    }
}