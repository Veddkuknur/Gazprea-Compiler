#include "TupleAssignErrorWalker.h"
#include <iostream>


std::any TupleAssignErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  node->global_block->accept(*this);
  return {};
}


std::any TupleAssignErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  for (auto stat: node->stat_nodes){
    stat->accept(*this);
  }
  return {};
}


std::any TupleAssignErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  if (node->proc_block) {
    node->proc_block->accept(*this);

  }
  return {};
}


std::any TupleAssignErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  if (node->func_block) {
    node->func_block->accept(*this);

  }
  return {};
}


std::any TupleAssignErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any TupleAssignErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any TupleAssignErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  node->loop_block->accept(*this);
  return {};
}


std::any TupleAssignErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  node->if_block->accept(*this);
  if (node->else_block){
    node->else_block->accept(*this);
  }
  return {};
}


std::any TupleAssignErrorWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {
  if (node->expr){
    if (auto tupNode = std::dynamic_pointer_cast<TupleNode>(node->expr)) {
      if (auto declType = std::dynamic_pointer_cast<TupleType>(node->type)){
        // Throw error if invalid assign
        auto declTypeVector = declType->element_types;
        auto tupeNodeElementVector = tupNode->tuple;

        if (declTypeVector.size() != tupeNodeElementVector.size()){
          throw TypeError(node->line, "Cannot assign a tuple expression of size " + std::to_string(tupeNodeElementVector.size()) +" to a tuple variable of size " +  std::to_string(declTypeVector.size()));
        }else {
          // check if the types are same
          for (auto i = 0; i < static_cast<int>(declTypeVector.size()); i++){
            if(declTypeVector[i]->getBaseType() == Type::BaseType::null){
              throw TypeError(node->line, "Unsupported base type for user defined type");
            }
            auto el = tupeNodeElementVector[i];
            if (std::dynamic_pointer_cast<IntNode>(el)){
              if (declTypeVector[i]->getBaseType() != Type::BaseType::integer
                && declTypeVector[i]->getBaseType() != Type::BaseType::real){
                throw TypeError(node->line, "Cannot assign an integer expression to a " + declTypeVector[i]->getTypeAsString() + " in a tuple");
              }
            } else if (std::dynamic_pointer_cast<RealNode>(el)){
              if (declTypeVector[i]->getBaseType() != Type::BaseType::real){
                throw TypeError(node->line, "Cannot assign a real expression to a " + declTypeVector[i]->getTypeAsString() + " in a tuple");
              }
            } else if (std::dynamic_pointer_cast<BoolNode>(el)){
              if (declTypeVector[i]->getBaseType() != Type::BaseType::boolean){
                throw TypeError(node->line, "Cannot assign a boolean expression to a " + declTypeVector[i]->getTypeAsString() + " in a tuple");
              }
            } else if (std::dynamic_pointer_cast<CharNode>(el)){
              if (declTypeVector[i]->getBaseType() != Type::BaseType::character){
                throw TypeError(node->line, "Cannot assign a character expression to a " + declTypeVector[i]->getTypeAsString() + " in a tuple");
              }
            } else {
              std::cout << "unidentified elem in tuple " << declTypeVector[i]->getTypeAsString() << std::endl;
            }
          }
        }
      } else {
        if (!node->declType.empty()) throw TypeError(node->line, "Cannot assign a tuple expression type to a variable of type " + node->declType);
      }
    }
  }
  return {};
}
