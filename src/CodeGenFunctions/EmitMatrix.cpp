#include "BackEnd.h"
#include "Symbol.h"

#include <mlir/Dialect/LLVMIR/FunctionCallUtils.h>

mlir::Value BackEnd::emitInitializedMatrix(mlir::Value initValue ,mlir::Value rows, mlir::Value cols, std::shared_ptr<Type> &type){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int64Ty = builder->getI64Type();
  auto int32Ty = builder->getI32Type();

  // Functions
  auto mallocFn = mlir::LLVM::lookupOrCreateMallocFn(module, int64Ty);

  // Constants
  auto c16i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 16);
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c0i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);
  auto c1i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 1);

  // Get the matrix type and space to alloc
  auto typeCheck = std::dynamic_pointer_cast<MatrixType>(type);
  mlir::Type matrixType;
  mlir::Value space;
  if (auto it = typeInfo.find(typeCheck->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    space = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, mSpace);
    matrixType = mType;
  }

  if(typeCheck->getBaseType() == Type::BaseType::real){
    if (isIntegerType(initValue)){
      initValue = builder->create<mlir::arith::SIToFPOp>(loc, builder->getF32Type(), initValue);
    }
  }

  // Malloc
  auto matrix = builder->create<mlir::LLVM::CallOp>(loc, mallocFn, mlir::ValueRange{c16i64}).getResult();

  // Store the row
  auto rowGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, int32Ty, matrix, mlir::ValueRange{c0i32});
  builder->create<mlir::LLVM::StoreOp>(loc, rows, rowGep);

  // Store the col
  auto colGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, int32Ty, matrix, mlir::ValueRange{c1i32});
  builder->create<mlir::LLVM::StoreOp>(loc, cols, colGep);

  // Calculate the space needed for the matrix
  mlir::Value size = builder->create<mlir::arith::MulIOp>(loc, rows, cols);
  size = builder->create<mlir::arith::ExtSIOp>(loc, int64Ty, size);
  space = builder->create<mlir::arith::MulIOp>(loc, size, space);

  // Malloc the space for the matrix
  auto matrixPtr = builder->create<mlir::LLVM::CallOp>(loc, mallocFn, mlir::ValueRange{space}).getResult();

  // init all the elements by init value
  std::stack<mlir::OpBuilder::InsertPoint> ipStack;
  auto forLoop = builder->create<mlir::scf::ForOp>(loc, c0i64, size, c1i64);
  mlir::Value i = forLoop.getInductionVar();
  mlir::Block *forBody = forLoop.getBody();
  ipStack.push(builder->saveInsertionPoint());
  builder->setInsertionPointToStart(forBody);
  auto elementGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, matrixType, matrixPtr, mlir::ValueRange{i});
  builder->create<mlir::LLVM::StoreOp>(loc, initValue, elementGep);
  builder->restoreInsertionPoint(ipStack.top());
  ipStack.pop();

  // Store the pointer in the struct
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i64});
  builder->create<mlir::LLVM::StoreOp>(loc, matrixPtr, matrixGep);

  // Return the matrix
  return matrix;
}

mlir::Value BackEnd::emitZeroInitMatrix(mlir::Value rows, mlir::Value cols, std::shared_ptr<Type> &type){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Get the matrix type and space to alloc
  auto typeCheck = std::dynamic_pointer_cast<MatrixType>(type);
  mlir::Type matrixType;
  if (auto it = typeInfo.find(typeCheck->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    matrixType = mType;
  }

  if (typeCheck->getBaseType() != Type::BaseType::real){
    auto zero = builder->create<mlir::LLVM::ConstantOp>(loc, matrixType, 0);
    return emitInitializedMatrix(zero, rows, cols, type);
  }
  auto zero = builder->create<mlir::LLVM::ConstantOp>(
    loc,
    builder->getF32Type(),
    mlir::FloatAttr::get(builder->getF32Type(), 0.0f)
  );
  return emitInitializedMatrix(zero, rows, cols, type);
}

mlir::Value BackEnd::emitGetRows(mlir::Value matrix){
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int32Ty = builder->getI32Type();
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto rowGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, int32Ty, matrix, mlir::ValueRange{c0i32});
  return builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, rowGep);
}

