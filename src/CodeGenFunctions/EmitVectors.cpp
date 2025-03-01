//
// Created by judafer2000 on 11/25/24.
//
#include <Symbol.h>

#include "BackEnd.h"
#include "mlir/Dialect/LLVMIR/FunctionCallUtils.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"


mlir::Value BackEnd::createVector(mlir::Value size, std::shared_ptr<Type> type) {
    /***
    createVector:

    Description: Takes an mlir::Value size parameter integer and creates a vector of 32 bit integers of the specified size.
    The vector itself is in the form of
    typedef struct {
             uint64_t size;
             [type] type;
             int *array;
     } Vector;
    //Arguments: mlir::Value size - Size of the vector.
    //Returns: mlir::Value vector_addr -> Pointer that contains the value of the struct
    ***/

    // Create the vector type struct

    // Allocate space for a vector
    auto int64_type = builder->getI64Type();
    //This should be replaced with create global at the beginning of the program.
    auto const64_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 0);
    auto const64_one = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 1);
    auto const64_four = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 4);

    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);

    mlir::Type element_type;
    if (type->getBaseType() == Type::BaseType::integer) {
        element_type = builder->getI32Type();
    } else if(type->getBaseType() == Type::BaseType::boolean) {
        element_type = builder->getI1Type();
    } else if (type->getBaseType() == Type::BaseType::real) {
        element_type = builder->getF32Type();
    } else if (type->getBaseType() == Type::BaseType::character) {
        element_type = builder->getI8Type();
    }




    mlir::ArrayRef<mlir::Type> member_types = {int64_type, ptr_type, element_type};
    auto vector_type = mlir::LLVM::LLVMStructType::getLiteral(&context, member_types);

    mlir::Value vector_addr = builder->create<mlir::LLVM::AllocaOp>(
        loc, ptr_type, vector_type, const64_one); //This is the address of our struct

    //So the struct itself is allocated in the stack, but the array is allocated in the heap.

    //Zero extend int32 size to int 64 (Size its always positive, and we cast to int64 to prevent issues when allocating)
    mlir::Value int64_size = builder->create<mlir::LLVM::ZExtOp>(loc, int64_type, size);


    auto byte_size = builder->create<mlir::LLVM::MulOp>(loc, builder->getI64Type(), const64_four, int64_size);

    // Obtain the address of the size element mlir::ValueRange{const_zero, const_zero} denotes to go to the 0th element of the struct
    //However more explanation might be needed on this arg...
    mlir::Value sizeAddr = builder->create<mlir::LLVM::GEPOp>(
        loc, ptr_type, vector_type, vector_addr, mlir::ValueRange{const64_zero, const64_zero});
    builder->create<mlir::LLVM::StoreOp>(loc, int64_size, sizeAddr);
    // Allocate the array
    mlir::LLVM::LLVMFuncOp mallocFn = mlir::LLVM::lookupOrCreateMallocFn(module, int64_type);
    mlir::Value arrayPtr = builder->create<mlir::LLVM::CallOp>(
        loc, mallocFn, mlir::ValueRange{byte_size}).getResult();

    // Store the array pointer in the struct
    mlir::Value arrayAddr = builder->create<mlir::LLVM::GEPOp>(
        loc, ptr_type, vector_type, vector_addr, mlir::ValueRange{const64_zero, const64_one});
    builder->create<mlir::LLVM::StoreOp>(loc, arrayPtr, arrayAddr);

    //Fill the array with zeros.


    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 0);

    auto const32_one= builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 1);

    mlir::Value vec_zero;

    //I don't even know if this helps to be completely honest, but just in case I am placing this if/else statement here
    if (type->getBaseType() == Type::BaseType::integer) {
        vec_zero = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(), 0);
    } else if (type->getBaseType() == Type::BaseType::real) {
        vec_zero = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(),  mlir::FloatAttr::get(builder->getF32Type(), 0.0f));
    } else if (type->getBaseType() == Type::BaseType::character) {
        vec_zero = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(),  mlir::IntegerAttr::get(builder->getI8Type(), 0));
    } else if (type->getBaseType() == Type::BaseType::boolean) {
        vec_zero = builder->create<mlir::LLVM::ConstantOp>(
        loc, builder->getI32Type(),  mlir::IntegerAttr::get(builder->getI1Type(), 0));

    }

    mlir::scf::ForOp add_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, size, const32_one);

    //Now how should a for loop work
    mlir::Value index = add_loop.getInductionVar();
    mlir::Block *for_body = add_loop.getBody();
    //Store insertion point in the stack
    auto old_insert_point = builder->saveInsertionPoint();
    builder->setInsertionPointToStart(for_body);
    storeArrVal(vector_addr, index, vec_zero, type);
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(old_insert_point);






    return vector_addr;
}





