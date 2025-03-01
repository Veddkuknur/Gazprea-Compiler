#include "ReturnErrorWalker.h"


std::any ReturnErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  node->global_block->accept(*this);
  return {};
}


std::any ReturnErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  for (auto stat: node->stat_nodes){
    if (auto ret = std::dynamic_pointer_cast<ReturnNode>(stat)){
      containsRet = true;
    }
    stat->accept(*this);
  }
  return {};
}

std::any ReturnErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  containsRet = false;
  if (node->proc_block) node->proc_block->accept(*this);
  if (!containsRet && node->return_type != nullptr){
    throw ReturnError(node->line,"Subroutine does not have a return statement reachable by all control flows.");
  }
  return {};
}


std::any ReturnErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  containsRet = false;
  if (node->func_block) node->func_block->accept(*this);
  if (node->func_block && !containsRet && node->return_type != nullptr){
    throw ReturnError(node->line,"Subroutine does not have a return statement reachable by all control flows.");
  }
  return {};
}


std::any ReturnErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ReturnErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ReturnErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ReturnErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}
