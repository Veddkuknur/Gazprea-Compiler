#include "StdInOutStatementErrorWalker.h"


std::any StdInOutStatementErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  for (const auto& statement: node->global_block->stat_nodes){
    if (std::dynamic_pointer_cast<OutStreamNode>(statement)){
      throw StatementError(statement->line,"Cannot use std_output in global scope");
    } else if (std::dynamic_pointer_cast<InStreamNode>(statement)){
      throw AssignError(statement->line,"Cannot re-assign a constant value a");
    } else {
      statement->accept(*this);
    }
  }
  return {};
}


std::any StdInOutStatementErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  if (node->func_block) node->func_block->accept(*this);
  return {};
}


std::any StdInOutStatementErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any StdInOutStatementErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any StdInOutStatementErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any StdInOutStatementErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}


std::any StdInOutStatementErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  // check if the block node contains std_output or std_input
  for (const auto& statement: node->stat_nodes){
    if (std::dynamic_pointer_cast<OutStreamNode>(statement)){
      throw StatementError(statement->line,"Cannot use std_output inside a function");
    } else if (std::dynamic_pointer_cast<InStreamNode>(statement)){
      throw StatementError(statement->line,"Cannot read input inside a function");
    } else {
      statement->accept(*this);
    }
  }

  return {};
}