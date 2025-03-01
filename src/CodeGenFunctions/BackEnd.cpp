#include <assert.h>
#include "BackEnd.h"
#include "iostream"
#include "llvm/IR/Intrinsics.h"
#include "mlir/Dialect/LLVMIR/FunctionCallUtils.h"

BackEnd::BackEnd() : loc(mlir::UnknownLoc::get(&context)) {
    // Load Dialects.
    context.loadDialect<mlir::LLVM::LLVMDialect>();
    context.loadDialect<mlir::func::FuncDialect>();
    context.loadDialect<mlir::arith::ArithDialect>();
    context.loadDialect<mlir::scf::SCFDialect>();
    context.loadDialect<mlir::cf::ControlFlowDialect>();
    context.loadDialect<mlir::memref::MemRefDialect>();

    // Initialize the MLIR context
    builder = std::make_shared<mlir::OpBuilder>(&context);
    module = mlir::ModuleOp::create(builder->getUnknownLoc());
    builder->setInsertionPointToStart(module.getBody());


    //Create global constants that I will use a lot


    // Some intial setup to get off the ground
    setupPrintf();
    setupFree();
    setupScanf();
    setupSScanf();
    setupCheckStdin();
    setupFormat();
    setStrLen();
    setUpStackPointerOps();
    setUpMatMul();

    // Set up the instream global stuff
    createInStreamBuffer();
    createStreamStateGlobalInt();

    //Set up runtime error functions
    setUpMathError();
    setUpSizeError();
    // Formats for various stuff
    createGlobalString("%512c%n\0", "bufferStringFormat");
    createGlobalString("%c%n\0", "bufferCharFormat");
    createGlobalString(" %[TF]%n\0", "bufferBoolFormat");
    createGlobalString("%d%n\0", "bufferIntFormat");
    createGlobalString("%g%n\0", "bufferFloatFormat");

    createGlobalString("%s\0", "stringFormat");
    createGlobalString("%c\0", "charFormat");
    createGlobalString("%d\0", "intFormat");
    createGlobalString("%g\0", "floatFormat");
    createGlobalString("[\0", "leftBracket");
    createGlobalString("]\0", "rightBracket");
    createGlobalString(" \0", "whiteSpace");
}


int BackEnd::emitModule() {

    module.dump();

    if (mlir::failed(mlir::verify(module))) {
        module.emitError("module failed to verify");
        return 1;
    }
    return 0;
}


int BackEnd::lowerDialects() {
    // Set up the MLIR pass manager to iteratively lower all the Ops
    mlir::PassManager pm(&context);

    // Lower Func dialect to LLVM
    pm.addPass(mlir::createConvertFuncToLLVMPass());

    // Lower SCF to CF (ControlFlow)
    pm.addPass(mlir::createConvertSCFToCFPass());

    // Lower Arith to LLVM
    pm.addPass(mlir::createArithToLLVMConversionPass());

    // Lower MemRef to LLVM
    pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());

    // Lower CF to LLVM
    pm.addPass(mlir::createConvertControlFlowToLLVMPass());

    // Finalize the conversion to LLVM dialect
    pm.addPass(mlir::createReconcileUnrealizedCastsPass());

    // Run the passes
    if (mlir::failed(pm.run(module))) {
        llvm::errs() << "Pass pipeline failed\n";
        return 1;
    }
    return 0;
}


void BackEnd::dumpLLVM(std::ostream &os) {
    // The only remaining dialects in our module after the passes are builtin
    // and LLVM. Setup translation patterns to get them to LLVM IR.
    mlir::registerBuiltinDialectTranslation(context);
    mlir::registerLLVMDialectTranslation(context);
    llvm_module = mlir::translateModuleToLLVMIR(module, llvm_context);

    // Create llvm ostream and dump into the output file
    llvm::raw_os_ostream output(os);
    output << *llvm_module;
}


