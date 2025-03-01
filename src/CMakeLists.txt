# Gather our source files in this directory.
set(
  gazprea_src_files
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/BackEnd.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SymbolTableClasses/Type.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AstBuildingPass/AST.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ExampleASTWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AstBuildingPass/ASTBuilder.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AstBuildingPass/ASTXMLWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/AstBuildingPass/DefineSymbols.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SymbolTableClasses/SymbolTable.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SymbolTableClasses/Symbol.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SymbolTableClasses/Scope.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SymbolTableClasses/ScopedSymbol.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/AddFreeNodesWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/AliasingErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ArgumentMutabilityErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ConstantFoldingWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/DefinitionErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/MainErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ConstAssignErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/GlobalErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/DeclStatementErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/TypeDefStatementErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/GlobalStatementErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/BreakContinueStatementErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/StdInOutStatementErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/CallFuncCallErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/SizeErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/SyntaxErrorListener.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ProcReturnCallErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/TupleAssignErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ControlExpressionsWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ReturnErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/TypeInferWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ValidExprErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/FuncProcDefContainsParamIDsWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/FuncProcCallWithSameNumberOfParamsWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/BinaryOpTypeErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ControlFlowReturnErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/BreakContinuePruneWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/TupleAccessErrorWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/validationPasses/ScopeToFreeWalker.cpp"

        # CodeGenFunctions
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenPass/CodeGenASTWalker.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/EmitFunctions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/EmitBinaryOps.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/TypeCheckFunctions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/StreamFunctions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/TypeCastingPromotionFunctions.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/EmitTuple.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/EmitVectors.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CodeGenFunctions/EmitMatrix.cpp"

)

# Build our executable from the source files.
add_executable(gazc ${gazprea_src_files})
target_include_directories(gazc PUBLIC ${ANTLR_GEN_DIR})

# Ensure that the antlr4-runtime is available.
add_dependencies(gazc antlr)

# Find the libraries that correspond to the LLVM components
# that we wish to use
set(LLVM_LINK_COMPONENTS Core Support)
llvm_map_components_to_libnames(llvm_libs core)
get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)

# Add the MLIR, LLVM, antlr runtime and parser as libraries to link.
target_link_libraries(gazc PRIVATE
    parser
    antlr4-runtime
    ${llvm_libs}
    ${dialect_libs}
    )

# Symbolic link our executable to the base directory so we don't have to go searching for it.
symlink_to_bin("gazc")
