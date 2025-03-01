#include "GazpreaLexer.h"
#include "GazpreaParser.h"

#include "ANTLRFileStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTree.h"
#include "tree/ParseTreeWalker.h"

#include "ASTBuilder.h"
#include "ASTWalker.h"
#include "ExampleASTWalker.h"

#include "ASTXMLWalker.h"

#include "CodeGenASTWalker.h"

#include "BackEnd.h"
#include "CompileTimeExceptions.h"
#include "DefineSymbols.h"

#include <fstream>

// Validation passes
#include <ControlFlowReturnErrorWalker.h>
#include "AddFreeNodesWalker.h"
#include "AliasingErrorWalker.h"
#include "ArgumentMutabilityErrorWalker.h"
#include "ConstantFoldingWalker.h"
#include "DefinitionErrorWalker.h"
#include "TypeInferWalker.h"
#include "MainErrorWalker.h"
#include "ConstAssignErrorWalker.h"
#include "GlobalErrorWalker.h"
#include "DeclStatementErrorWalker.h"
#include "TypeDefStatementErrorWalker.h"
#include "BreakContinueStatementErrorWalker.h"
#include "GlobalStatementErrorWalker.h"
#include "StdInOutStatementErrorWalker.h"
#include "CallFuncCallErrorWalker.h"
#include "SizeErrorWalker.h"
#include "SyntaxErrorListener.h"
#include "ProcReturnCallErrorWalker.h"
#include "TupleAssignErrorWalker.h"
#include "ControlExpressionsWalker.h"
#include "ValidExprErrorWalker.h"
#include "FuncProcDefContainsParamIDsWalker.h"
#include "FuncProcCallWithSameNumberOfParamsWalker.h"
#include "BinaryOpTypeErrorWalker.h"
#include "BreakContinuePruneWalker.h"
#include "TupleAccessErrorWalker.h"
#include "ScopeToFreeWalker.h"