void BackEnd::setupPrintf() {
    // Create a function declaration for printf, the signature is:
    //   * `i32 (ptr, ...)`
    const mlir::Type intType = mlir::IntegerType::get(&context, 32);
    const auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(intType, ptrTy,
                                                        /*isVarArg=*/true);

    // Insert the printf function into the body of the parent module.
    builder->create<mlir::LLVM::LLVMFuncOp>(loc, "printf", llvmFnType);
}


void BackEnd::setupFree() {
    // Create a function declaration for free, the signature is:
    //   * `void (ptr)`

    // Get the context and integer type
    mlir::Type voidType = mlir::LLVM::LLVMVoidType::get(&context);

    // Create a pointer type for the argument (void*)
    auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context); // void* is typically a pointer to i8

    // Define the function type for free, which takes a pointer and returns void
    auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(voidType, ptrTy, /*isVarArg=*/false);

    // Insert the free function into the body of the parent module.
    builder->create<mlir::LLVM::LLVMFuncOp>(loc, "free", llvmFnType);
}


void BackEnd::setupScanf() {
    const mlir::Type intType = mlir::IntegerType::get( &context, 32 );
    const auto ptrTy = mlir::LLVM::LLVMPointerType::get( &context );
    auto llvmFnType = mlir::LLVM::LLVMFunctionType::get( intType, ptrTy,
                                                         /*isVarArg=*/true );
    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "scanf",llvmFnType );
}

void BackEnd::setupFormat() {
    const mlir::Type intType = mlir::IntegerType::get(&context, 32);
    const auto ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
    auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(intType,{ptrTy, ptrTy}, /*isVarArg=*/true);

    // Insert the sprintf function into the body of the parent module.
    builder->create<mlir::LLVM::LLVMFuncOp>(loc, "sprintf", llvmFnType);
}



void BackEnd::createGlobalString(const char *str, const char *stringName) {

    mlir::Type charType = mlir::IntegerType::get(&context, 8);

    // create string and string type
    auto mlirString = mlir::StringRef(str, strlen(str) + 1);
    auto mlirStringType = mlir::LLVM::LLVMArrayType::get(charType, mlirString.size());

    builder->create<mlir::LLVM::GlobalOp>(loc, mlirStringType, /*isConstant=*/true,
                            mlir::LLVM::Linkage::Internal, stringName,
                            builder->getStringAttr(mlirString), /*alignment=*/0);
}


mlir::Value BackEnd::createGlobalInteger(const std::string& integerName, mlir::Type int_type, mlir::LLVM::ConstantOp zero_const) {
    // Create an MLIR global of value zero, we will modify its value later at runtime
    std::string global_int_label = "g_" + integerName;
    auto int_attr = zero_const.getValue().cast<mlir::IntegerAttr>();
    auto global_op = builder->create<mlir::LLVM::GlobalOp>(loc, int_type, /*isConstant=*/false,
                                          mlir::LLVM::Linkage::Internal, global_int_label,
                                          int_attr, /*alignment=*/0);

    // set the insertion point to the global_decl_block
    builder->setInsertionPointToEnd(global_decl_block);

    // get the address of the global so that we can store it in the symbol table
    return builder->create<mlir::LLVM::AddressOfOp>(loc, global_op);
}


mlir::Value BackEnd::createGlobalFloat(const std::string &floatName) {
    // Create an MLIR constant of the given integer value
    auto float_type = builder->getF32Type();
    std::string global_int_label = "g_" + floatName;
    auto zero_const = builder->create<mlir::LLVM::ConstantOp>(loc, float_type, builder->getFloatAttr(float_type,0.0f));
    auto float_attr = zero_const.getValue().cast<mlir::FloatAttr>();

    auto global_op = builder->create<mlir::LLVM::GlobalOp>(loc, float_type, /*isConstant=*/false,
                                          mlir::LLVM::Linkage::Internal, global_int_label,
                                          float_attr, /*alignment=*/0);

    // set the insertion point to the global_decl_block
    builder->setInsertionPointToEnd(global_decl_block);

    // get the address of the global so that we can store it in the symbol table
    return builder->create<mlir::LLVM::AddressOfOp>(loc, global_op);
}