mlir::Value BackEnd::emitGetCols(mlir::Value matrix){
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int32Ty = builder->getI32Type();
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);
  auto colGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, int32Ty, matrix, mlir::ValueRange{c1i32});
  return builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, colGep);
}

mlir::Value BackEnd::loadMatrixId(const std::shared_ptr<Symbol>& symbol){
  auto matrix = std::any_cast<mlir::Value>(symbol->value);
  return cloneMatrix(matrix, symbol->type);
}

mlir::Value BackEnd::cloneMatrix(mlir::Value matrix, const std::shared_ptr<Type>& type){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int64Ty = builder->getI64Type();
  auto int32Ty = builder->getI32Type();

  // Constants
  auto c16i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 16);
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);
  auto c1i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 1);

  // Functions
  auto mallocFn = mlir::LLVM::lookupOrCreateMallocFn(module, int64Ty);

  // Get the matrix type and space to alloc
  mlir::Value space;
  if (auto it = typeInfo.find(type->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    space = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, mSpace);
  }

  // Get the size of the matrix
  auto rows = emitGetRows(matrix);
  auto cols = emitGetCols(matrix);
  mlir::Value size = builder->create<mlir::arith::MulIOp>(loc, rows, cols);
  size = builder->create<mlir::arith::ExtSIOp>(loc, int64Ty, size);
  space = builder->create<mlir::arith::MulIOp>(loc, space, size);

  // Get the matrix pointer
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i64});
  auto matrixPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, matrixGep);

  auto copiedMatrixPtr = builder->create<mlir::LLVM::CallOp>(loc, mallocFn, mlir::ValueRange{space}).getResult();
  builder->create<mlir::LLVM::MemcpyOp>(loc, copiedMatrixPtr, matrixPtr, space, 0);

  // Store the matrix in the struct
  auto copiedMatrix = builder->create<mlir::LLVM::CallOp>(loc, mallocFn, mlir::ValueRange{c16i64}).getResult();

  // Store the row
  auto rowGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, int32Ty, copiedMatrix, mlir::ValueRange{c0i32});
  builder->create<mlir::LLVM::StoreOp>(loc, rows, rowGep);

  // Store the col
  auto colGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, int32Ty, copiedMatrix, mlir::ValueRange{c1i32});
  builder->create<mlir::LLVM::StoreOp>(loc, cols, colGep);

  // Store the pointer in the struct
  auto copiedMatrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, copiedMatrix, mlir::ValueRange{c1i64});
  builder->create<mlir::LLVM::StoreOp>(loc, copiedMatrixPtr, copiedMatrixGep);

  return copiedMatrix;
}

void BackEnd::freeMatrix(mlir::Value matrix){
  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int64Ty = builder->getI64Type();

  // Constants
  auto c1i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 1);

  // Functions
  auto freeFn = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("free");

  // Get the matrix pointer
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i64});
  auto matrixPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, matrixGep);

  // Free the matrix ptr
  builder->create<mlir::LLVM::CallOp>(loc, freeFn, mlir::ValueRange{matrixPtr});
  builder->create<mlir::LLVM::CallOp>(loc, freeFn, mlir::ValueRange{matrix});
}

void BackEnd::setRowValues(mlir::Value val, mlir::Value matrix, int r, const std::shared_ptr<Type>& type){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Get the matrix type and space to alloc
  auto typeCheck = std::dynamic_pointer_cast<MatrixType>(type);
  mlir::Type matrixType;
  if (auto it = typeInfo.find(typeCheck->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    matrixType = mType;
  }

  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int64Ty = builder->getI64Type();
  auto int32Ty = builder->getI32Type();

  // Constants
  auto c1i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 1);
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);

  // Get the matrix pointer
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i64});
  auto matrixPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, matrixGep);

  // Get number of cols
  auto cols = emitGetCols(matrix);
  auto row = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, r);

  if(typeCheck->getBaseType() == Type::BaseType::real){
    if (isIntegerType(val)){
      val = builder->create<mlir::arith::SIToFPOp>(loc, builder->getF32Type(), val);
    }
  }

  std::stack<mlir::OpBuilder::InsertPoint> ipStack;
  auto colsFor = builder->create<mlir::scf::ForOp>(loc, c0i32, cols, c1i32);
  mlir::Value j = colsFor.getInductionVar();
  mlir::Block *colsForBody = colsFor.getBody();
  ipStack.push(builder->saveInsertionPoint());
  builder->setInsertionPointToStart(colsForBody);
  // inside the cols for
  // Calculate the index (i * cols + j)
  mlir::Value index = builder->create<mlir::LLVM::MulOp>(loc, row, cols);
  index = builder->create<mlir::LLVM::AddOp>(loc, index, j);

  // Store the value
  auto elementGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, matrixType, matrixPtr, mlir::ValueRange{index});
  builder->create<mlir::LLVM::StoreOp>(loc, val, elementGep);


  builder->restoreInsertionPoint(ipStack.top());
  ipStack.pop();

}

