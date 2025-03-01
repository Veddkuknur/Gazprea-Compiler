#include "GlobalStatementErrorWalker.h"


// This pass checks if a function/procedure is in another function or procedure
// and if everything in the global scope are valid

std::any GlobalStatementErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  node->global_block->accept(*this);
  return {};
}


std::any GlobalStatementErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  // visit all the statements in global scope and see if they are valid
  if (is_global){
    for (const auto& stat: node->stat_nodes){
      if (std::dynamic_pointer_cast<InfLoopNode>(stat) ||
          std::dynamic_pointer_cast<PredLoopNode>(stat) ||
          std::dynamic_pointer_cast<IterLoopNode>(stat) ||
          std::dynamic_pointer_cast<CondNode>(stat) ||
          std::dynamic_pointer_cast<FuncProcCallNode>(stat)){
        throw GlobalError(stat->line, "Cannot use this in global scope");
      } else if (std::dynamic_pointer_cast<ReturnNode>(stat)){
        throw StatementError(stat->line, "Return with expression detected outside of routine with return type");
      } else{
        is_global = false;
        stat->accept(*this);
        is_global = true;
      }
    }
  } else {
    // check if any of the nodes contain a func/proc node
    for (const auto& stat: node->stat_nodes){
      if (std::dynamic_pointer_cast<ProcDefNode>(stat) || std::dynamic_pointer_cast<FuncDefNode>(stat)){
        throw StatementError(stat->line,"SubRoutine declaration outside of global scope");
      } else {
        stat->accept(*this);
      }
    }
  }
  return {};
}


std::any GlobalStatementErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  if (node->proc_block) node->proc_block->accept(*this);
  return {};
}


std::any GlobalStatementErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  if (node->func_block) node->func_block->accept(*this);
  return {};
}


std::any GlobalStatementErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any GlobalStatementErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any GlobalStatementErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any GlobalStatementErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}
