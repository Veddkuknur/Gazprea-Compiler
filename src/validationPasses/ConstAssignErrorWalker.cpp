#include "ConstAssignErrorWalker.h"

#include <Symbol.h>

// This pass is assuming that all the declarations in the global scope are const, there should be a
// pass before this that checks if the global declarations are valid -- Added

std::any ConstAssignErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  node->global_block->accept(*this);
  return {};
}


std::any ConstAssignErrorWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {
  auto id_symbol = node->containing_scope->resolve(node->id);
  if (id_symbol->mutability == false)
    throw AssignError(node->line, "Cannot re-assign a constant value " + node->id);
  return {};
}


std::any ConstAssignErrorWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {

  // index case (variable storing a vector or matrix)
  if (auto index_node = std::dynamic_pointer_cast<IndexNode>(node)) {
    if (auto id_node = std::dynamic_pointer_cast<IdNode>(index_node)) {
      auto id_symbol = id_node->containing_scope->resolve(id_node->id);
      if (id_symbol->mutability == false)
        throw AssignError(node->line, "Cannot re-assign a constant value " + id_node->id);
    }
  }
  // tuple case (variable storing a tuple)
  else if (auto tuple_access_node = std::dynamic_pointer_cast<TupleAccessNode>(node)) {
    auto tuple_symbol = tuple_access_node->containing_scope->resolve(tuple_access_node->tuple_id->id);
    if (tuple_symbol->mutability == false)
      throw AssignError(node->line, "Cannot re-assign a constant value " + tuple_access_node->tuple_id->id);
  }
  return {};
}


std::any ConstAssignErrorWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {

  auto l_values  = node->ivalues;

  for (auto l_value : l_values) {
    // id case
    if (auto id_node = std::dynamic_pointer_cast<IdNode>(l_value)) {
      auto id_symbol = id_node->containing_scope->resolve(id_node->id);
      if (id_symbol->mutability == false)
        throw AssignError(node->line, "Cannot re-assign a constant value " + id_node->id);
    }
    // index case (variable storing a vector or matrix)
    else if (auto index_node = std::dynamic_pointer_cast<IndexNode>(l_value)) {
      if (auto id_node = std::dynamic_pointer_cast<IdNode>(index_node)) {
        auto id_symbol = id_node->containing_scope->resolve(id_node->id);
        if (id_symbol->mutability == false)
          throw AssignError(node->line, "Cannot re-assign a constant value " + id_node->id);
      }
    }
    // tuple case (variable storing a tuple)
    else if (auto tuple_access_node = std::dynamic_pointer_cast<TupleAccessNode>(l_value)) {
      auto tuple_symbol = tuple_access_node->containing_scope->resolve(tuple_access_node->tuple_id->id);
      if (tuple_symbol->mutability == false)
        throw AssignError(node->line, "Cannot re-assign a constant value " + tuple_access_node->tuple_id->id);
    }
  }
  return {};
}


std::any ConstAssignErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

  for (const auto& stat : node->stat_nodes) {
    stat->accept(*this);
  }
  return {};
}


std::any ConstAssignErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

  if (node->proc_block)
    node->proc_block->accept(*this);
  return {};
}


std::any ConstAssignErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {

  if (node->func_block)
    node->func_block->accept(*this);
  return {};
}


std::any ConstAssignErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ConstAssignErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ConstAssignErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ConstAssignErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}
