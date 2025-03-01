#include "FuncProcCallWithSameNumberOfParamsWalker.h"
#include <iostream>


std::any FuncProcCallWithSameNumberOfParamsWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  for (auto stat: node->global_block->stat_nodes){
    stat->accept(*this);
  }
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  subroutines[node->func_id] = node->params.size();
  if (node->func_block) node->func_block->accept(*this);
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  subroutines[node->proc_id] = node->params.size();
  if (node->proc_block) node->proc_block->accept(*this);
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  for (auto stat: node->stat_nodes){
      stat->accept(*this);
  }
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {
  if (node->expr){
    node->expr->accept(*this);
  }
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {
  node->expr->accept(*this);
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
  node->r_expr->accept(*this);
  node->l_expr->accept(*this);
  return {};
}


std::any FuncProcCallWithSameNumberOfParamsWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node){
  auto it = subroutines.find(node->id);
  if (it != subroutines.end() && node->params.size() != it->second){
    throw CallError(node->line, "Subroutine call has incorrect number of parameters");
  }
  return {};
}