mlir::Value BackEnd::matrixIndexGep(mlir::Value matrix, int r, int c, std::shared_ptr<Type> &type){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Get the matrix type and space to alloc
  auto typeCheck = std::dynamic_pointer_cast<MatrixType>(type);
  mlir::Type matrixType;
  if (auto it = typeInfo.find(typeCheck->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    matrixType = mType;
  }

  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int64Ty = builder->getI64Type();
  auto int32Ty = builder->getI32Type();

  // Constants
  auto c1i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 1);
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);

  // Get the matrix pointer
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i64});
  auto matrixPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, matrixGep);

  // Get the number of cols
  auto cols = emitGetCols(matrix);

  // Calculate the index
  auto row = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, r);
  auto col = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, c);

  mlir::Value index = builder->create<mlir::LLVM::MulOp>(loc, row, cols);
  index = builder->create<mlir::LLVM::AddOp>(loc, index, col);

  // Return the gep
  return builder->create<mlir::LLVM::GEPOp>(loc, pointerType, matrixType, matrixPtr, mlir::ValueRange{index});
}




mlir::Value BackEnd::matrixIndexGepMLIR(mlir::Value matrix, mlir::Value row, mlir::Value column, std::shared_ptr<Type> &type){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Get the matrix type and space to alloc
  auto typeCheck = std::dynamic_pointer_cast<MatrixType>(type);
  mlir::Type matrixType;
  if (auto it = typeInfo.find(typeCheck->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    matrixType = mType;
  }

  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int64Ty = builder->getI64Type();
  auto int32Ty = builder->getI32Type();

  // Constants
  auto c1i64 = builder->create<mlir::LLVM::ConstantOp>(loc, int64Ty, 1);
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);

  // Get the matrix pointer
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i64});
  auto matrixPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, matrixGep);

  // Get the number of cols
  auto cols = emitGetCols(matrix);

  // Calculate the index
  //auto row = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, r);
  //auto col = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, c);

  mlir::Value index = builder->create<mlir::LLVM::MulOp>(loc, row, cols);
  index = builder->create<mlir::LLVM::AddOp>(loc, index, column);

  // Return the gep
  return builder->create<mlir::LLVM::GEPOp>(loc, pointerType, matrixType, matrixPtr, mlir::ValueRange{index});
}






void BackEnd::setMatrixIndexValueMLIR(mlir::Value val, mlir::Value matrix, mlir::Value row, mlir::Value column, std::shared_ptr<Type> &type){
  auto typeCheck = std::dynamic_pointer_cast<MatrixType>(type);
  if(typeCheck->getBaseType() == Type::BaseType::real){
    if (isIntegerType(val)){
      val = builder->create<mlir::arith::SIToFPOp>(loc, builder->getF32Type(), val);
    }
  }

  auto gep = matrixIndexGepMLIR(matrix, row, column, type);
  builder->create<mlir::LLVM::StoreOp>(loc, val, gep);
}















