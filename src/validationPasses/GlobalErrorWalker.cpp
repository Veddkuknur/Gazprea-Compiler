#include "GlobalErrorWalker.h"


// This pass checks validity of global declarations

std::any GlobalErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
  node->global_block->accept(*this);
  return {};
}


std::any GlobalErrorWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {


    if (node->expr)
        node->expr->accept(*this);
    else
        throw GlobalError(node->line, "Global declaration fails to initialize variable");

    if (!node->constant)
        throw GlobalError(node->line, "Non-constant global variable declaration");
    return {};
}


std::any GlobalErrorWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {
    node->expr->accept(*this);
    return {};
}


std::any GlobalErrorWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {
    node->expr_assign_to->accept(*this);
    node->expr_assign_with->accept(*this);
    return {};
}


std::any GlobalErrorWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {
    node->tuple->accept(*this);
    return {};
}


std::any GlobalErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

    // don't worry about global block statements
    if (depth > 0)
        return {};

    depth++;
    // we assume that this block is the global block
    for (const auto& stat : node->stat_nodes) {
        stat->accept(*this);
    }
    return {};
}


std::any GlobalErrorWalker::visitTupleNode(std::shared_ptr<TupleNode> node) {
    for (auto element : node->tuple)
        element->accept(*this);
    return {};
}


std::any GlobalErrorWalker::visitVecMatNode(std::shared_ptr<VecMatNode> node) {
    for (auto element : node->elements)
        element->accept(*this);
    return {};
}


std::any GlobalErrorWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
    node->expr->accept(*this);
    return {};
}


std::any GlobalErrorWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
  node->l_expr->accept(*this);
  node->r_expr->accept(*this);
  return {};
}


std::any GlobalErrorWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {
    node->expr->accept(*this);
    return {};
}


std::any GlobalErrorWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {
    throw GlobalError(node->line, "Global variable definition contains routine");
}


std::any GlobalErrorWalker::visitLengthCallNode(std::shared_ptr<LengthCallNode> node) {
    throw GlobalError(node->line, "Global variable definition contains routine");
}


std::any GlobalErrorWalker::visitRowsCallNode(std::shared_ptr<RowsCallNode> node) {
    throw GlobalError(node->line, "Global variable definition contains routine");
}


std::any GlobalErrorWalker::visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node) {
    throw GlobalError(node->line, "Global variable definition contains routine");
}


std::any GlobalErrorWalker::visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) {
    throw GlobalError(node->line, "Global variable definition contains routine");
}


std::any GlobalErrorWalker::visitFormatCallNode(std::shared_ptr<FormatCallNode> node) {
    throw GlobalError(node->line, "Global variable definition contains routine");
}


std::any GlobalErrorWalker::visitStreamStateCallNode(std::shared_ptr<StreamStateCallNode> node) {
    throw GlobalError(node->line, "Global variable definition contains routine");
}


std::any GlobalErrorWalker::visitIndexNode(std::shared_ptr<IndexNode> node) {

    node->collection->accept(*this);
    node->index1->accept(*this);
    if (node->index2)
        node->index2->accept(*this);
    return {};
}


std::any GlobalErrorWalker::visitRangeNode(std::shared_ptr<RangeNode> node) {

    node->lower_bound->accept(*this);
    node->upper_bound->accept(*this);
    return {};
}