void BackEnd::freeVector(mlir::Value vector_addr) {

    //Pass a vector address
    mlir::LLVM::LLVMFuncOp free_func = mlir::LLVM::lookupOrCreateFreeFn(module);


    //Load the array of the vector value.

    auto int64_type = builder->getI64Type();
    //TODO: This should be replaced with create global at the beginning of the program.
    auto const64_zero = builder->create<mlir::LLVM::ConstantOp>(
            loc, builder->getI64Type(), 0);
    auto const64_one = builder->create<mlir::LLVM::ConstantOp>(
            loc, builder->getI64Type(), 1);
    auto const64_four = builder->create<mlir::LLVM::ConstantOp>(
            loc, builder->getI64Type(), 4);



    auto int32_type = builder->getI32Type();
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    mlir::ArrayRef<mlir::Type> member_types = {int64_type, ptr_type};
    auto vector_type = mlir::LLVM::LLVMStructType::getLiteral(&context, member_types);




    mlir::Value struct_arr_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc, ptr_type, vector_type, vector_addr, mlir::ValueRange{const64_zero, const64_one});

    auto arr_ptr= builder->create<mlir::LLVM::LoadOp>(loc, ptr_type, struct_arr_ptr)->getResult(0);

    builder->create<mlir::LLVM::CallOp>(loc, free_func, arr_ptr);
    //Free the memory inside of vector

    //Return
}

mlir::Value BackEnd::emitVectorLiteral(std::vector<mlir::Value> vector, mlir::Value size, std::shared_ptr<Type> type) {
    //Sadly to emit a literal there will have to be lots of loads and stores as there is no way to
    //do them all in a nice llvm loop.

    auto vector_addr = createVector(size, type);

    //Promote it using the type promotion matrix.

    for (int i =0; i < vector.size(); i++) {
        auto val = vector[i];
        val = typeCast(val, type->getBaseType());
        storeArrVal(vector_addr, emitInt(i), val, type);
    }

    return vector_addr;
}







mlir::Value BackEnd::createMatrixGenerator(mlir::Value vector_addr1, mlir::Value vector_addr2, mlir::Block * expr_block,
        std::shared_ptr<Symbol> id1, std::shared_ptr<Symbol> id2, mlir::Value result, std::shared_ptr<Type> &result_type,
        std::shared_ptr<Type> &dom1_type,std::shared_ptr<Type> &dom2_type, int line) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;

    //The domain of a generator must always be evaluated first
    //Remove operation from scope where it was inserted
    auto arr_size1 = getArrSize(vector_addr1);


    auto arr_size2 = getArrSize(vector_addr2);



    auto result_mat = emitZeroInitMatrix(arr_size1, arr_size2, result_type);

    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 0);

    auto const32_one= builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 1);

    mlir::scf::ForOp row_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size1, const32_one);

    mlir::Value index_i = row_loop.getInductionVar();
    mlir::Block *row_body = row_loop.getBody();
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(row_body);
    //Go into the second for loop

    mlir::scf::ForOp col_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size1, const32_one);

    mlir::Value index_j = col_loop.getInductionVar();
    mlir::Block *col_body = col_loop.getBody();
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(col_body);

    auto dom_val1 = loadArrVal(vector_addr1, index_i, dom1_type);
    emitAssignId(id1, dom_val1, line); //Assign the current value to the id

    auto dom_val2 = loadArrVal(vector_addr2, index_j, dom2_type);

    emitAssignId(id2, dom_val2, line);
    mergeBlocks(expr_block,  col_body, builder->getInsertionPoint());


    //setMatrixIndexValue(result,
    //result = typeCast(result, result_type->getBaseType()); //In case value is the wrong type cast result of the operation to the correct type.
    setMatrixIndexValueMLIR(result, result_mat, index_i, index_j, result_type);




    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();



    //storeArrVal(result_vec, index, result, result_type);

    //freeVector(vector_addr);

    //May god forgive me for the spaghetti I have written

    return result_mat;
}



