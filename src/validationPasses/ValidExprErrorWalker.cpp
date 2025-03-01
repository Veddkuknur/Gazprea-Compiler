#include "ValidExprErrorWalker.h"
#include "iostream"


std::any ValidExprErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    auto result = node->global_block->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {
    if (node->expr){
        auto result = node->expr->accept(*this);
    }
    return {};
}


std::any ValidExprErrorWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {
    node->expr->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {
    /*if (not std::dynamic_pointer_cast<TupleAccessNode>(node->expr_assign_to)) {
        throw SyntaxError(node->line, "Invalid tuple assignment expression.");
    } */
    node->expr_assign_with->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {

    // visit the i-values to unpack to
    for (auto ivalue : node->ivalues) {
        auto result = ivalue->accept(*this);
    }
    // visit the tuple
    auto result = node->tuple->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
    for (auto stat : node->stat_nodes) {
        auto result = stat->accept(*this);
    }
    return {};
}


std::any ValidExprErrorWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
    auto result = node->expr->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
    auto result = node->l_expr->accept(*this);
    result = node->r_expr->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {
    auto result = node->expr->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {

    // visit the loop block
    auto result = node->loop_block->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

    // visit the control expression
    auto result = node->contr_expr->accept(*this);

    // visit the loop block
    result = node->loop_block->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

    // visit the domain expression
    auto result = node->dom_expr->accept(*this);

    // visit the loop block
    result = node->loop_block->accept(*this);
    return {};
}


std::any ValidExprErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {

    // visit control expression
    auto result = node->contr_expr->accept(*this);

    // visit "if" block
    result = node->if_block->accept(*this);

    // visit "else" block (if it exists)
    if (node->else_block != nullptr) {
        result = node->else_block->accept(*this);
    }
    return {};
}


std::any ValidExprErrorWalker::visitOutStreamNode(std::shared_ptr<OutStreamNode> node) {

    // visit the expression to be printed
    auto result = node->expr->accept(*this);
    return {};
}

// TODO: Garrett was here. needs to be rewritten to account for change to InStreamNodes
std::any ValidExprErrorWalker::visitInStreamNode(std::shared_ptr<InStreamNode> node) {
    if(const auto idNode = std::dynamic_pointer_cast<IdNode>(node->l_value)){
        return {};
    }
    if (const auto idNode = std::dynamic_pointer_cast<TupleAccessNode>(node->l_value)) {
        return {};
    }
    if (const auto idNode = std::dynamic_pointer_cast<VecMatTupAssignNode>(node->l_value)) {
        return {};
    }
    if (const auto idNode = std::dynamic_pointer_cast<IndexNode>(node->l_value)) {
        return {};
    }
    throw GlobalError(node->line, "fix me in ValidExprErrorWalker.cpp");
}


std::any ValidExprErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
    if (node->func_block) {
        auto result = node->func_block->accept(*this);
    }
    return {};
}


std::any ValidExprErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    if (node->proc_block) {
        auto result = node->proc_block->accept(*this);
    }
    return {};
}


std::any ValidExprErrorWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {
    if (node->expr) {
        auto result = node->expr->accept(*this);
    }
    return {};
}


std::any ValidExprErrorWalker::visitIdNode(std::shared_ptr<IdNode> node) {
    return {};
}


std::any ValidExprErrorWalker::visitBreakNode(std::shared_ptr<BreakNode> node) {
    std::string return_val = "return value from BreakNode!";
    return {return_val};
}


std::any ValidExprErrorWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {

    // visit the id node
    auto result = node->tuple_id->accept(*this);

    // visit the tuple element node
    result = node->element->accept(*this);
    return {};
}
