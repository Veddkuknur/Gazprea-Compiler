#include "CallFuncCallErrorWalker.h"


// Pass to check if a function is calling a procedure

std::any CallFuncCallErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  for (const auto& stat: node->global_block->stat_nodes){
    stat->accept(*this);
  }
  return {};
}


std::any CallFuncCallErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  // iterate over the vector and check if the call is valid
  for (const auto& stat: node->stat_nodes){
    if (auto callNode =  std::dynamic_pointer_cast<FuncProcCallNode>(stat)){
      if(functionNames.find(callNode->id) == functionNames.end() && in_func){
        throw CallError(callNode->line, "Cannot call procedure inside a function.");
      } else if (functionNames.find(callNode->id) != functionNames.end() && callNode->call_used){
        throw CallError(callNode->line, "Call statement used on non-procedure type");
      }
    } else {
      stat->accept(*this);
    }
  }
  return {};
}


std::any CallFuncCallErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  if (node->proc_block)
    node->proc_block->accept(*this);
  return {};
}


std::any CallFuncCallErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  functionNames[node->func_id] = true;
  in_func = true;
  if (node->func_block)
    node->func_block->accept(*this);
  in_func = false;
  return {};
}


std::any CallFuncCallErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any CallFuncCallErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any CallFuncCallErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any CallFuncCallErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}


std::any CallFuncCallErrorWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {
  if (node->expr){
    node->expr->accept(*this);
  }
  if (procCountInExpr > 1){
    throw CallError(node->expr->line, "Procedure is being used with a binary operator");
  }
  procCountInExpr = 0;
  return {};
}


std::any CallFuncCallErrorWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {
  node->expr->accept(*this);

  if (procCountInExpr > 1){
    throw CallError(node->expr->line, "Procedure is being used with a binary operator");
  }
  procCountInExpr = 0;
  return {};
}


std::any CallFuncCallErrorWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
  // Walk the expression tree and add the number of procs in the expr tree
  if(auto callNode = std::dynamic_pointer_cast<FuncProcCallNode>(node->l_expr)) {
    if (functionNames.find(callNode->id) == functionNames.end())
      ++procCountInExpr;
  }
  if(auto callNode = std::dynamic_pointer_cast<FuncProcCallNode>(node->r_expr)) {
    if (functionNames.find(callNode->id) == functionNames.end())
      ++procCountInExpr;
  }
  node->r_expr->accept(*this);
  node->l_expr->accept(*this);

  return {};
}


std::any CallFuncCallErrorWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {
  if (auto call_node = std::dynamic_pointer_cast<FuncProcCallNode>(node->expr)) {
    if (functionNames.find(call_node->id) == functionNames.end())
      throw CallError(node->expr->line, "Procedure is being used outside an assignment statement");
  }
  return {};
}
