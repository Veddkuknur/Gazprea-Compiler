#include "BreakContinueStatementErrorWalker.h"


std::any BreakContinueStatementErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  for(auto statement: node->global_block->stat_nodes){
    // Visit all the statements
    statement->accept(*this);
  }
  return {};
}


std::any BreakContinueStatementErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  if (node->proc_block) node->proc_block->accept(*this);
  return {};
}


std::any BreakContinueStatementErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  if (node->func_block) node->func_block->accept(*this);
  return {};
}


std::any BreakContinueStatementErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}


std::any BreakContinueStatementErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  for(auto stat: node->stat_nodes){
    stat->accept(*this);
  }
  return {};
}


std::any BreakContinueStatementErrorWalker::visitBreakNode(std::shared_ptr<BreakNode> node) {
  // Throw error if came here
  throw StatementError(node->line, "Break detected outside of loop");
}


std::any BreakContinueStatementErrorWalker::visitContinueNode(std::shared_ptr<ContinueNode> node) {
  // Throw error if came here
  throw StatementError(node->line, "Continue detected outside of loop");
}
