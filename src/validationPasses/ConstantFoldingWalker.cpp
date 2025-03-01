#include "ConstantFoldingWalker.h"

std::any ConstantFoldingWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    node->global_block->accept(*this);
    return {};
}


std::any ConstantFoldingWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {

    // vector declaration
    if (auto vector_type = std::dynamic_pointer_cast<VectorType>(node->type)) {
        if (not vector_type->size)
            return {};
        auto return_val = vector_type->size->accept(*this);
        if (return_val.has_value()) {
            int int_val = std::any_cast<int>(return_val);
            std::shared_ptr<ASTNode> int_node = std::make_shared<IntNode>(int_val, node->line, node->column);
            vector_type->size = std::move(int_node);
        }
    }
    // matrix declaration
    if (auto matrix_type = std::dynamic_pointer_cast<MatrixType>(node->type)) {
        if (not matrix_type->rows or not matrix_type->cols)
            return {};
        auto return_val1 = matrix_type->rows->accept(*this);
        auto return_val2 = matrix_type->cols->accept(*this);
        if (return_val1.has_value() and return_val2.has_value()) {
            int row_val = std::any_cast<int>(return_val1);
            int col_val = std::any_cast<int>(return_val2);
            std::shared_ptr<ASTNode> row_int_node = std::make_shared<IntNode>(row_val, node->line, node->column);
            std::shared_ptr<ASTNode> col_int_node = std::make_shared<IntNode>(col_val, node->line, node->column);
            matrix_type->rows = std::move(row_int_node);
            matrix_type->cols = std::move(col_int_node);
        }
    }
    return {};
}


std::any ConstantFoldingWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
    for (const auto& stat : node->stat_nodes)
        stat->accept(*this);
    return {};
}


std::any ConstantFoldingWalker::visitIntNode(std::shared_ptr<IntNode> node) {
    return node->val;
}


std::any ConstantFoldingWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
    auto expr_return = node->expr->accept(*this);

    if (not expr_return.has_value())
        return {};

    int int_val = std::any_cast<int>(expr_return);

    if (node->op == "+") {return int_val;}
    if (node->op == "-") {return -int_val;}
    return {};
}


std::any ConstantFoldingWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
    auto l_expr_return = node->l_expr->accept(*this);
    auto r_expr_return = node->r_expr->accept(*this);

    if (not (l_expr_return.has_value() and r_expr_return.has_value()))
        return {};

    int l_int = std::any_cast<int>(l_expr_return);
    int r_int = std::any_cast<int>(r_expr_return);

    if (node->op == "+") {return l_int + r_int;}
    if (node->op == "-") {return l_int - r_int;}
    if (node->op == "*") {return l_int * r_int;}
    if (node->op == "/") {return l_int / r_int;}
    if (node->op == "%") {return l_int % r_int;}
    if (node->op == "^") {return l_int ^ r_int;}
    return {};
}


std::any ConstantFoldingWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {
    if (auto prim_type = std::dynamic_pointer_cast<PrimitiveType>(node->cast_type)) {
        if (prim_type->getBaseType() == Type::BaseType::integer) {
            if (auto int_node = std::dynamic_pointer_cast<IntNode>(node->expr))
                return int_node->val;
        }
    }
    return {};
}


std::any ConstantFoldingWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any ConstantFoldingWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any ConstantFoldingWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any ConstantFoldingWalker::visitCondNode(std::shared_ptr<CondNode> node) {
    node->if_block->accept(*this);
    if (node->else_block)
        node->else_block->accept(*this);
    return {};
}


std::any ConstantFoldingWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
    if (node->func_block)
        node->func_block->accept(*this);
    return {};
}


std::any ConstantFoldingWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    if (node->proc_block)
        node->proc_block->accept(*this);
    return {};
}