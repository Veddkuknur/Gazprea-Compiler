#include "ScopeToFreeWalker.h"

#include <ScopedSymbol.h>


std::any ScopeToFreeWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    global_scope = node->containing_scope;
    node->global_block->accept(*this);
    return {};
}


std::any ScopeToFreeWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {

    auto symbol = node->containing_scope->resolve(node->id);
    auto base_scope = std::dynamic_pointer_cast<BaseScope>(node->containing_scope);

    // if we are in the main function, add the symbol to the global scope's symbols to free list
    // if this decl is in an inner scope inside main, we don't add it the symbol to the global free list
    if (in_main_procedure and not in_nested_scope)
        base_scope = std::dynamic_pointer_cast<BaseScope>(global_scope);


    if (std::dynamic_pointer_cast<VectorType>(symbol->type) or
        std::dynamic_pointer_cast<MatrixType>(symbol->type) or
        std::dynamic_pointer_cast<TupleType>(symbol->type)) {

        base_scope->symbols_to_free.push_back(symbol);
    }
    return {};
}


std::any ScopeToFreeWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
    for (const auto& stat : node->stat_nodes) {
        stat->accept(*this);
    }
    return {};
}


std::any ScopeToFreeWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {

    loop_scope_stack.push(node->loop_block->containing_scope);
    in_nested_scope = true;
    node->loop_block->accept(*this);
    in_nested_scope = false;
    loop_scope_stack.pop();
    return {};
}


std::any ScopeToFreeWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

    loop_scope_stack.push(node->loop_block->containing_scope);
    in_nested_scope = true;
    node->loop_block->accept(*this);
    in_nested_scope = false;
    loop_scope_stack.pop();
    return {};
}


std::any ScopeToFreeWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

    loop_scope_stack.push(node->loop_block->containing_scope);
    in_nested_scope = true;
    node->loop_block->accept(*this);
    in_nested_scope = false;
    loop_scope_stack.pop();
    return {};
}


std::any ScopeToFreeWalker::visitCondNode(std::shared_ptr<CondNode> node) {
    node->if_block->accept(*this);
    if (node->else_block) {
        in_nested_scope = true;
        node->else_block->accept(*this);
        in_nested_scope = false;
    }
    return {};
}


std::any ScopeToFreeWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {
    if (node->func_block) {
        func_proc_scope = node->func_block->containing_scope;
        node->func_block->accept(*this);
    }
    return {};
}


std::any ScopeToFreeWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    if (node->proc_id == "main")
        in_main_procedure = true;
    if (node->proc_block) {
        std::shared_ptr<BaseScope> base_scope;
        if (in_main_procedure)
            base_scope = std::dynamic_pointer_cast<BaseScope>(global_scope);
        else
            base_scope = std::dynamic_pointer_cast<BaseScope>(node->proc_block->containing_scope);

        // add all the pass by value malloc'd objects to the free list
        auto proc_symbol = std::dynamic_pointer_cast<ProcedureSymbol>(
            node->containing_scope->resolveProcFunc(node->proc_id));

        for (const auto& param_symbol : proc_symbol->orderedArgs) {
            if (param_symbol->mutability == false) {
                if (std::dynamic_pointer_cast<VectorType>(param_symbol->type) or
                    std::dynamic_pointer_cast<MatrixType>(param_symbol->type) or
                    std::dynamic_pointer_cast<TupleType>(param_symbol->type)) {

                    base_scope->symbols_to_free.push_back(param_symbol);
                }
            }
        }
        func_proc_scope = node->proc_block->containing_scope;
        node->proc_block->accept(*this);
    }
    in_main_procedure = false;
    return {};
}


std::any ScopeToFreeWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {
    node->free_node->accept(*this);
    return {};
}


std::any ScopeToFreeWalker::visitFreeNode(std::shared_ptr<FreeNode> node) {
    if (node->scope_id == 0)
        node->scope_to_free_up_to = node->containing_scope;
    else if (node->scope_id == 1)
        node->scope_to_free_up_to = func_proc_scope;
    else if (node->scope_id == 2)
        node->scope_to_free_up_to = loop_scope_stack.top();
    else if (node->scope_id == 3)
        node->scope_to_free_up_to = global_scope;
    return {};
}