mlir::Value BackEnd::createGlobalptr(const std::string& ptrName) {
    // Create an MLIR global of value zero, we will modify its value later at runtime
    std::string global_label = "g_" + ptrName;
    mlir::Type ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    auto global_op = builder->create<mlir::LLVM::GlobalOp>(loc, ptr_type, /*isConstant=*/false,
                                      mlir::LLVM::Linkage::Internal, global_label,
                                     mlir::Attribute({}), /*alignment=*/0);

    // set the insertion point to the global_decl_block
    builder->setInsertionPointToEnd(global_decl_block);

    // get the address of the global so that we can store it in the symbol table
    return builder->create<mlir::LLVM::AddressOfOp>(loc, global_op);
}


//Set up runtime error functions

void BackEnd::setUpMathError() {
    mlir::Type int32_type = builder->getI32Type();
    mlir::Type float32_type = builder->getF32Type();
    mlir::Type void_type = mlir::LLVM::LLVMVoidType::get(&context);
    mlir::LLVM::LLVMFunctionType int_div_zero = mlir::LLVM::LLVMFunctionType::get(void_type, int32_type);
    mlir::LLVM::LLVMFunctionType float_div_zero = mlir::LLVM::LLVMFunctionType::get(void_type, float32_type);


    mlir::LLVM::LLVMFunctionType int_exp_zero = mlir::LLVM::LLVMFunctionType::get(void_type, {int32_type, int32_type});
    mlir::LLVM::LLVMFunctionType float_exp_zero = mlir::LLVM::LLVMFunctionType::get(void_type, {float32_type, float32_type});

    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "intDivZero",int_div_zero);
    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "floatDivZero",float_div_zero);


    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "intZeroExp",int_exp_zero);
    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "floatZeroExp",float_exp_zero);

}

void BackEnd::setStrLen() {
    mlir::Type int32_type = builder->getI32Type();
    const auto ptrTy = mlir::LLVM::LLVMPointerType::get( &context );
    mlir::LLVM::LLVMFunctionType strlen = mlir::LLVM::LLVMFunctionType::get(int32_type, {ptrTy},
        /*isVarArg=*/true);
    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "strlen",strlen);

}

void BackEnd::setUpSizeError() {
    mlir::Type int32_type = builder->getI32Type();
    mlir::Type float32_type = builder->getF32Type();
    mlir::Type void_type = mlir::LLVM::LLVMVoidType::get(&context);


    mlir::LLVM::LLVMFunctionType vec_size_error = mlir::LLVM::LLVMFunctionType::get(void_type, {int32_type, int32_type});
    mlir::LLVM::LLVMFunctionType mat_size_error = mlir::LLVM::LLVMFunctionType::get(void_type, {int32_type, int32_type,int32_type, int32_type});
    mlir::LLVM::LLVMFunctionType empty = mlir::LLVM::LLVMFunctionType::get(void_type, {});

    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "vectorSizeCheck",vec_size_error);
    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "declVectorSizeCheck",vec_size_error);
    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "matrixSizeCheck",mat_size_error);
    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "matrixDeclSizeCheck",mat_size_error);
    builder->create<mlir::LLVM::LLVMFuncOp>(loc, "throwMatrixIndexError", empty);
    builder->create<mlir::LLVM::LLVMFuncOp>(loc, "throwMatrixSizeError", empty);

    builder->create< mlir::LLVM::LLVMFuncOp >( loc, "vectorIndexCheck",vec_size_error);


}




