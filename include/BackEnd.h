#ifndef BACKEND_H
#define BACKEND_H
// Pass manager
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVMPass.h"

// Translation
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "llvm/Support/raw_os_ostream.h"

// MLIR IR
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/IR/Verifier.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/BuiltinOps.h"

// Dialects
#include <AST.h>

#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"

class CodeGenASTWalker;

class BackEnd {
 public:
    BackEnd();
    int emitModule();
    int lowerDialects();
    void dumpLLVM(std::ostream &os);

    mlir::Block* global_decl_block = new mlir::Block();
    void insertGlobalDeclBlock();

    mlir::Block* global_free_block = new mlir::Block();
    void insertGlobalFreeBlock();

    // tuple(back_edge, forward_edge, parent loop's starting stack pointer, loop's domain vector)
    std::vector<std::tuple<mlir::Block*, mlir::Block*, mlir::Value, mlir::Value>> nested_blocks_stack;

    // the last continue block from the previous inner scope (used for merging blocks)
    mlir::Block* last_inner_continue_block = nullptr;

    [[nodiscard]] mlir::Type getMLIRTypeFromTypeObject(const std::shared_ptr<Type> &type);
    [[nodiscard]] mlir::Type getMLIRTypeFromBaseType(const std::shared_ptr<Type> &type);

    //Emit functions
    [[nodiscard]] mlir::Value emitInt(int val) const;
    [[nodiscard]] mlir::Value emitReal(float val) const;
    [[nodiscard]] mlir::Value emitBool(bool val) const;
    [[nodiscard]] mlir::Value emitChar(char val) const;

    mlir::Value emitIterLoopIndex();
    mlir::Value emitIncrementIndex(mlir::Value index);
    mlir::Value emitLoadIndex(mlir::Value index_ptr);
    void emitStoreIndex(mlir::Value index_ptr, mlir::Value new_value);
    mlir::Value emitDomainVariable(mlir::Type type);
    void emitStoreDomainVariable(mlir::Value dv_pointer, mlir::Value new_value);

    void emitUnreachableOp();

    mlir::Value emitLessThan(mlir::Value left_side, mlir::Value right_side);
    mlir::Value intLessThan(mlir::Value left_side, mlir::Value right_side); //This ones should probably be protected
    mlir::Value realLessThan(mlir::Value left_side, mlir::Value right_side);

    mlir::Value emitGreaterThan(mlir::Value left_side, mlir::Value right_side);
    mlir::Value intGreaterThan(mlir::Value left_side, mlir::Value right_side); //This ones should probably be protected
    mlir::Value realGreaterThan(mlir::Value left_side, mlir::Value right_side);

    //Greater than or equal to operations
    mlir::Value emitGreaterEq(mlir::Value left_side, mlir::Value right_side);
    mlir::Value intGreaterEq(mlir::Value left_side, mlir::Value right_side); //This ones should probably be protected
    mlir::Value realGreaterEq(mlir::Value left_side, mlir::Value right_side);

    //Less than or equal to operations
    mlir::Value emitLessEq(mlir::Value left_side, mlir::Value right_side);
    mlir::Value intLessEq(mlir::Value left_side, mlir::Value right_side); //This ones should probably be protected
    mlir::Value realLessEq(mlir::Value left_side, mlir::Value right_side);

    mlir::Value intAdd(mlir::Value value, mlir::Value right_side);

