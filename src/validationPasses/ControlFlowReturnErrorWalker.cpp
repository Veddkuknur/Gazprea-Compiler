#include "ControlFlowReturnErrorWalker.h"


std::any ControlFlowReturnErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    node->global_block->accept(*this);
    return {};
}


std::any ControlFlowReturnErrorWalker::visitBlockNode(const std::shared_ptr<BlockNode> node) {

    for (int i = 0; i < static_cast<int>(node->stat_nodes.size()); i++) {
        auto return_val = node->stat_nodes[i]->accept(*this);

        // if the stat we just visited was a return or a conditional with a guaranteed return,
        // we have confirmed the body has a reachable return by all control flows
        if (return_val.has_value() && std::any_cast<bool>(return_val) == true) {

            // prune all nodes following this terminal node
            node->stat_nodes.resize(i + 1);
            return true;
        }
    }
    return false;
}


std::any ControlFlowReturnErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any ControlFlowReturnErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any ControlFlowReturnErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}



std::any ControlFlowReturnErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

    // ignore procedure prototypes
    if (not node->proc_block)
        return {};

    auto return_val = node->proc_block->accept(*this);

    // a return value of false indicates that the procedure's body block does not have a
    // reachable return by all control flows
    if (std::any_cast<bool>(return_val) == false) {
        throw ReturnError(node->line,"Subroutine does not have a return statement reachable by all control flows.");
    }
    return {};
}


std::any ControlFlowReturnErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {

    // ignore function prototypes
    if (not node->func_block)
        return {};

    auto return_val = node->func_block->accept(*this);

    // a return value of false indicates that the function's body block does not have a
    // reachable return by all control flows
    if (std::any_cast<bool>(return_val) == false)
        throw ReturnError(node->line,"Subroutine does not have a return statement reachable by all control flows.");
    return {};
}


std::any ControlFlowReturnErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {

    bool if_block_return_val = std::any_cast<bool>(node->if_block->accept(*this));
    bool else_block_return_val = false;

    if (node->else_block)
        else_block_return_val = std::any_cast<bool>(node->else_block->accept(*this));

    // if the if block and else block are not guaranteed to return, the conditional isn't terminal
    if (not (if_block_return_val and else_block_return_val))
        return false;

    // tag this node as terminal
    node->is_terminal = true;
    return true; // the conditional is a guaranteed return
}


std::any ControlFlowReturnErrorWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {
    // signals to the caller that a return node was found
    return true;
}