void BackEnd::setUpStackPointerOps() {

    auto save_type = mlir::LLVM::LLVMPointerType::get(&context);
    auto stack_save_type = mlir::LLVM::LLVMFunctionType::get(save_type, {});
    auto stackSaveFunc = builder->create<mlir::LLVM::LLVMFuncOp>(
        module.getLoc(), "llvm.stacksave.p0", stack_save_type);

    auto restore_type = mlir::LLVM::LLVMFunctionType::get(mlir::LLVM::LLVMVoidType::get(&context), {save_type});
    auto stack_restore = builder->create<mlir::LLVM::LLVMFuncOp>(
    module.getLoc(), "llvm.stackrestore.p0", restore_type);

}

void BackEnd::setupSScanf() {
  const mlir::Type intType = mlir::IntegerType::get( &context, 32 );
  const auto ptrTy = mlir::LLVM::LLVMPointerType::get( &context );
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get( intType, {ptrTy, ptrTy},
    /*isVarArg=*/true );
  builder->create< mlir::LLVM::LLVMFuncOp >( loc, "sscanf",llvmFnType );
}

void BackEnd::createInStreamBuffer(){
  mlir::Type charType = mlir::IntegerType::get(&context, 8);

  // Buffer string name
  std::string buff = "instream_buffer_512_string";

  // Create a buffer
  std::string buffer(512, ' ');
  char* str = &buffer[0];
  auto mlirString = mlir::StringRef(str, strlen(str) + 1);
  auto mlirStringType = mlir::LLVM::LLVMArrayType::get(charType, mlirString.size());

  builder->create<mlir::LLVM::GlobalOp>(loc, mlirStringType, /*isConstant=*/false,
                                        mlir::LLVM::Linkage::Internal, buff,
                                        builder->getStringAttr(mlirString), /*alignment=*/0);

  // Pointer to the buffer
  buff = "instream_buffer_512";
  mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
  builder->create<mlir::LLVM::GlobalOp>(loc, ptrTy, /*isConstant=*/false,
                                                         mlir::LLVM::Linkage::Internal, buff,
                                                         mlir::Attribute({}), /*alignment=*/0);

  // left in buffer
  buff = "instream_buffer_512_len";
  auto intType = builder->getI32Type();
  builder->create<mlir::LLVM::GlobalOp>(loc, intType, /*isConstant=*/false,
                                        mlir::LLVM::Linkage::Internal, buff,
                                        builder->getIntegerAttr(intType, 0), /*alignment=*/0);
}

void BackEnd::createStreamStateGlobalInt() {
    const char *streamState = "stream_state_g_int";

    const auto intType = builder->getI32Type();

    builder->create<mlir::LLVM::GlobalOp>(loc, intType, /*isConstant=*/false,
                                          mlir::LLVM::Linkage::Internal, streamState,
                                          builder->getIntegerAttr(intType, 0), /*alignment=*/0);
}

void BackEnd::initInStreamBufferInMain(){
  auto buffArray = module.lookupSymbol<mlir::LLVM::GlobalOp>("instream_buffer_512_string");
  auto buffArrayAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, buffArray);
  auto buffer = module.lookupSymbol<mlir::LLVM::GlobalOp>("instream_buffer_512");
  auto bufferAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, buffer);
  builder->create<mlir::LLVM::StoreOp>(loc, buffArrayAddr, bufferAddr);
}

void BackEnd::setupCheckStdin(){
  mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
  mlir::Type voidType = mlir::LLVM::LLVMVoidType::get(&context);
  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(voidType, ptrTy, /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, "checkStdin", llvmFnType);
}

void BackEnd::setUpMatMul(){
  mlir::Type ptrTy = mlir::LLVM::LLVMPointerType::get(&context);
  mlir::Type charTy = builder->getI8Type();

  auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(ptrTy, {ptrTy, ptrTy, charTy}, /*isVarArg=*/false);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, "matrixMultiply", llvmFnType);
  builder->create<mlir::LLVM::LLVMFuncOp>(loc, "matrixAdd", llvmFnType);
}