void BackEnd::setMatrixIndexValue(mlir::Value val, mlir::Value matrix, int r, int c, std::shared_ptr<Type> &type){
  auto typeCheck = std::dynamic_pointer_cast<MatrixType>(type);
  if(typeCheck->getBaseType() == Type::BaseType::real){
    if (isIntegerType(val)){
      val = builder->create<mlir::arith::SIToFPOp>(loc, builder->getF32Type(), val);
    }
  }

  // Types
  auto int32Ty = builder->getI32Type();

  // Constants
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);


  auto rows = emitGetRows(matrix);
  auto cols = emitGetCols(matrix);

  auto row = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, r);
  auto col = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, c);

  // if row < 0 or row >= rows or col < 0 or col > cols raise size error
  auto cmp11 = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, row, c0i32);
  auto cmp12 =  builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sge, row, rows);
  auto cmp21 = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, col, c0i32);
  auto cmp22 = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sge, col, cols);
  auto cmp1 = builder->create<mlir::LLVM::OrOp>(loc, cmp11, cmp12);
  auto cmp2 = builder->create<mlir::LLVM::OrOp>(loc, cmp21, cmp22);
  auto cmp = builder->create<mlir::LLVM::OrOp>(loc, cmp1, cmp2);

  builder->create<mlir::scf::IfOp>(
    loc,
    cmp,
    [&](mlir::OpBuilder &b, mlir::Location bloc){
      auto throwSizeError = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("throwMatrixIndexError");
      builder->create<mlir::LLVM::CallOp>(loc, throwSizeError, mlir::ValueRange{});
      b.create<mlir::scf::YieldOp>(bloc);
    });



  auto gep = matrixIndexGep(matrix, r, c, type);
  builder->create<mlir::LLVM::StoreOp>(loc, val, gep);
}

mlir::Value BackEnd::getMatrixIndexValue(mlir::Value matrix, int r, int c, std::shared_ptr<Type> &type){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Get the matrix type and space to alloc
  auto typeCheck = std::dynamic_pointer_cast<MatrixType>(type);
  mlir::Type matrixType;
  if (auto it = typeInfo.find(typeCheck->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    matrixType = mType;
  }

  // Types
  auto int32Ty = builder->getI32Type();

  // Constants
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);

  auto rows = emitGetRows(matrix);
  auto cols = emitGetCols(matrix);

  auto row = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, r);
  auto col = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, c);

  // if row < 0 or row >= rows or col < 0 or col > cols raise index error
  auto cmp11 = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, row, c0i32);
  auto cmp12 =  builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sge, row, rows);
  auto cmp21 = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::slt, col, c0i32);
  auto cmp22 = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sge, col, cols);
  auto cmp1 = builder->create<mlir::LLVM::OrOp>(loc, cmp11, cmp12);
  auto cmp2 = builder->create<mlir::LLVM::OrOp>(loc, cmp21, cmp22);
  auto cmp = builder->create<mlir::LLVM::OrOp>(loc, cmp1, cmp2);

  builder->create<mlir::scf::IfOp>(
    loc,
    cmp,
    [&](mlir::OpBuilder &b, mlir::Location bloc){
      auto throwSizeError = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("throwMatrixIndexError");
      builder->create<mlir::LLVM::CallOp>(loc, throwSizeError, mlir::ValueRange{});
      b.create<mlir::scf::YieldOp>(bloc);
    });

  auto gep = matrixIndexGep(matrix, r, c, type);
  return builder->create<mlir::LLVM::LoadOp>(loc, matrixType, gep);
}

