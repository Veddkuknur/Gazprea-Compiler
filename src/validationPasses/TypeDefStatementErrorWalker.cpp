#include "TypeDefStatementErrorWalker.h"


// Checks if typedef is in the global scope

std::any TypeDefStatementErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  for(auto statement: node->global_block->stat_nodes){
    statement->accept(*this);
  }
  return {};
}


std::any TypeDefStatementErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  if (node->proc_block) node->proc_block->accept(*this);
  return {};
}


std::any TypeDefStatementErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  if (node->func_block) node->func_block->accept(*this);
  return {};
}


std::any TypeDefStatementErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any TypeDefStatementErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any TypeDefStatementErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any TypeDefStatementErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}


std::any TypeDefStatementErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  for(auto stat: node->stat_nodes){
    if(std::dynamic_pointer_cast<TypeDefNode>(stat)){
      throw StatementError(stat->line, "Typedef outside of global scope");
    } else{
      stat->accept(*this);
    }
  }
  return {};
}