#include "ControlExpressionsWalker.h"
#include <iostream>


std::any ControlExpressionsWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  node->global_block->accept(*this);
  return {};
}


std::any ControlExpressionsWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  for (auto stat: node->stat_nodes){
    if (!std::dynamic_pointer_cast<FuncProcCallNode>(stat)) stat->accept(*this);
  }
  return {};
}


std::any ControlExpressionsWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  if (node->proc_block) node->proc_block->accept(*this);
  subroutinesReturns[node->proc_id] = node->return_type;
  return {};
}


std::any ControlExpressionsWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  if (node->func_block) node->func_block->accept(*this);
  subroutinesReturns[node->func_id] = node->return_type;
  return {};
}


std::any ControlExpressionsWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ControlExpressionsWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  if (!std::any_cast<bool>(node->contr_expr->accept(*this))){
    throw TypeError(node->line, "Condition type is not a boolean");
  }
  node->loop_block->accept(*this);
  return {};
}


std::any ControlExpressionsWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any ControlExpressionsWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  if (!std::any_cast<bool>(node->contr_expr->accept(*this))){
    throw TypeError(node->line, "Condition type is not a boolean");
  }
  node->if_block->accept(*this);
  if (node->else_block) node->else_block->accept(*this);
  return {};
}


std::any ControlExpressionsWalker::visitIntNode(std::shared_ptr<IntNode> node) {
  return false;
}


std::any ControlExpressionsWalker::visitRealNode(std::shared_ptr<RealNode> node) {
  return false;
}


std::any ControlExpressionsWalker::visitCharNode(std::shared_ptr<CharNode> node) {
  return false;
}


std::any ControlExpressionsWalker::visitBoolNode(std::shared_ptr<BoolNode> node) {
  return true;
}


std::any ControlExpressionsWalker::visitTupleNode(std::shared_ptr<TupleNode> node) {
  return false;
}

std::any ControlExpressionsWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {
  if (auto cast_type = std::dynamic_pointer_cast<PrimitiveType>(node->cast_type)) {
    if (cast_type->base_type == Type::BaseType::boolean)
      return true;
  }
  return false;
}


std::any ControlExpressionsWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
  return node->op == "not";
}


std::any ControlExpressionsWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
  if (node->op == "==" || node->op == "!=" || node->op == "or" ||
       node->op == "xor" || node->op == "and" || node->op == "<" ||
       node->op == ">" || node->op == "<=" || node->op == ">=")
    return true;
  return false;
}


std::any ControlExpressionsWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {
  if (subroutinesReturns.find(node->id) != subroutinesReturns.end()
    && subroutinesReturns[node->id]->getBaseType() != Type::BaseType::boolean){
    return false;
  }
  return true;
}


std::any ControlExpressionsWalker::visitGeneratorNode(std::shared_ptr<GeneratorNode> node) {
  return false;

}


std::any ControlExpressionsWalker::visitFilterNode(std::shared_ptr<FilterNode> node) {
  return false;
}


std::any ControlExpressionsWalker::visitIndexNode(std::shared_ptr<IndexNode> node) {
  return false;
}
std::any ControlExpressionsWalker::visitRangeNode(std::shared_ptr<RangeNode> node) {
  return false;
}

std::any ControlExpressionsWalker::visitVecMatNode(std::shared_ptr<VecMatNode> node) {
  return false;
}












std::any ControlExpressionsWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {
  if (node->type){
    varTypes[node->id] = node->type;
  } else {
    if (not std::any_cast<bool>(node->expr->accept(*this))){
        varTypes[node->id] = std::make_shared<PrimitiveType>(Type::BaseType::null);
    }
    else {
        varTypes[node->id] = std::make_shared<PrimitiveType>(Type::BaseType::boolean);
    }
  }
  return {};
}


std::any ControlExpressionsWalker::visitIdNode(std::shared_ptr<IdNode> node) {
  if (varTypes.find(node->id) != varTypes.end() && varTypes[node->id]->getBaseType() != Type::BaseType::boolean){
    return false;
  }
  return true;
}


std::any ControlExpressionsWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {
    return false;
}
std::any ControlExpressionsWalker::visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) {
  return false;
}