void BackEnd::copyVecToRow(mlir::Value matrix, mlir::Value vecAddr ,int r, std::shared_ptr<Type>& type){
  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int64Ty = builder->getI64Type();
  auto int32Ty = builder->getI32Type();

  // Constants
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);

  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Get the matrix type and space to alloc
  mlir::Type matrixType;
  if (auto it = typeInfo.find(type->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    matrixType = mType;
  }

  // Get the size of the matrix
  mlir::ArrayRef<mlir::Type> members = {int64Ty, pointerType};
  auto vectorType = mlir::LLVM::LLVMStructType::getLiteral(&context, members);

  auto sizeGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, vectorType, vecAddr, mlir::ValueRange{c0i32, c0i32});
  mlir::Value size = builder->create<mlir::LLVM::LoadOp>(loc, int64Ty, sizeGep);
  auto trucSize = builder->create<mlir::LLVM::TruncOp>(loc, int32Ty, size);

  // Get the ptr to the row
  auto gep = matrixIndexGep(matrix, r, 0, type);
  auto row = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, r);
  auto cols = emitGetCols(matrix);

  // If size <= cols
  auto cmp = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sle, trucSize, cols);
  builder->create<mlir::scf::IfOp>(
    loc,
    cmp,
    [&](mlir::OpBuilder &b, mlir::Location bloc) {
      // For loop to get the element and store it
      std::stack<mlir::OpBuilder::InsertPoint> ipStack;
      auto colsFor = builder->create<mlir::scf::ForOp>(loc, c0i32, trucSize, c1i32);
      mlir::Value j = colsFor.getInductionVar();
      mlir::Block *colsForBody = colsFor.getBody();
      ipStack.push(builder->saveInsertionPoint());
      builder->setInsertionPointToStart(colsForBody);
      // inside the cols for

      // Get the value from the
      auto val =loadArrVal(vecAddr, j, type);


      // Store the value
      auto elementGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, matrixType, gep, mlir::ValueRange{j});
      builder->create<mlir::LLVM::StoreOp>(loc, val, elementGep);


      builder->restoreInsertionPoint(ipStack.top());
      ipStack.pop();
      b.create<mlir::scf::YieldOp>(bloc);
    },
    [&](mlir::OpBuilder &b, mlir::Location bloc){
      auto throwSizeError = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("throwMatrixSizeError");
      builder->create<mlir::LLVM::CallOp>(loc, throwSizeError, mlir::ValueRange{});
      b.create<mlir::scf::YieldOp>(bloc);
    });

  // Free the vector
  freeVector(vecAddr);

}

void BackEnd::UpCastIToFMat(mlir::Value resultant, mlir::Value matrix){
  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int32Ty = builder->getI32Type();
  auto float32Ty = builder->getF32Type();

  // Constants
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);

  // Get rows and cols from the input matrix
  auto rows = emitGetRows(matrix);
  auto cols = emitGetCols(matrix);

  // Compute size = rows * cols
  mlir::Value size = builder->create<mlir::arith::MulIOp>(loc, rows, cols);

  // Get pointers to the data arrays of both input and resultant matrices
  // Input matrix pointer
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i32});
  auto matrixPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, matrixGep);

  // Resultant matrix pointer
  auto resGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, resultant, mlir::ValueRange{c1i32});
  auto resPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, resGep);

  // Create a loop over all elements
  std::stack<mlir::OpBuilder::InsertPoint> ipStack;
  auto forLoop = builder->create<mlir::scf::ForOp>(loc, c0i32, size, c1i32);
  mlir::Value i = forLoop.getInductionVar();
  mlir::Block *forBody = forLoop.getBody();
  ipStack.push(builder->saveInsertionPoint());
  builder->setInsertionPointToStart(forBody);

  // Load the integer value from the original matrix
  auto elementGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, int32Ty, matrixPtr, mlir::ValueRange{i});
  auto val = builder->create<mlir::LLVM::LoadOp>(loc, int32Ty, elementGep);

  // Cast the integer to float
  auto fVal = builder->create<mlir::arith::SIToFPOp>(loc, float32Ty, val);

  // Store the float value into the resultant matrix
  auto resElementGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, float32Ty, resPtr, mlir::ValueRange{i});
  builder->create<mlir::LLVM::StoreOp>(loc, fVal, resElementGep);

  builder->restoreInsertionPoint(ipStack.top());
  ipStack.pop();
}