mlir::Value BackEnd::createVectorGenerator(mlir::Value vector_addr, mlir::Block * expr_block,
    std::shared_ptr<Symbol> id, mlir::Value result, std::shared_ptr<Type> &result_type,
    std::shared_ptr<Type> &dom1_type, int line)  {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;

    //The domain of a generator must always be evaluated first
    //Remove operation from scope where it was inserted
    auto arr_size = getArrSize(vector_addr);

    auto result_vec = createVector(arr_size, result_type);

    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 0);

    auto const32_one= builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 1);

    std::cout << "If statement failed" << std::endl;
    mlir::scf::ForOp gen_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);


    //Now the real fun starts
    mlir::Value index = gen_loop.getInductionVar();
    mlir::Block *for_body = gen_loop.getBody();
    //std::cout << "Going inside for body statement" << std::endl;

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());

    builder->setInsertionPointToStart(for_body);
    std::cout << "Obtaining domain value" << std::endl;


    auto dom_val = loadArrVal(vector_addr, index, dom1_type);
    emitAssignId(id, dom_val, line); //Assign the current value to the id
    std::cout << "Obtaining store value" << std::endl;

    //Remove block from region
    mergeBlocks(expr_block,  for_body, builder->getInsertionPoint());
    result = typeCast(result, result_type->getBaseType()); //In case value is the wrong type cast result of the operation to the correct type.
    storeArrVal(result_vec, index, result, result_type);


    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();

    //The expression for a generator is a strange beast.
    freeVector(vector_addr);
    return result_vec;

}





