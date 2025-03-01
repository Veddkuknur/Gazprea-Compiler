#include "DeclStatementErrorWalker.h"


std::any DeclStatementErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  node->global_block->accept(*this);
  return {};
}


std::any DeclStatementErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  // Visit all the statements and if decl comes after any other kind of statement then throw error
  if (is_global) {
    // visit all the statements
    for (const auto& stat: node->stat_nodes){
      stat->accept(*this);
      is_global = true;
    }
  } else {
    bool declStatmentsOver = false;
    for (const auto& stat: node->stat_nodes){
      if (auto decNode = std::dynamic_pointer_cast<DeclNode>(stat)){
        if (declStatmentsOver){
          // throw error
          throw StatementError(decNode->line, "Declaration is not at the beginning of the block.");
        }
      } else if (std::dynamic_pointer_cast<CondNode>(stat) ||
          std::dynamic_pointer_cast<IterLoopNode>(stat) ||
          std::dynamic_pointer_cast<InfLoopNode>(stat) ||
          std::dynamic_pointer_cast<PredLoopNode>(stat) ||
          std::dynamic_pointer_cast<BlockNode>(stat)){
        stat->accept(*this);
      } else {
        declStatmentsOver = true;
      }
    }
  }
  return {};
}


std::any DeclStatementErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  is_global = false;
  if (node->proc_block) node->proc_block->accept(*this);
  return {};
}


std::any DeclStatementErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  is_global = false;
  if (node->func_block) node->func_block->accept(*this);
  return {};
}


std::any DeclStatementErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any DeclStatementErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any DeclStatementErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any DeclStatementErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}
