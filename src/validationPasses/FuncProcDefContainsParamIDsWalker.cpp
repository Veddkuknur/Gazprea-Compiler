#include "FuncProcDefContainsParamIDsWalker.h"
#include <tuple>


std::any FuncProcDefContainsParamIDsWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  for (auto stat: node->global_block->stat_nodes){
    stat->accept(*this);
  }
  return {};
}


std::any FuncProcDefContainsParamIDsWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
  if (node->proc_block){
    // iter over the params and check if they have ids
    for(auto param: node->params){
      if(std::get<2>(param).empty()){
        throw SyntaxError(node->line, "Param in pxrocedure definition has missing id");
      }
    }
  }
  return {};
}


std::any FuncProcDefContainsParamIDsWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
  if (node->func_block){
    // iter over the params and check if they have ids
    for(auto param: node->params){
      if(std::get<1>(param).empty()){
        throw SyntaxError(node->line, "Param in function definition has missing id");
      }
    }
  }
  return {};
}