mlir::Value BackEnd::createFilter(mlir::Value vector_address, mlir::Block *predicate_block, std::shared_ptr<Symbol> id,
    mlir::Value result, std::shared_ptr<Type> &domain_type) {
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 0);

    auto const32_one= builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 1);
    auto const32_four= builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 4);

    auto int32_type = builder->getI32Type();
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);

    std::stack<mlir::OpBuilder::InsertPoint> ipStack;

    //The domain of a filter must always be evaluated first
    //Remove operation from scope where it was inserted
    auto arr_size = getArrSize(vector_address);

    // we start with a vector with the same size of the array, but we may cut it down depending on if some of the
    // elements don't satisfy the predicate
    std::cout << "Creating vector..." << std::endl;
    auto result_vec = createVector(arr_size, domain_type);

    std::cout << "Creating the insertion index (hit counter)" << std::endl;
    auto insert_index = builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, int32_type, const32_four);
    std::cout << "space allocated!" << std::endl;
    builder->create<mlir::LLVM::StoreOp>(loc, const32_zero, insert_index);


    // create
    mlir::scf::ForOp filter_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_size, const32_one);

    std::cout << "Going inside for block statement" << std::endl;
    mlir::Value index = filter_loop.getInductionVar();
    mlir::Block *for_body = filter_loop.getBody();
    std::cout << "Going inside for body statement" << std::endl;

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());

    builder->setInsertionPointToStart(for_body);
    std::cout << "Obtaining domain value" << std::endl;

    auto element_val = loadArrVal(vector_address, index, domain_type);

    emitAssignId(id, element_val, 0);
    std::cout << "Obtaining store value" << std::endl;

    //Remove block from region
    mergeBlocks(predicate_block,  for_body, builder->getInsertionPoint());
    std::cout << "Obtaining store value" << std::endl;


    mlir::Value condition = emitNotEqual(result, emitBool(false));

    // Create a conditional operation (if the condition is true, add the element. Then increment the index)
    builder->create<mlir::scf::IfOp>(loc, condition, [&](mlir::OpBuilder &b, mlir::Location loc) {
        auto current_insert_index = builder->create<mlir::LLVM::LoadOp>(loc, int32_type, insert_index);

        storeArrVal(result_vec, current_insert_index, element_val, domain_type); // Add element to result vector

        auto incremented_index = b.create<mlir::LLVM::AddOp>(loc, current_insert_index, const32_one);

        builder->create<mlir::LLVM::StoreOp>(loc, incremented_index, insert_index);

        b.create<mlir::scf::YieldOp>(loc); // Yield in the "then" block
    });

    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();

    // now we need to resize the vector
    auto resized_vec = resizeVector(result_vec, insert_index, domain_type); // THe result vector gets freed in here
    //freeVector(result_vec); //However we are free to free the result vector

    //We do not free the domain in this case as we will need itt again
    return resized_vec;

};

mlir::Value BackEnd::resizeVector(mlir::Value old_vector, mlir::Value index_ptr, std::shared_ptr<Type> &domain_type) {
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 0);

    auto const32_one= builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 1);

    auto int32_type = builder->getI32Type();

    // load the index value (hit counter) into index
    auto index = builder->create<mlir::LLVM::LoadOp>(loc, int32_type, index_ptr);

    // make a new vector with the size we want the new one to be
    auto resized_vector = createVector(index, domain_type);

    // loop <index> times, adding the elements from old vector into new vector
    auto loop = builder->create<mlir::scf::ForOp>(loc, const32_zero, index, const32_one);

    mlir::Value loop_index = loop.getInductionVar();
    mlir::Block *loop_body = loop.getBody();

    builder->setInsertionPointToStart(loop_body);

    auto element = loadArrVal(old_vector, loop_index, domain_type);
    storeArrVal(resized_vector, loop_index, element, domain_type);

    builder->setInsertionPointAfter(loop);

    freeVector(old_vector);
    return resized_vector;
}





mlir::Value BackEnd::resizeVectorWithInt(mlir::Value old_vector, mlir::Value index, std::shared_ptr<Type> &domain_type) {
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 0);

    auto const32_one= builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 1);

    auto int32_type = builder->getI32Type();


    // make a new vector with the size we want the new one to be
    auto resized_vector = createVector(index, domain_type);

    // loop <index> times, adding the elements from old vector into new vector
    auto loop = builder->create<mlir::scf::ForOp>(loc, const32_zero, index, const32_one);

    mlir::Value loop_index = loop.getInductionVar();
    mlir::Block *loop_body = loop.getBody();

    builder->setInsertionPointToStart(loop_body);

    auto element = loadArrVal(old_vector, loop_index, domain_type);
    storeArrVal(resized_vector, loop_index, element, domain_type);

    builder->setInsertionPointAfter(loop);

    freeVector(old_vector);
    return resized_vector;
}










void BackEnd::mergeBlocks(mlir::Block *source_block, mlir::Block *target_block, mlir::Block::iterator insertion_point) {
    // Move operations from the source block to the target block
    while (!source_block->empty()) {
        mlir::Operation &op = source_block->front();
        op.remove();  // Remove the operation from the source block
        target_block->getOperations().insert(insertion_point, &op);  // Insert into destination block at insertion point
    }
}






