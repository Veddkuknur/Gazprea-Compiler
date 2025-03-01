#include "AddFreeNodesWalker.h"


std::any AddFreeNodesWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    node->global_block->accept(*this);
    return {};
}


std::any AddFreeNodesWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

    auto free_node = std::make_shared<FreeNode>();

    for (int i = 0; i < static_cast<int>(node->stat_nodes.size()); i++) {
        node->stat_nodes[i]->accept(*this);
        if (std::dynamic_pointer_cast<BreakNode>(node->stat_nodes[i]) or
            std::dynamic_pointer_cast<ContinueNode>(node->stat_nodes[i])) {

            // add a free node before the first occurring break, continue
            free_node->scope_id = 2; // 2 for loop
            node->stat_nodes.insert(node->stat_nodes.begin() + i, free_node);
            return {};
        }
    }
    // no break, continue, or return was found. Add a free node at the end of the block if we arent in the global scope
    if (not is_in_global_scope) {
        free_node->scope_id = 0; // 0 for current scope
        node->stat_nodes.push_back(free_node);
    }
    return {};
}


std::any AddFreeNodesWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any AddFreeNodesWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any AddFreeNodesWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any AddFreeNodesWalker::visitCondNode(std::shared_ptr<CondNode> node) {
    node->if_block->accept(*this);
    if (node->else_block)
        node->else_block->accept(*this);
    return {};
}


std::any AddFreeNodesWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
    is_in_global_scope = false;
    if (node->func_block)
        node->func_block->accept(*this);
    is_in_global_scope = true;
    return {};
}


std::any AddFreeNodesWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

    is_in_global_scope = false;
    if (node->proc_id == "main")
        is_in_main_procedure = true;
    if (node->proc_block)
        node->proc_block->accept(*this);
    is_in_main_procedure = false;
    is_in_global_scope = true;
    return {};
}


std::any AddFreeNodesWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {

    node->free_node = std::make_shared<FreeNode>();
    if (is_in_main_procedure)
        node->free_node->scope_id = 3; // for main proc
    else
        node->free_node->scope_id = 1; // 1 for func/proc
    return {};
}