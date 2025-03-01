#include <iostream>
#include <Symbol.h>
#include "BackEnd.h"
#include <mlir/Dialect/LLVMIR/FunctionCallUtils.h>


void BackEnd::outStream(const mlir::Value val) {
    if (isIntegerType(val)) {
        printInt(val);
    }if (isCharType(val)) {
        printChar(val);
    }if (isFloatType(val)) {
        printFloat(val);
    }if (isBoolType(val)) {
        printBool(val);
    }
}


mlir::Value BackEnd::inStream(const std::shared_ptr<Type> &type){
  // Lookup table
  static const std::unordered_map<Type::BaseType, std::tuple<int, const char*, bool, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, "bufferIntFormat", false, builder->getI32Type()}},
    {Type::BaseType::real,      {4, "bufferFloatFormat", false, builder->getF32Type()}},
    {Type::BaseType::character, {1, "bufferCharFormat", false, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, "bufferBoolFormat", true, builder->getI8Type()}},
  };

  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int64Ty = builder->getI64Type();
  auto int32Ty = builder->getI32Type();

  // Constants
  auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto one = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);
  auto four = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 4);
  auto c512 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 512);

  // Get the buffer address
  auto bufferGlobal = module.lookupSymbol<mlir::LLVM::GlobalOp>("instream_buffer_512");
  auto bufferGlobalAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, bufferGlobal);
  auto bufferGlobalPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, bufferGlobalAddr);

  // Stream state
  auto streamState = module.lookupSymbol<mlir::LLVM::GlobalOp>("stream_state_g_int");
  auto streamStateAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, streamState);

  // use sscanf on buffer if it returns 0 then populate the buffer first using scanf
  auto sscanfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("sscanf");
  auto scanfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("scanf");
  auto mallocFn = mlir::LLVM::lookupOrCreateMallocFn(module, int64Ty);
  auto freeFn = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("free");
  auto checkStdin = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("checkStdin");

  mlir::Type inputType;
  std::string typeFormatStr;
  mlir::Value spaceAlloc;
  bool checkBoolean = false;
  const auto typeCheck = std::dynamic_pointer_cast<PrimitiveType>(type);
  if (auto it = typeInfo.find(typeCheck->base_type); it != typeInfo.end()) {
    auto [allocSize, formatStr, isBool, iType] = it->second;
    spaceAlloc = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, allocSize);
    typeFormatStr = formatStr;
    checkBoolean = isBool;
    inputType = iType;
  }

  // Create a pointer to store the read value
  auto inputAddr = builder->create<mlir::LLVM::CallOp>(loc, mallocFn, mlir::ValueRange{spaceAlloc}).getResult();
  auto bytesReadAddr = builder->create<mlir::LLVM::CallOp>(loc, mallocFn, mlir::ValueRange{four}).getResult();
  auto formatStr = module.lookupSymbol<mlir::LLVM::GlobalOp>(typeFormatStr);
  auto formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatStr);
  auto stringFormat = module.lookupSymbol<mlir::LLVM::GlobalOp>("bufferStringFormat");
  auto stringFormatPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, stringFormat);
  auto charFormat = module.lookupSymbol<mlir::LLVM::GlobalOp>("bufferCharFormat");
  auto charFormatPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, charFormat);

  // Check if the buffer is not empty
  auto buffLenG = module.lookupSymbol<mlir::LLVM::GlobalOp>("instream_buffer_512_len");
  auto buffLenAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, buffLenG);
  auto buffLen = builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, buffLenAddr);
  auto condition = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::eq, buffLen, zero);
  builder->create<mlir::scf::IfOp>(
    loc,
    condition,
    [&](mlir::OpBuilder &b, mlir::Location bloc) {
      // Fill the buffer
      builder->create<mlir::LLVM::CallOp>(bloc, scanfFunc, mlir::ValueRange{stringFormatPtr, bufferGlobalPtr, bytesReadAddr});

      // store len in buffLen
      auto len = builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, bytesReadAddr);
      builder->create<mlir::LLVM::StoreOp>(loc, len, buffLenAddr);
      b.create<mlir::scf::YieldOp>(loc);
    }
  );

  // Read from the buffer then check if it was success else fill the buffer using scanf and try reading again
  auto readSuccess = builder->create<mlir::LLVM::CallOp>(loc, sscanfFunc, mlir::ValueRange{bufferGlobalPtr, formatStringPtr, inputAddr, bytesReadAddr}).getResult();
  storeStreamState(readSuccess);

  // If the read failed and len of the buff is lt 512 then fill the buffer by those number of bytes
  auto failed = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, readSuccess, one);
  auto len = builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, buffLenAddr);
  auto lenlt512 = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, len, c512);
  auto condition2 = builder->create<mlir::arith::AndIOp>(loc, failed, lenlt512);
  builder->create<mlir::scf::IfOp>(
    loc,
    condition2,
    [&](mlir::OpBuilder &b, mlir::Location bloc) {
      // Fill the buffer
      builder->create<mlir::scf::WhileOp>(
        loc,
        mlir::TypeRange(),
        mlir::ValueRange(),
        // While loop condition
        [&](mlir::OpBuilder &b, mlir::Location bloc, mlir::ValueRange args) {
          auto len = builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, buffLenAddr);
          auto lenLt512 = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, len, c512);
          b.create<mlir::scf::ConditionOp>(loc, lenLt512, args);
        },
        // While loop body
        [&](mlir::OpBuilder &b, mlir::Location bloc, mlir::ValueRange args) {
          auto len = builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, buffLenAddr);
          auto gep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, builder->getI8Type(), bufferGlobalPtr, mlir::ValueRange{len});
          builder->create<mlir::LLVM::CallOp>(bloc, scanfFunc, mlir::ValueRange{charFormatPtr, gep, bytesReadAddr});

          //Increment the length
          auto newlen = builder->create<mlir::LLVM::AddOp>(loc, len, one);
          builder->create<mlir::LLVM::StoreOp>(loc, newlen, buffLenAddr);
          b.create<mlir::scf::YieldOp>(loc, args);
        });


      // Try reading from the buffer again
      readSuccess = builder->create<mlir::LLVM::CallOp>(loc, sscanfFunc, mlir::ValueRange{bufferGlobalPtr, formatStringPtr, inputAddr, bytesReadAddr}).getResult();
      storeStreamState(readSuccess);
      b.create<mlir::scf::YieldOp>(loc);
    }
  );

  // Increment the buffer by the bytes read if success
  readSuccess = loadStreamState();
  condition = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sgt, readSuccess, zero);
  builder->create<mlir::scf::IfOp>(
    loc,
    condition,
    [&](mlir::OpBuilder &b, mlir::Location bloc) {
      // Increment the pointer if bytes is not eq 512
      auto bytes = builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, bytesReadAddr);
      auto cond = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::eq, bytes, c512);
      auto incr = builder->create<mlir::arith::SelectOp>(loc, cond, zero, bytes);
      auto incrPtr = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, builder->getI8Type(), bufferGlobalPtr, mlir::ValueRange{incr});
      builder->create<mlir::LLVM::StoreOp>(loc, incrPtr, bufferGlobalAddr);

      // decrement the length
      len = builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, buffLenAddr);
      auto newlen = builder->create<mlir::LLVM::SubOp>(loc, len, bytes);
      builder->create<mlir::LLVM::StoreOp>(loc, newlen, buffLenAddr);
      storeStreamState(zero);
      builder->create<mlir::LLVM::CallOp>(loc, checkStdin, mlir::ValueRange{streamStateAddr});
      b.create<mlir::scf::YieldOp>(loc);
    },
    [&](mlir::OpBuilder &b, mlir::Location bloc){
      // Set state as one
      storeStreamState(one);
      builder->create<mlir::LLVM::StoreOp>(loc, zero, inputAddr);
      b.create<mlir::scf::YieldOp>(loc);
    }
  );

  // Return the value
  const mlir::Value result = builder->create<mlir::LLVM::LoadOp>(loc,inputType, inputAddr);

  // Free inputAddr and bytesReadAddr
  builder->create<mlir::LLVM::CallOp>(loc, freeFn, mlir::ValueRange{inputAddr});
  builder->create<mlir::LLVM::CallOp>(loc, freeFn, mlir::ValueRange{bytesReadAddr});

  if(checkBoolean) {
    return intEqualTo(result,emitChar('T'));
  }
  return result;
}