    //Binary Integer operation:
    mlir::Value emitAdd(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitExpo(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitRem(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitDiv(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitMul(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitSub(mlir::Value left_side, mlir::Value right_side);

    //Equal to operations...
    mlir::Value emitEqualTo(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitEqualTo(const std::vector<mlir::Value> &left_side, const std::vector<mlir::Value> &right_side);
    mlir::Value emitEqualTo(const std::vector<mlir::Value> &left_side, const std::vector<mlir::Value> &right_side,
        const std::shared_ptr<TupleType> &leftTupleType,const std::shared_ptr<TupleType> &rightTupleType);
    mlir::Value intEqualTo(mlir::Value left_side, mlir::Value right_side); //This ones should probably be protected
    mlir::Value realEqualTo(mlir::Value left_side, mlir::Value right_side);
    mlir::Value boolEqualTo(mlir::Value left_side, mlir::Value right_side);

    //Not equal to operations...
    mlir::Value emitNotEqual(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitNotEqual(const std::vector<mlir::Value> &left_side, const std::vector<mlir::Value> &right_side);
    mlir::Value intNotEqual(mlir::Value left_side, mlir::Value right_side); //This ones should probably be protected
    mlir::Value realNotEqual(mlir::Value left_side, mlir::Value right_side);
    mlir::Value boolNotEqual(mlir::Value left_side, mlir::Value right_side);

    //emit logical operations
    mlir::Value emitAnd(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitOr(mlir::Value left_side, mlir::Value right_side);
    mlir::Value emitXor(mlir::Value left_side, mlir::Value right_side);

    mlir::Value emitUnaryNot(mlir::Value value);
    mlir::Value emitUnaryMinus(mlir::Value value);

    // Tuple Functions
    // mlir::Value emitCreateTuple(std::vector<std::shared_ptr<ASTNode>> tuple);
    //void emitCreateTuple(const std::shared_ptr<TupleType> &type, const std::shared_ptr<Symbol> &val,std::shared_ptr<ASTNode> tuple);
    //void emitCreateTuple(const std::shared_ptr<TupleType> &type, const std::shared_ptr<Symbol>& symbol);
    void emitCreateTuple(const std::shared_ptr<TupleType> &type, const std::shared_ptr<Symbol> &symbol, CodeGenASTWalker &walker);
    mlir::Value emitCreateTuple(const std::shared_ptr<TupleType> &type, const std::vector<mlir::Value> &tupleVec);
    mlir::Value emitCastTuple(const std::shared_ptr<TupleType> &toType, const std::shared_ptr<TupleType> &fromType,
                                       mlir::Value fromStructPtr, CodeGenASTWalker &walker);
    mlir::Value emitTupleAccess(const std::shared_ptr<Symbol> &symbol, int pos);
    mlir::Value emitTupleEqNeq(mlir::Value leftStructPtr, const std::shared_ptr<TupleType> &leftTupleType, mlir::Value rightStructPtr, const std::
                               shared_ptr<TupleType> &rightTupleType, const std::string &op);

    std::vector<mlir::Value> tupleValues(const std::shared_ptr<Symbol> &symbol);

    std::vector<mlir::Value> tupleValues(mlir::Value structPtr, const std::shared_ptr<TupleType> &tupleType);
    void emitTupAssign(const std::shared_ptr<Symbol> &symbol, int pos, mlir::Value value);

    mlir::Value getTupleElemPointer(const std::shared_ptr<Symbol> &symbol, int pos);

    //Insertion point operations...
    mlir::OpBuilder::InsertPoint saveInsertionPoint();
    void restoreInsertionPoint(mlir::OpBuilder::InsertPoint insert_point);
    void setInsertionPointToStart(mlir::Block *block);
    void setInsertionPointToEnd(mlir::Block *block);
    void setInsertionPointBeforeOp(mlir::Operation* op);
    void setInsertionPointToModuleEnd();
    mlir::Block::iterator getInsertionPoint();
    mlir::Block * createBlock();
    mlir::Block * getInsertionBlock();

    //Conditional operations
    bool hasTerminator(mlir::Block *block);
    bool isTerminator(mlir::Operation *op);
    void emitCondBranch(mlir::Value condition, mlir::Block *if_block, mlir::Block *else_block);
    void emitBranch(mlir::Block * block);
    void emitDec(const std::shared_ptr<Symbol> &symbol);
    void emitDec(const std::shared_ptr<Symbol> &symbol, mlir::Value val);
    void emitDec(const std::shared_ptr<Type> & type, const std::shared_ptr<Symbol> & val, mlir::Value value);
    void emitDecGlobal(const std::shared_ptr<Symbol> &symbol, mlir::Value global_addr, mlir::Value val);
    mlir::Value declareGlobal(const std::shared_ptr<Symbol> &symbol, std::string &id);
    //void declareGlobal(const std::shared_ptr<Symbol> &symbol, mlir::Value value, std::string &id);
    void emitAssignId(std::shared_ptr<Symbol> &symbol, mlir::Value value, int line);
    mlir::Value emitLoadID(const std::shared_ptr<Symbol>& symbol);
    mlir::Value emitLoadID(const std::shared_ptr<Symbol> &symbol, const std::string& id);

    void emitFuncDefStart(const std::shared_ptr<Symbol> &symbol, bool is_prototype);
    void emitFuncDefEnd();

    void emitProcDefStart(const std::shared_ptr<Symbol> &symbol, bool is_prototype);
    void emitProcDefEnd();

    mlir::Value emitFuncProcCall(std::shared_ptr<Symbol> &symbol, std::vector<mlir::Value> &evaluated_args);
    mlir::func::ReturnOp emitReturn(mlir::Value return_val);

    //Stream Functions
    void outStream(mlir::Value val);
    mlir::Value inStream(const std::shared_ptr<Type> &type);
    void printChar(mlir::Value character);
    void printString(mlir::Value string);
    void printFloat(mlir::Value val);
    void printBool(mlir::Value val);
    void printInt(mlir::Value val);

    // stream state
    void storeStreamState(mlir::Value state);
    mlir::Value loadStreamState();

    bool isSameType(mlir::Value value1, mlir::Value value2);
    static bool isSameType(mlir::Value value1, const std::shared_ptr<Type> &type);

    //Type Checking functions
    static bool isIntegerType(mlir::Value value);
    static bool isFloatType(mlir::Value value);
    static bool isBoolType(mlir::Value value);
    static bool isCharType(mlir::Value value);
    static bool isStructType(mlir::Value value);

    //Type Promotion
    [[nodiscard]] mlir::Value typeCast(mlir::Value value, Type::BaseType toType) const;
    [[nodiscard]] mlir::Value castInt(mlir::Value value, Type::BaseType toType) const;
    [[nodiscard]] mlir::Value castChar(mlir::Value value,Type::BaseType toType) const;
    [[nodiscard]] mlir::Value castFloat(mlir::Value value,Type::BaseType toType) const;
    [[nodiscard]] mlir::Value castBool(mlir::Value value, Type::BaseType toType) const;

    //Restore and save stack pointer
    mlir::Value emitCurrentStackPointer();
    void emitRestoreStackPointer(mlir::Value stack_pointer);

    mlir::Value loadArrVal(mlir::Value vector_addr, mlir::Value index, std::shared_ptr<Type> &type);
    void storeArrVal(mlir::Value vector_addr, mlir::Value index, mlir::Value value , std::shared_ptr<Type> &type);


    //Vectors
    mlir::Value createVector(mlir::Value size, std::shared_ptr<Type> type);
    mlir::Value emitVectorLiteral(std::vector<mlir::Value> values, mlir::Value size , std::shared_ptr<Type> type);
    mlir::Value getArrSize(mlir::Value vector_addr, bool truncated = true);
    void emitAssignVector(std::shared_ptr<Symbol>& symbol, mlir::Value value);
    mlir::Value cloneVector(mlir::Value vector_addr, std::shared_ptr<Type> &type);
    mlir::Value getVector(mlir::Value vector_addr);
    void printVector(mlir::Value vector, std::shared_ptr<Type> &type);
    bool isVectorType(const mlir::Value value);
    mlir::Value emitVectorDec(mlir::Value vector);
    void freeVector(mlir::Value vector_addr);
    mlir::Value reverseVector(mlir::Value original_vector_addr, std::shared_ptr<Type> type);
    mlir::Value emitVectorConcat(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Type getVectorType(mlir::Value vector_addr);
    mlir::Value loadVectorId(const std::shared_ptr<Symbol>& symbol);
    mlir::Value loadVectorId(const std::shared_ptr<Symbol>& symbol, const std::string& id);

    mlir::Value emitRangeVector(mlir::Value upper_bound, mlir::Value lower_bound);

    void transferVecElements(mlir::Value from_vector, mlir::Value to_vector, std::shared_ptr<Type> &to_type);

    //Vector type promotion stuff
    mlir::Value scalarToVector(mlir::Value scalar, mlir::Value arr_size, std::shared_ptr<Type> &type);
    mlir::Value promoteVectorTo(mlir::Value vector_addr, std::shared_ptr<Type> &old_type, std::shared_ptr<Type> &new_type);
    //Vector arithmetic ops
    mlir::Value emitVectorAdd(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorSub(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorMul(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorDiv(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorRem(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorExp(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorDot(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);

    //Vector logical operations
    mlir::Value emitVectorAnd(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorOr(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorXor(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    //Vector unary ops
    mlir::Value emitVectorNot(mlir::Value value, std::shared_ptr<Type> &type);
    mlir::Value emitVectorUnaryMinus(mlir::Value value, std::shared_ptr<Type> &type);

    //Generators and filters
    mlir::Value createVectorGenerator(mlir::Value vector_addr, mlir::Block * expr_block,
        std::shared_ptr<Symbol> id, mlir::Value result, std::shared_ptr<Type> &result_type,
        std::shared_ptr<Type> &dom1_type, int line);
    mlir::Value createMatrixGenerator(mlir::Value vector_addr1, mlir::Value vector_addr2, mlir::Block * expr_block,
        std::shared_ptr<Symbol> id1, std::shared_ptr<Symbol> id2, mlir::Value result,
        std::shared_ptr<Type> &result_type, std::shared_ptr<Type> &dom1_type,
        std::shared_ptr<Type> &dom2_type, int line);

    mlir::Value createFilter(mlir::Value vector_address, mlir::Block *predicate_block, std::shared_ptr<Symbol> id,
    mlir::Value result, std::shared_ptr<Type> &domain_type);



    mlir::Value resizeVector(mlir::Value old_vector, mlir::Value index_ptr, std::shared_ptr<Type> &domain_type);




    mlir::Value emitCastVector(mlir::Value vector_addr,  std::shared_ptr<Type> &from_type,
         std::shared_ptr<Type> &to_type, CodeGenASTWalker &walker);

    void mergeBlocks(mlir::Block *source_block, mlir::Block *target_block, mlir::Block::iterator insertion_point);

    //Vector equality operations
    mlir::Value emitVectorEqualTo(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorNotEqualTo(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    //Vector comparison operations
    mlir::Value emitVectorLessEq(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorGreaterEq(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorLessThan(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    mlir::Value emitVectorGreaterThan(mlir::Value left_side, mlir::Value right_side, std::shared_ptr<Type> &type);
    Type::BaseType promoteBaseType(std::shared_ptr<Type> &left_type, std::shared_ptr<Type> &right_type);

    // temp integer stack allocation
    mlir::Value emitAndStoreInteger(mlir::Value integer);
    mlir::Value emitLoadInteger(mlir::Value integer_ptr);
    void storeIntegerAtPointer(mlir::Value integer_val, mlir::Value integer_ptr);
  
    void checkDeclSizeError(mlir::Value decl_size, mlir::Value vec_size);

    // Matrices
    mlir::Value emitFormatScalar(mlir::Value value);

    // Matrices
    mlir::Value emitInitializedMatrix(mlir::Value initValue ,mlir::Value rows, mlir::Value cols, std::shared_ptr<Type> &type);
    mlir::Value emitZeroInitMatrix(mlir::Value rows, mlir::Value cols, std::shared_ptr<Type> &type);
    mlir::Value emitGetRows(mlir::Value matrix);
    mlir::Value emitGetCols(mlir::Value matrix);
    void emitPrintMatrix(mlir::Value matrix, const std::shared_ptr<Type>& type);
    mlir::Value cloneMatrix(mlir::Value matrix, const std::shared_ptr<Type>& type);
    mlir::Value loadMatrixId(const std::shared_ptr<Symbol>& symbol); // give a copy of a matrix in the local scope
    void freeMatrix(mlir::Value matrix);

    // row and col number (r, c) are zero indexed here if u want [1,1] give it r=0, c=0
    mlir::Value matrixIndexGep(mlir::Value matrix, int r, int c, std::shared_ptr<Type> &type);

    void setRowValues(mlir::Value val, mlir::Value matrix, int r, const std::shared_ptr<Type>& type);
    void copyVecToRow(mlir::Value matrix, mlir::Value vecAddr ,int r, std::shared_ptr<Type>& type);
    void setMatrixIndexValue(mlir::Value val, mlir::Value matrix, int r, int c, std::shared_ptr<Type> &type);
    mlir::Value getMatrixIndexValue(mlir::Value matrix, int r, int c, std::shared_ptr<Type> &type);
    void UpCastIToFMat(mlir::Value resultant, mlir::Value matrix);
    void UpSizeMatrix(mlir::Value resultant, mlir::Value matrix, std::shared_ptr<Type>& resType);

    void setMatrixIndexValueMLIR(mlir::Value val, mlir::Value matrix, mlir::Value row, mlir::Value column, std::shared_ptr<Type> &type);
    mlir::Value matrixIndexGepMLIR(mlir::Value matrix, mlir::Value row, mlir::Value column, std::shared_ptr<Type> &type);

    mlir::Value callMatmul(mlir::Value A, mlir::Value B, mlir::Value option);
    mlir::Value callMatAdd(mlir::Value A, mlir::Value B, mlir::Value option);

    // access individual elements of a matrix, free matrix
    mlir::Value emitVectorStride(mlir::Value vector_addr, mlir::Value stride, std::shared_ptr<Type> &type);

    mlir::Value resizeVectorWithInt(mlir::Value old_vector, mlir::Value index, std::shared_ptr<Type> &domain_type);
protected:
    void setupPrintf();
    void setupFree();
    void setupScanf();
    void setupFormat();
    void setupSScanf();
    void setupCheckStdin();
    void setUpStackPointerOps();
    void setUpMatMul();

    void createGlobalString(const char *str, const char *stringName);
    mlir::Value createGlobalInteger(const std::string &integerName, mlir::Type int_type, mlir::LLVM::ConstantOp zero_const) ;
    mlir::Value createGlobalFloat(const std::string &floatName);
    mlir::Value createGlobalptr(const std::string &ptrName);

    void createInStreamBuffer();
    void initInStreamBufferInMain();
    void createStreamStateGlobalInt();

    void createGlobalStruct(mlir::Value value, const std::string &floatName);
    // void createGlobalInteger(mlir::Value value, const std::string &integerName, mlir::Type intType) ;
    // void createGlobalFloat(mlir::Value value, const std::string &floatName);


    //Set up run time error functions
    void setUpMathError();

    void setStrLen();

    void setUpSizeError();

 private:
    // MLIR
    mlir::MLIRContext context;
    mlir::ModuleOp module;
    std::shared_ptr<mlir::OpBuilder> builder;
    mlir::Location loc;

    // LLVM
    llvm::LLVMContext llvm_context;
    std::unique_ptr<llvm::Module> llvm_module;
};

#endif //BACKEND_H
