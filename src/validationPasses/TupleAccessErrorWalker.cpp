#include "TupleAccessErrorWalker.h"
#include "iostream"
#include "Symbol.h"


std::any TupleAccessErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    auto result = node->global_block->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {
    if (node->expr){
        node->expr->accept(*this);
    }
    return {};
}


std::any TupleAccessErrorWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {
    if (node->expr){
        node->expr->accept(*this);
    }
    return {};
}


std::any TupleAccessErrorWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {
    node->expr_assign_to->accept(*this);
    node->expr_assign_with->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {

    // visit the i-values to unpack to
    for (auto ivalue : node->ivalues) {
        auto result = ivalue->accept(*this);
    }
    // visit the tuple
    auto result = node->tuple->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
    for (auto stat : node->stat_nodes) {
        auto result = stat->accept(*this);
    }
    return {};
}


std::any TupleAccessErrorWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {
    auto result = node->expr->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {
    auto result = node->l_expr->accept(*this);
    result = node->r_expr->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {
    auto result = node->expr->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
    // visit the loop block
    auto result = node->loop_block->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

    // visit the control expression
    auto result = node->contr_expr->accept(*this);

    // visit the loop block
    result = node->loop_block->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

    // visit the domain expression
    auto result = node->dom_expr->accept(*this);

    // visit the loop block
    result = node->loop_block->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {

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


std::any TupleAccessErrorWalker::visitOutStreamNode(std::shared_ptr<OutStreamNode> node) {

    // visit the expression to be printed
    auto result = node->expr->accept(*this);
    return {};
}


std::any TupleAccessErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
    if (node->func_block) {
        auto result = node->func_block->accept(*this);
    }
    return {};
}


std::any TupleAccessErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    if (node->proc_block) {
        auto result = node->proc_block->accept(*this);
    }
    return {};
}


std::any TupleAccessErrorWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {
    for (auto node: node->params){
        node->accept(*this);
    }
    return {};
}


std::any TupleAccessErrorWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {
    if (node->expr) {
        auto result = node->expr->accept(*this);
    }
    return {};
}


std::any TupleAccessErrorWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {

    // visit the id node
    auto result = node->tuple_id->accept(*this);

    // visit the tuple element node
    node->element->accept(*this);

    auto tup_symbol = node->containing_scope->resolve(node->tuple_id->id);
    auto tup_type = std::dynamic_pointer_cast<TupleType>(tup_symbol->type);
    auto size_of_tuple = tup_type->element_types.size();

    // visit the tuple element node
    if (auto int_node = std::dynamic_pointer_cast<IntNode>(node->element)) {
        if (int_node->val > static_cast<int>(size_of_tuple) or int_node->val < 1) {
            throw SymbolError(int_node->line, "integer '" + std::to_string(int_node->val)  +"' value is out of range.");
        }
    } else if (auto id_node = std::dynamic_pointer_cast<IdNode>(node->element)) {
        if (std::find(tup_type->element_names.begin(), tup_type->element_names.end(),id_node->id) == tup_type->element_names.end()){
            auto err_desc = "'" + id_node->id+"' is not a member in tuple";
            throw SymbolError(node->line, err_desc);
        }
    }
    return {};
}

