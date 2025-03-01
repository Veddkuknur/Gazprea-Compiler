#include "ProcReturnCallErrorWalker.h"
#include <iostream>


std::any ProcReturnCallErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  for (auto stat: node->global_block->stat_nodes){
    stat->accept(*this);
  }
  return {};
}


std::any ProcReturnCallErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  if (procReturns.find(node->proc_id) == procReturns.end()){
    if (node->return_type != nullptr){
      procReturns[node->proc_id] = true;
    } else {
      procReturns[node->proc_id] = false;
    }
  }
  if (node->proc_block) node->proc_block->accept(*this);
  return {};
}


std::any ProcReturnCallErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  for (auto stat: node->stat_nodes){
    if (auto assignNode = std::dynamic_pointer_cast<IdAssignNode>(stat)){
      if (auto rFC = std::dynamic_pointer_cast<FuncProcCallNode>(assignNode->expr)){
        if (procReturns.find(rFC->id) != procReturns.end()) {
          if (!procReturns[rFC->id]) {
            throw CallError(rFC->line, "Procedure with None return being used in assignment");
          }
        }
      }
      assignNode->expr->accept(*this);
    } else if (auto declnode =  std::dynamic_pointer_cast<DeclNode>(stat)){
      if (declnode->expr != nullptr){
        if (auto lfc = std::dynamic_pointer_cast<FuncProcCallNode>(declnode->expr)){
          if (procReturns.find(lfc->id) != procReturns.end()){
            if (!procReturns[lfc->id]) {
              throw CallError(lfc->line, "Procedure with None return being used in declaration");
            }
          }
        }
        declnode->expr->accept(*this);
      }
    } else {
      stat->accept(*this);
    }
  }
  return {};
}


std::any ProcReturnCallErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ProcReturnCallErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ProcReturnCallErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ProcReturnCallErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}


std::any ProcReturnCallErrorWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
  // Walk the expression tree and check if all the procs that are called have a return
  if(auto callNode = std::dynamic_pointer_cast<FuncProcCallNode>(node->l_expr)) {
    if (procReturns.find(callNode->id) != procReturns.end()){
      if (!procReturns[callNode->id]){
        throw CallError(callNode->line, "Procedure with None return being used in Binary Operation");
      }
    }
  }
  if(auto callNode = std::dynamic_pointer_cast<FuncProcCallNode>(node->r_expr)) {
    if (procReturns.find(callNode->id) != procReturns.end()) {
      if (!procReturns[callNode->id]) {
        throw CallError(callNode->line, "Procedure with None return being used in Binary Operation");
      }
    }
  }

  node->r_expr->accept(*this);
  node->l_expr->accept(*this);

  return {};
}


std::any ProcReturnCallErrorWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
  if(auto callNode = std::dynamic_pointer_cast<FuncProcCallNode>(node->expr)) {
    if (procReturns.find(callNode->id) != procReturns.end()) {
      if (!procReturns[callNode->id]) {
        throw CallError(callNode->line, "Procedure with None return being used in Unary Operation");
      }
    }
  }
  return {};
}