void BackEnd::printChar(const mlir::Value character) {
    //Maybe try to figure out how to make this into a function that you can call you finish everything else
    auto formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("charFormat");
    const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
    mlir::ValueRange args = {formatStringPtr, character};
    auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}


void BackEnd::printString(const mlir::Value string){
    mlir::ValueRange args = {string};
    auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}


void BackEnd::printInt(const mlir::Value val) {
    auto formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("intFormat");
    const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
    mlir::ValueRange args = {formatStringPtr, val};
    auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}


void BackEnd::printFloat(const mlir::Value val) {
    auto formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("floatFormat");
    const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
    const mlir::Value doubleVal = builder->create<mlir::LLVM::FPExtOp>(loc, builder->getF64Type(), val);
    mlir::ValueRange args = {formatStringPtr, doubleVal};
    auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}


void BackEnd::printBool(mlir::Value val) {
    auto formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("charFormat");
    const mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
    auto boolStr = builder->create<mlir::LLVM::SelectOp>(loc, val, emitChar('T'), emitChar('F'));
    mlir::ValueRange args = {formatStringPtr, boolStr};
    auto printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);
}

void BackEnd::printVector(mlir::Value vector_addr, std::shared_ptr<Type> &type) {



    std::stack<mlir::OpBuilder::InsertPoint> ipStack;
    const bool isString= std::dynamic_pointer_cast<VectorType>(type)->isString;

    //Load brackets
    auto whiteSpace  = module.lookupSymbol<mlir::LLVM::GlobalOp>("whiteSpace");
    auto leftBracket  = module.lookupSymbol<mlir::LLVM::GlobalOp>("leftBracket");
    auto rightBracket  = module.lookupSymbol<mlir::LLVM::GlobalOp>("rightBracket");

    //Get address of each opperation
    auto whiteSpacePtr = builder->create<mlir::LLVM::AddressOfOp>(loc, whiteSpace);
    auto leftBracketPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, leftBracket);
    auto rightBracketPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, rightBracket);

    //Obtain the size of the array.
    auto arr_size = getArrSize(vector_addr);
    auto const32_zero = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0);

    auto const32_one = builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 1);
    //How do we make sure we are not putting a space in the last element?
    //Print everything but the last element
    if (!isString) {
        printString(leftBracketPtr);
    }
    auto arr_bound = emitSub(arr_size, const32_one);
    mlir::scf::ForOp range_loop = builder->create<mlir::scf::ForOp>(
            loc, const32_zero, arr_bound, const32_one);


    //Now how should a for loop work
    mlir::Value index = range_loop.getInductionVar();
    mlir::Block *forBody = range_loop.getBody();

    //Store insertion point in the stack
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(forBody);
    auto value = loadArrVal(vector_addr, index, type);
    //I could make this simpler, and more elegant I know but at this point we are in a time crunch and i could not bother tbh.

    if (isIntegerType(value)) {
        printInt(value);
    } else if (isCharType(value)) {
        printChar(value);
    }else if (isFloatType(value)) {
        printFloat(value);
    }else if (isBoolType(value)) {
        printBool(value);
    }

    //If else statement depending on the type
    //Print white space regardless
    if (!isString) {
        printString(whiteSpacePtr);
    }
    //Restore insertion point to normal logic
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();
    //If your index is greater than one then print the last element, otherwise

    mlir::Value greater_than_one = builder->create<mlir::LLVM::ICmpOp>(
            loc, builder->getI1Type(), mlir::LLVM::ICmpPredicate::sgt, arr_size, const32_zero);
    auto if_op = builder->create<mlir::scf::IfOp>(loc, greater_than_one, false);
    // Populate the 'then' block
    ipStack.push(builder->saveInsertionPoint());
    builder->setInsertionPointToStart(if_op.thenBlock());
    auto last_val = loadArrVal(vector_addr, arr_bound, type);

    //I know I could make this simpler but yet again i could not bother for the love of jesus christ
    if (isIntegerType(last_val)) {
        printInt(last_val);
    } else if (isCharType(last_val)) {
        printChar(last_val);
    }else if (isFloatType(last_val)) {
        printFloat(last_val);
    }else if (isBoolType(last_val)) {
        printBool(last_val);
    }
    builder->restoreInsertionPoint(ipStack.top());
    ipStack.pop();
    if (!isString) {
        printString(rightBracketPtr);
    }
    //freeVector(vector_addr);

}