void BackEnd::UpSizeMatrix(mlir::Value resultant, mlir::Value matrix, std::shared_ptr<Type>& resType){
  static const std::unordered_map<Type::BaseType, std::tuple<int, mlir::Type>> typeInfo = {
    {Type::BaseType::integer,   {4, builder->getI32Type()}},
    {Type::BaseType::real,      {4, builder->getF32Type()}},
    {Type::BaseType::character, {1, builder->getI8Type()}},
    {Type::BaseType::boolean,   {1, builder->getI1Type()}}
  };

  // Get the matrix type and space to alloc
  mlir::Type matrixType;
  if (auto it = typeInfo.find(resType->getBaseType());it != typeInfo.end()){
    auto [mSpace, mType] = it->second;
    matrixType = mType;
  }

  // Types
  auto pointerType = mlir::LLVM::LLVMPointerType::get(&context);
  auto int32Ty = builder->getI32Type();
  auto float32Ty = builder->getF32Type();

  // Constants
  auto c0i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 0);
  auto c1i32 = builder->create<mlir::LLVM::ConstantOp>(loc, int32Ty, 1);

  // Get rows and cols from the input matrix
  auto iRows = emitGetRows(matrix);
  auto iCols = emitGetCols(matrix);

  // Get the rows and cols from the result matrix
  auto rRows = emitGetRows(resultant);
  auto rCols = emitGetCols(resultant);

  auto irlerr = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sle, iRows, rRows);
  auto iclerc = builder->create<mlir::arith::CmpIOp>(loc, mlir::arith::CmpIPredicate::sle, iCols, rCols);
  auto cmp = builder->create<mlir::LLVM::AndOp>(loc, irlerr, iclerc);

  // Get pointers to the data arrays of both input and resultant matrices
  // Input matrix pointer
  auto matrixGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, matrix, mlir::ValueRange{c1i32});
  auto matrixPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, matrixGep);

  // Resultant matrix pointer
  auto resGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, pointerType, resultant, mlir::ValueRange{c1i32});
  auto resPtr = builder->create<mlir::LLVM::LoadOp>(loc, pointerType, resGep);

  // if iRows <= rRows and iCols <= rCols then populate resultant else throw size error
  builder->create<mlir::scf::IfOp>(
    loc,
    cmp,
    [&](mlir::OpBuilder &b, mlir::Location bloc){
      // for loops to get the elements of the matrix
      std::stack<mlir::OpBuilder::InsertPoint> ipStack;
      auto rowsFor = builder->create<mlir::scf::ForOp>(loc, c0i32, iRows, c1i32);
      mlir::Value i = rowsFor.getInductionVar();
      mlir::Block *rowsForBody = rowsFor.getBody();
      ipStack.push(builder->saveInsertionPoint());
      builder->setInsertionPointToStart(rowsForBody);

      // Inside the rows for

      auto colsFor = builder->create<mlir::scf::ForOp>(loc, c0i32, iCols, c1i32);
      mlir::Value j = colsFor.getInductionVar();
      mlir::Block *colsForBody = colsFor.getBody();
      ipStack.push(builder->saveInsertionPoint());
      builder->setInsertionPointToStart(colsForBody);
      // inside the cols for
      // Calculate the index (i * cols + j)
      mlir::Value oldIndex = builder->create<mlir::LLVM::MulOp>(loc, i, iCols);
      oldIndex = builder->create<mlir::LLVM::AddOp>(loc, oldIndex, j);

      // Load the value
      auto elementGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, matrixType, matrixPtr, mlir::ValueRange{oldIndex});
      auto element = builder->create<mlir::LLVM::LoadOp>(loc, matrixType, elementGep);

      // Store in the resultant
      mlir::Value newIndex = builder->create<mlir::LLVM::MulOp>(loc, i, rCols);
      newIndex = builder->create<mlir::LLVM::AddOp>(loc, newIndex, j);
      auto resGep = builder->create<mlir::LLVM::GEPOp>(loc, pointerType, matrixType, resPtr, mlir::ValueRange{newIndex});
      builder->create<mlir::LLVM::StoreOp>(loc, element, resGep);


      builder->restoreInsertionPoint(ipStack.top());
      ipStack.pop();


      builder->restoreInsertionPoint(ipStack.top());
      ipStack.pop();
      b.create<mlir::scf::YieldOp>(bloc);
    },
    [&](mlir::OpBuilder &b, mlir::Location bloc){
      auto throwSizeError = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("throwMatrixSizeError");
      builder->create<mlir::LLVM::CallOp>(loc, throwSizeError, mlir::ValueRange{});
      b.create<mlir::scf::YieldOp>(bloc);
    });

}

mlir::Value BackEnd::callMatmul(mlir::Value A, mlir::Value B, mlir::Value option){
  auto matmull = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("matrixMultiply");
  return builder->create<mlir::LLVM::CallOp>(loc, matmull, mlir::ValueRange{A,B,option}).getResult();
}

mlir::Value BackEnd::callMatAdd(mlir::Value A, mlir::Value B, mlir::Value option) {
  auto matmull = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("matrixAdd");
  return builder->create<mlir::LLVM::CallOp>(loc, matmull, mlir::ValueRange{A,B,option}).getResult();
}