void BackEnd::storeArrVal(mlir::Value vector_addr, mlir::Value index, mlir::Value value, std::shared_ptr<Type> &type) {

    auto int64_type = builder->getI64Type();
    //TODO: This should be replaced with create global at the beginning of the program.
    auto const64_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 0);
    auto const64_one = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 1);
    auto const64_four = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 4);




    auto arr_size = getArrSize(vector_addr);

    mlir::LLVM::LLVMFuncOp index_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorIndexCheck");

    builder->create<mlir::LLVM::CallOp>(loc, index_check, mlir::ValueRange{index, arr_size});


    auto int32_type = builder->getI32Type();
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    mlir::ArrayRef<mlir::Type> member_types = {int64_type, ptr_type};
    auto vector_type = mlir::LLVM::LLVMStructType::getLiteral(&context, member_types);



    mlir::Value array_addr = builder->create<mlir::LLVM::GEPOp>(
        loc, ptr_type, vector_type, vector_addr, mlir::ValueRange{const64_zero, const64_one});

    //Obtain array pointer inside of the array address in the struct.
    mlir::Value array_ptr = builder->create<mlir::LLVM::LoadOp>(loc, ptr_type, array_addr);
    //Set the pointer at the proper index

    mlir::Value element_ptr = builder->create<mlir::LLVM::GEPOp>(
        loc, ptr_type, int32_type, array_ptr, mlir::ValueRange{index});
    //Store the value
    builder->create<mlir::LLVM::StoreOp>(loc, value, element_ptr);

}



mlir::Value BackEnd::loadArrVal(mlir::Value vector_addr, mlir::Value index, std::shared_ptr<Type> &type) {
    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto int64_type = builder->getI64Type();
    //TODO: This should be replaced with create global at the beginning of the program.


    auto const64_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 0);
    auto const64_one = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 1);
    auto const64_four = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI64Type(), 4);
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(
    loc, builder->getI32Type(), 0);
    auto bool_type = builder->getI1Type();


    auto int32_type = builder->getI32Type();
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    mlir::ArrayRef<mlir::Type> member_types = {int64_type, ptr_type};
    auto vector_type = mlir::LLVM::LLVMStructType::getLiteral(&context, member_types);

    auto arr_size = getArrSize(vector_addr);

    mlir::LLVM::LLVMFuncOp size_check = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vectorIndexCheck");

    builder->create<mlir::LLVM::CallOp>(loc, size_check, mlir::ValueRange{index, arr_size});

    //TODO: Put size error here in case array is out of bounds




    auto result_type = getMLIRTypeFromBaseType(type);



    //Otherwise return the array size
    //-------------------------------then statement---------------------------------------------------------------------
    //Obtain the array element in the struct
    mlir::Value array_addr = builder->create<mlir::LLVM::GEPOp>(
            loc, ptr_type, vector_type, vector_addr, mlir::ValueRange{const64_zero, const64_one});
    //Obtain array pointer inside of the array address in the struct.
    mlir::Value array_ptr = builder->create<mlir::LLVM::LoadOp>(loc, ptr_type, array_addr);

    // Load the struct and access the last element (equivalent to printf("%d", v.array[127]);)
    mlir::Value element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc, ptr_type, int32_type, array_ptr, mlir::ValueRange{index});

    //Load element and return it to the caller
    mlir::Value loaded_val = builder->create<mlir::LLVM::LoadOp>(loc, result_type, element_ptr);

    return loaded_val;
}