void BackEnd::storeStreamState(mlir::Value state){
  auto streamState = module.lookupSymbol<mlir::LLVM::GlobalOp>("stream_state_g_int");
  auto streamStateAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, streamState);
  builder->create<mlir::LLVM::StoreOp>(loc, state, streamStateAddr);
}

mlir::Value BackEnd::loadStreamState(){
  auto streamState = module.lookupSymbol<mlir::LLVM::GlobalOp>("stream_state_g_int");
  auto streamStateAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, streamState);
  return builder->create<mlir::LLVM::LoadOp>(loc, builder->getI32Type(), streamStateAddr);
}

void BackEnd::emitPrintMatrix(mlir::Value matrix,  const std::shared_ptr<Type>& type){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int32Ty = builder->getI32Type();

  mlir::Type matrixType;
  if (auto it = typeInfo.find(type->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    matrixType = mType;
  }

  // Constants
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);

  //Load brackets
  auto whiteSpace  = module.lookupSymbol<mlir::LLVM::GlobalOp>("whiteSpace");
  auto leftBracket  = module.lookupSymbol<mlir::LLVM::GlobalOp>("leftBracket");
  auto rightBracket  = module.lookupSymbol<mlir::LLVM::GlobalOp>("rightBracket");
  auto whiteSpacePtr = builder->create<mlir::LLVM::AddressOfOp>(loc, whiteSpace);
  auto leftBracketPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, leftBracket);
  auto rightBracketPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, rightBracket);

  // Get the size of the matrix
  mlir::Value rows = emitGetRows(matrix);
  mlir::Value cols = emitGetCols(matrix);

  // Get the matrix pointer
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i32});
  auto matrixPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, matrixGep);

  // Print "["
  printString(leftBracketPtr);

  // for loops to get the elements of the matrix
  std::stack<mlir::OpBuilder::InsertPoint> ipStack;
  auto rowsFor = builder->create<mlir::scf::ForOp>(loc, c0i32, rows, c1i32);
  mlir::Value i = rowsFor.getInductionVar();
  mlir::Block *rowsForBody = rowsFor.getBody();
  ipStack.push(builder->saveInsertionPoint());
  builder->setInsertionPointToStart(rowsForBody);

  // Inside the rows for
  // Print "["
  printString(leftBracketPtr);

  auto colsFor = builder->create<mlir::scf::ForOp>(loc, c0i32, cols, c1i32);
  mlir::Value j = colsFor.getInductionVar();
  mlir::Block *colsForBody = colsFor.getBody();
  ipStack.push(builder->saveInsertionPoint());
  builder->setInsertionPointToStart(colsForBody);
  // inside the cols for
  // Calculate the index (i * cols + j)
  mlir::Value index = builder->create<mlir::LLVM::MulOp>(loc, i, cols);
  index = builder->create<mlir::LLVM::AddOp>(loc, index, j);

  // Load the value
  auto elementGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, matrixType, matrixPtr, mlir::ValueRange{index});
  auto element = builder->create<mlir::LLVM::LoadOp>(loc, matrixType, elementGep);

  outStream(element);

  // print the whitespace if j != cols-1
  auto c = builder->create<mlir::arith::SubIOp>(loc, cols, c1i32);
  auto cmp = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, j, c);
  builder->create<mlir::scf::IfOp>(
    loc,
    cmp,
    [&](mlir::OpBuilder &b, mlir::Location bloc){
      printString(whiteSpacePtr);
      b.create<mlir::scf::YieldOp>(bloc);
    });


  builder->restoreInsertionPoint(ipStack.top());
  ipStack.pop();

  // Print "] "
  printString(rightBracketPtr);

  // print the whitespace if i != rows-1
  auto r = builder->create<mlir::arith::SubIOp>(loc, rows, c1i32);
  cmp = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::ne, i, r);
  builder->create<mlir::scf::IfOp>(
    loc,
    cmp,
    [&](mlir::OpBuilder &b, mlir::Location bloc){
      printString(whiteSpacePtr);
      b.create<mlir::scf::YieldOp>(bloc);
    });

  builder->restoreInsertionPoint(ipStack.top());
  ipStack.pop();

  printString(rightBracketPtr);
}