int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Missing required argument.\n"
              << "Required arguments: <input file path> <output file path>\n";
    return 1;
  }
  try {
    SyntaxErrorListener syntaxErrorListener;

    // Open the file then parse and lex it.
    antlr4::ANTLRFileStream afs;
    afs.loadFromFile(argv[1]);
    gazprea::GazpreaLexer lexer(&afs);
    lexer.removeErrorListeners();
    lexer.addErrorListener(&syntaxErrorListener);
    antlr4::CommonTokenStream tokens(&lexer);

    gazprea::GazpreaParser parser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&syntaxErrorListener);

    // Get the root of the parse tree. Use your base rule name.
    antlr4::tree::ParseTree *tree;
    tree = parser.file();

    // Generating the ast
    ASTBuilder ast_builder;
    ast_builder.visit(tree);
    auto ast_root = ast_builder.getRoot();

    // write the AST in XML representation to a file (comment this out when not debugging)
    // place these two lines anywhere in main to check the status of the AST after a pass
    //ASTXMLWalker xml_walker;
    //ast_root->accept(xml_walker);

    ValidExprErrorWalker valid_expr_walker;
    ast_root->accept(valid_expr_walker);

    MainErrorWalker main_error_walker;
    ast_root->accept(main_error_walker);

    DefinitionErrorWalker definition_error_walker;
    ast_root->accept(definition_error_walker);

    GlobalErrorWalker globalErrorWalker;
    ast_root->accept(globalErrorWalker);

    GlobalStatementErrorWalker globalStatementErrorWalker;
    ast_root->accept(globalStatementErrorWalker);

    TypeDefStatementErrorWalker typeDefStatementErrorWalker;
    ast_root->accept(typeDefStatementErrorWalker);

    BreakContinueStatementErrorWalker breakContinueStatementErrorWalker;
    ast_root->accept(breakContinueStatementErrorWalker);

    ControlExpressionsWalker controlExpressionsWalker;
    ast_root->accept(controlExpressionsWalker);

    StdInOutStatementErrorWalker stdInOutStatementErrorWalker;
    ast_root->accept(stdInOutStatementErrorWalker);

    FuncProcDefContainsParamIDsWalker funcProcDefContainsParamIDsWalker;
    ast_root->accept(funcProcDefContainsParamIDsWalker);

    CallFuncCallErrorWalker callFuncCallErrorWalker;
    ast_root->accept(callFuncCallErrorWalker);

    DeclStatementErrorWalker declStatementErrorWalker;
    ast_root->accept(declStatementErrorWalker);

    TupleAssignErrorWalker tupleAssignErrorWalker;
    ast_root->accept(tupleAssignErrorWalker);

    ProcReturnCallErrorWalker procReturnCallErrorWalker;
    ast_root->accept(procReturnCallErrorWalker);

    FuncProcCallWithSameNumberOfParamsWalker funcProcCallWithSameNumberOfParamsWalker;
    ast_root->accept(funcProcCallWithSameNumberOfParamsWalker);

    AddFreeNodesWalker add_free_walker;
    ast_root->accept(add_free_walker);

    DefineSymbols symbol_definer;
    ast_root->accept(symbol_definer);

    ConstAssignErrorWalker constAssignErrorWalker;
    ast_root->accept(constAssignErrorWalker);

    AliasingErrorWalker aliasing_error_walker;
    ast_root->accept(aliasing_error_walker);

    // NOTE: THIS PASS WILL PRUNE UNREACHABLE NODES!!!
    ControlFlowReturnErrorWalker control_flow_return_error_walker;
    ast_root->accept(control_flow_return_error_walker);

    // NOTE: THIS PASS WILL PRUNE UNREACHABLE NODES!!!
    BreakContinuePruneWalker break_continue_prune_walker;
    ast_root->accept(break_continue_prune_walker);

    TypeInferWalker typeInferWalker;
    ast_root->accept(typeInferWalker);

    ConstantFoldingWalker const_fold_walker;
    ast_root->accept(const_fold_walker);

    SizeErrorWalker size_error_walker;
    ast_root->accept(size_error_walker);

    TupleAccessErrorWalker tup_access_walker;
    ast_root->accept(tup_access_walker);

    BinaryOpTypeErrorWalker type_error_walker;
    ast_root->accept(type_error_walker);

    ArgumentMutabilityErrorWalker arg_mut_walker;
    ast_root->accept(arg_mut_walker);

    ScopeToFreeWalker scope_to_free_walker;
    ast_root->accept(scope_to_free_walker);

    std::ofstream os(argv[2]);
    CodeGenASTWalker codeGenASTWalker;
    ast_root->accept(codeGenASTWalker);
    codeGenASTWalker.backend.emitModule();
    codeGenASTWalker.backend.lowerDialects();
    codeGenASTWalker.backend.dumpLLVM(os);
  }
  catch (MainError& e) {
    std::cerr << e.what();
    return 1;
  } catch (AssignError& e){
    std::cerr << e.what();
    return 1;
  } catch (GlobalError& e){
    std::cerr << e.what();
    return 1;
  } catch (StatementError& e) {
    std::cerr << e.what();
    return 1;
  } catch (TypeError& e) {
    std::cerr << e.what();
    return 1;
  } catch (AliasingError& e) {
    std::cerr << e.what();
    return 1;
  } catch (SymbolError& e){
    std::cerr << e.what();
    return 1;
  } catch (CallError& e){
    std::cerr << e.what();
    return 1;
  } catch (SyntaxError& e) {
    std::cerr << e.what();
    return 1;
  } catch (ReturnError& e){
    std::cerr << e.what();
    return 1;
  } catch( SizeError&e) {
    std::cerr << e.what();
    return 1;
  } catch (DefinitionError& e) {
    std::cerr << e.what();
    return 1;
  }
  return 0;
}
