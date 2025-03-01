#include "BreakContinuePruneWalker.h"


std::any BreakContinuePruneWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  node->global_block->accept(*this);
  return {};
}


std::any BreakContinuePruneWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

  for (int i = 0; i < static_cast<int>(node->stat_nodes.size()); i++) {
    auto return_val = node->stat_nodes[i]->accept(*this);

    // if the stat we just visited was a break/continue, prune the statements following the break/continue
    if (return_val.has_value() && std::any_cast<bool>(return_val) == true) {
      node->stat_nodes.resize(i+1);
    }
  }
  return {};
}


std::any BreakContinuePruneWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {

  node->loop_block->accept(*this);
  return {};
}


std::any BreakContinuePruneWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

  node->loop_block->accept(*this);
  return {};
}


std::any BreakContinuePruneWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

  node->loop_block->accept(*this);
  return {};
}


std::any BreakContinuePruneWalker::visitCondNode(std::shared_ptr<CondNode> node) {

  node->if_block->accept(*this);

  if (node->else_block)
    node->else_block->accept(*this);

  return {};
}


std::any BreakContinuePruneWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {

  // ignore function prototypes
  if (not node->func_block)
    return {};

  node->func_block->accept(*this);
  return {};
}


std::any BreakContinuePruneWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

  // ignore procedure prototypes
  if (not node->proc_block)
    return {};

  node->proc_block->accept(*this);
  return {};
}


std::any BreakContinuePruneWalker::visitBreakNode(std::shared_ptr<BreakNode> node) {
  return true;
}


std::any BreakContinuePruneWalker::visitContinueNode(std::shared_ptr<ContinueNode> node) {
  return true;
}
