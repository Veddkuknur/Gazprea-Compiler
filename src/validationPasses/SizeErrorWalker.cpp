#include "SizeErrorWalker.h"

#include <ScopedSymbol.h>


bool SizeErrorWalker::isNonVecMatLiteral(const std::shared_ptr<ASTNode> &node) {

    if (std::dynamic_pointer_cast<BoolNode>(node) or
        std::dynamic_pointer_cast<CharNode>(node) or
        std::dynamic_pointer_cast<IntNode>(node) or
        std::dynamic_pointer_cast<RealNode>(node))
        return true;

    return false;
}


std::any SizeErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {

    node->global_block->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {

    // size error case #2 vectors
    if (auto vector_type = std::dynamic_pointer_cast<VectorType>(node->type)) {
        if (auto int_size_node = std::dynamic_pointer_cast<IntNode>(vector_type->size)) {
            if (std::dynamic_pointer_cast<VecMatNode>(node->expr)) {
                auto result = node->expr->accept(*this);
                if (result.has_value()) {
                    auto rows_cols = std::any_cast<std::pair<int,int>>(result);
                    if (rows_cols.first > int_size_node->val)
                        throw SizeError(node->line,
                            "Size of the initialization expression is larger, "
                            "in the first dimension, than the declared size.");
                }
            }
        }
    }
    // size error case #2 matrices
    if (auto matrix_type = std::dynamic_pointer_cast<MatrixType>(node->type)) {
        if (auto row_num_node = std::dynamic_pointer_cast<IntNode>(matrix_type->rows)) {
            if (auto col_num_node = std::dynamic_pointer_cast<IntNode>(matrix_type->cols)) {
                if (std::dynamic_pointer_cast<VecMatNode>(node->expr)) {
                    auto result = node->expr->accept(*this);
                    if (result.has_value()) {
                        auto rows_cols = std::any_cast<std::pair<int,int>>(result);
                        if (rows_cols.first > row_num_node->val)
                            throw SizeError(node->line, "Size of the initialization expression is larger, "
                            "in the first dimension, than the declared size.");
                        if (rows_cols.second > col_num_node->val)
                            throw SizeError(node->line, "Size of the initialization expression is larger, "
                                                        "in the second dimension, than the declared size.");
                    }
                }
            }
        }
    }


    // size error case #3 and #4 vectors
    if (auto vector_type = std::dynamic_pointer_cast<VectorType>(node->type)) {
        if (vector_type->size == nullptr) {
            if (node->expr == nullptr)
                throw SizeError(node->line, "Cannot declare an implicit size vector without also assigning a value.");
            if (std::dynamic_pointer_cast<VectorType>(node->expr->type) == nullptr)
                throw SizeError(node->line, "Cannot initialize a vector variable of implicit size with non-matching type");
        }
    }
    // size error case #3 and #4 matrices
    if (auto matrix_type = std::dynamic_pointer_cast<MatrixType>(node->type)) {
        if (matrix_type->rows == nullptr or matrix_type->cols == nullptr) {
            if (node->expr == nullptr)
                throw SizeError(node->line,
                    "Cannot declare an implicit size matrix without also assigning a value.");
            if (std::dynamic_pointer_cast<MatrixType>(node->expr->type) == nullptr)
                throw SizeError(node->line, "Cannot initialize a matrix variable of implicit size with non-matching type");
        }
    }
    if (node->expr)
        node->expr->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

    for (const auto& stat : node->stat_nodes)
        stat->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitBoolNode(std::shared_ptr<BoolNode> node) {
    return true;
}


std::any SizeErrorWalker::visitCharNode(std::shared_ptr<CharNode> node) {
    return true;
}


std::any SizeErrorWalker::visitIntNode(std::shared_ptr<IntNode> node) {
    return true;
}


std::any SizeErrorWalker::visitRealNode(std::shared_ptr<RealNode> node) {
    return true;
}


std::any SizeErrorWalker::visitVecMatNode(std::shared_ptr<VecMatNode> node) {

    // if the vector/subvector literal is empty, we cant determine the size at compile time
    if (node->elements.empty())
        return {};

    // vector case
    if (std::dynamic_pointer_cast<VectorType>(node->type)) {
        // loop through the elements of the vector and if you find a non-literal primitive type,
        // the size cannot be determined at compile time
        for (auto elem : node->elements) {
            if (not isNonVecMatLiteral(elem))
                return {};
        }
        std::pair<int, int> rows = {node->elements.size(), -1};
        return rows;
    }

    // matrix case
    // get the size of the first sub vector so we can check equality of the size of every sub vector
    if (std::dynamic_pointer_cast<MatrixType>(node->type)) {
        int largest_sub_vec_size = 0;
        for (auto elem : node->elements) {

            // make sure the matrix elements are vectors
            if (auto sub_vec = std::dynamic_pointer_cast<VecMatNode>(elem)) {
                for (auto sub_vec_elem : sub_vec->elements) {
                    if (not isNonVecMatLiteral(sub_vec_elem))
                        return {};
                }
                int sub_vec_size = static_cast<int>(sub_vec->elements.size());
                if (sub_vec_size > largest_sub_vec_size)
                    largest_sub_vec_size = sub_vec_size;
            }
            else
                return {};
        }
        std::pair<int, int> rows_cols = {node->elements.size(), largest_sub_vec_size};
        return rows_cols;
    }
}



std::any SizeErrorWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
    node->expr->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {

    // size error case #1

    if (node->op == "||" or node->op == "by")
        return {};

    auto l_expr = std::dynamic_pointer_cast<VecMatNode>(node->l_expr);
    auto r_expr = std::dynamic_pointer_cast<VecMatNode>(node->r_expr);

    if (l_expr == nullptr or r_expr == nullptr)
        return {};

    auto l_vector = std::dynamic_pointer_cast<VectorType>(l_expr->type);
    auto r_vector = std::dynamic_pointer_cast<VectorType>(r_expr->type);
    if (l_vector != nullptr and r_vector != nullptr) {
        auto l_vector_size = node->l_expr->accept(*this);
        auto r_vector_size = node->r_expr->accept(*this);
        if (l_vector_size.has_value() and r_vector_size.has_value()) {
            auto l_pair = std::any_cast<std::pair<int, int>>(l_vector_size);
            auto r_pair = std::any_cast<std::pair<int, int>>(r_vector_size);
            if (l_pair.first != r_pair.first)
                throw SizeError(node->line,
                    "Cannot perform a binary operation on vectors/matrices of different size.");
        }
        return {};
    }
    auto l_matrix = std::dynamic_pointer_cast<MatrixType>(l_expr->type);
    auto r_matrix = std::dynamic_pointer_cast<MatrixType>(r_expr->type);
    if (l_matrix != nullptr and r_matrix != nullptr) {
        auto l_matrix_size = node->l_expr->accept(*this);
        auto r_matrix_size = node->r_expr->accept(*this);
        if (l_matrix_size.has_value() and r_matrix_size.has_value()) {
            auto l_pair = std::any_cast<std::pair<int, int>>(l_matrix_size);
            auto r_pair = std::any_cast<std::pair<int, int>>(r_matrix_size);
            if (l_pair.first != r_pair.first or l_pair.second != r_pair.second)
                throw SizeError(node->line,
                    "Cannot perform a binary operation on vectors/matrices of different size.");
        }
    }
    return {};
}


std::any SizeErrorWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node)  {
    node->expr->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

    node->contr_expr->accept(*this);
    node->loop_block->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

    node->dom_expr->accept(*this);
    node->loop_block->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {
    node->contr_expr->accept(*this);

    node->if_block->accept(*this);
    if (node->else_block)
      node->else_block->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitOutStreamNode(std::shared_ptr<OutStreamNode> node)  {
    // hi
    node->expr->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {

    if (node->func_block) {
        current_func_proc_symbol = node->func_block->containing_scope->resolveProcFunc(node->func_id);
        node->func_block->accept(*this);
        current_func_proc_symbol = nullptr;
    }
    return {};
}


std::any SizeErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

    if (node->proc_block) {
        current_func_proc_symbol = node->proc_block->containing_scope->resolveProcFunc(node->proc_id);
        node->proc_block->accept(*this);
        current_func_proc_symbol = nullptr;
    }
    return {};
}


std::any SizeErrorWalker::visitLengthCallNode(std::shared_ptr<LengthCallNode> node) {
    node->vector->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitRowsCallNode(std::shared_ptr<RowsCallNode> node) {
    node->matrix->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node) {
    node->matrix->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) {
    node->vector->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {

    // size error case #5
    if (not std::dynamic_pointer_cast<VecMatNode>(node->expr))
        return {};

    std::shared_ptr<Type> return_type;
    if (auto proc_sym = std::dynamic_pointer_cast<ProcedureSymbol>(current_func_proc_symbol))
        return_type = proc_sym->type;
    else if (auto func_sym = std::dynamic_pointer_cast<FunctionSymbol>(current_func_proc_symbol))
        return_type = func_sym->type;

    auto return_val = node->expr->accept(*this);
    if (not return_val.has_value())
        return {};

    auto rows_cols = std::any_cast<std::pair<int, int>>(return_val);

    // vector case
    if (std::dynamic_pointer_cast<VectorType>(node->expr->type)) {
        if (auto vector_type = std::dynamic_pointer_cast<VectorType>(return_type)) {
            if (auto size_node = std::dynamic_pointer_cast<IntNode>(vector_type->size)) {
                if (size_node->val != rows_cols.first)
                    throw SizeError(node->line,
                        "return type size does not match function/procedure's defined return type size.");
            }
        }
    }
    // matrix case
    if (std::dynamic_pointer_cast<MatrixType>(node->expr->type)) {
        if (auto matrix_type = std::dynamic_pointer_cast<MatrixType>(return_type)) {
            if (auto rows_node = std::dynamic_pointer_cast<IntNode>(matrix_type->rows)) {
                if (auto cols_node = std::dynamic_pointer_cast<IntNode>(matrix_type->cols)) {
                    if (rows_node->val != rows_cols.first or cols_node->val != rows_cols.second)
                        throw SizeError(node->line,
                            "return type size does not match function/procedure's defined return type size.");
                }
            }
        }
    }
    return {};
}


std::any SizeErrorWalker::visitIndexNode(std::shared_ptr<IndexNode> node) {
    node->collection->accept(*this);
    node->index1->accept(*this);
    if (node->index2)
        node->index2->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitGeneratorNode(std::shared_ptr<GeneratorNode> node) {
    node->vec_expr1->accept(*this);
    if (node->vec_expr2)
        node->vec_expr2->accept(*this);
    return {};
}


std::any SizeErrorWalker::visitFilterNode(std::shared_ptr<FilterNode> node) {
    node->vec_expr->accept(*this);
    return {};
}