mlir::Value BackEnd::getArrSize(mlir::Value vector_addr, bool truncated) {
    /***
    Description - Returns the size of a given vector
    Arguments: mlir::Value -> Pointer to vector struct that contains the elements inside of the vector
    Returns: mlir::Value int64 value of the array size (It might need to be casted down to int32 for further operations)
    ***/
    auto int64_type = builder->getI64Type();
    auto int32_type = builder->getI32Type();
    auto ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    mlir::ArrayRef<mlir::Type> member_types = {int64_type, ptr_type};
    auto vector_type = mlir::LLVM::LLVMStructType::getLiteral(&context, member_types);
    auto const64_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI64Type(), 0);
    mlir::Value size_addr = builder->create<mlir::LLVM::GEPOp>(
        loc, ptr_type, vector_type, vector_addr, mlir::ValueRange{const64_zero, const64_zero});
    mlir::Value arr_size = builder->create<mlir::LLVM::LoadOp>(loc, int64_type, size_addr);

    if (truncated) {
        arr_size = builder->create<mlir::LLVM::TruncOp>(loc, int32_type, arr_size);
    }
    return arr_size;
}
mlir::Value BackEnd::reverseVector(mlir::Value original_vector_addr, std::shared_ptr<Type> type) {

    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    auto const32_one  = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);
    auto const32_minus_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), -1);
    //Obtain the array with the maximum size, obtain the array with the minimum size
    auto arr_size = getArrSize(original_vector_addr);
    const auto result_vector = createVector(arr_size, type);
    auto reverse_loop =  builder->create<mlir::scf::ForOp>(
             loc, const32_one, arr_size, const32_one);

    //Now how should a for loop work
    mlir::Value index = reverse_loop.getInductionVar();
    mlir::Block *for_body = reverse_loop.getBody();
    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(for_body);
    auto revIndex = emitSub(arr_size,index);
    revIndex = emitSub(revIndex,const32_one);
    const auto left_val = loadArrVal(original_vector_addr, revIndex, type);
    storeArrVal(result_vector, index, left_val, type);
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();
    revIndex = emitSub(arr_size,const32_one);
    storeArrVal(result_vector, const32_zero, loadArrVal(original_vector_addr, revIndex, type), type);
    freeVector(original_vector_addr);
    return result_vector;
}


mlir::Value BackEnd::getVector(mlir::Value vector_addr) {

    //Create a function to clone your vector, here we pass it the
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    //Load vector struct
    auto val = std::any_cast<mlir::Value>(vector_addr);
    return builder->create<mlir::LLVM::LoadOp>(loc, ptrTy, val)->getResult(0);
}


mlir::Value BackEnd::emitRangeVector(mlir::Value upper_bound, mlir::Value lower_bound) {

    std::stack<mlir::OpBuilder::InsertPoint> ipStack;

    std::shared_ptr<Type> int_type = std::make_shared<PrimitiveType>(Type::BaseType::integer);

    auto zero = emitInt(0);
    auto one = emitInt(1);

    // calculate the size of the vector to be made
    auto size = emitSub(upper_bound, lower_bound);
    size = emitAdd(size, one);
    auto size_ptr = emitAndStoreInteger(size);

    size = emitAdd(size, one);
    // comp: size <= 0
    auto cond = emitLessEq(size, zero);

    // leave the size the same or set to zero depending on if size <= 0
    builder->create<mlir::scf::IfOp>(loc, cond, [&](mlir::OpBuilder &builder, mlir::Location loc) {

        // then branch, change size to 0;
        storeIntegerAtPointer(zero, size_ptr);
        builder.create<mlir::scf::YieldOp>(loc);
    }, nullptr);

    auto new_size = emitLoadInteger(size_ptr);
    auto range_vector = createVector(new_size, int_type);
    auto loop_upper_bound = emitAdd(new_size, one);

    auto loop_op = builder->create<mlir::scf::ForOp>(
        loc, zero, new_size, one);

    mlir::Value index = loop_op.getInductionVar();
    mlir::Block *loop_body = loop_op.getBody();

    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(loop_body);
    auto elem_val = emitAdd(index, lower_bound);
    storeArrVal(range_vector, index, elem_val, int_type);

    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();

    return range_vector;
}


//Need a type infer types and sizes



