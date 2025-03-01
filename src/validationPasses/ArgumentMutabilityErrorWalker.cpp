#include "ArgumentMutabilityErrorWalker.h"

#include <ScopedSymbol.h>


std::any ArgumentMutabilityErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {

    node->global_block->accept(*this);
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

    for (const auto& stat : node->stat_nodes) {
        stat->accept(*this);
    }
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {

    node->loop_block->accept(*this);
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

    node->loop_block->accept(*this);
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

    node->loop_block->accept(*this);
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {

    node->if_block->accept(*this);
    if (node->else_block)
        node->else_block->accept(*this);
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    if (node->proc_block)
        node->proc_block->accept(*this);
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {

    auto symbol = std::dynamic_pointer_cast<ProcedureSymbol>(node->containing_scope->resolveProcFunc(node->id));
    if (symbol == nullptr)
        return {};

    for (int i = 0; i < static_cast<int>(symbol->orderedArgs.size()); i++) {
        if (symbol->orderedArgs[i]->mutability == false)
            continue;

        node->params[i]->accept(*this);
        if (not might_be_lvalue)
            throw TypeError(node->line, "l-value must be given to a var procedure call");

        // id case
        if (auto id_arg = std::dynamic_pointer_cast<IdNode>(node->params[i])) {
            auto id_arg_symbol = id_arg->containing_scope->resolve(id_arg->id);
            if (id_arg_symbol->mutability == false)
                throw TypeError(node->line, "l-value must be given to a var procedure call");

            // if the id holds an entire tuple, check for type promotion in the tuple elements
            if (auto arg_type = std::dynamic_pointer_cast<TupleType>(id_arg_symbol->type)) {
                auto param_type = std::dynamic_pointer_cast<TupleType>(symbol->orderedArgs[i]->type);
                for (int j = 0; j < static_cast<int>(arg_type->element_types.size()); j++) {
                    if (arg_type->element_types[j]->getBaseType() != param_type->element_types[j]->getBaseType())
                        throw TypeError(node->line, "l-value must be given to a var procedure call");
                }
            }
        }
        // index case
        else if (auto index_arg = std::dynamic_pointer_cast<IndexNode>(node->params[i])) {
            auto index_arg_id = std::dynamic_pointer_cast<IdNode>(index_arg->collection);
            if (index_arg->containing_scope->resolve(index_arg_id->id)->mutability == false)
                throw TypeError(node->line, "l-value must be given to a var procedure call");

            // check for type promotion
            auto param_base_type = symbol->orderedArgs[i]->type->getBaseType();
            if (param_base_type != index_arg_id->type->getBaseType())
                throw TypeError(node->line, "l-value must be given to a var procedure call");
        }
        // tuple access case
        else if (auto tup_access_arg = std::dynamic_pointer_cast<TupleAccessNode>(node->params[i])) {
            auto tup_access_arg_symbol = tup_access_arg->containing_scope->resolve(tup_access_arg->tuple_id->id);
            if (tup_access_arg_symbol->mutability == false)
                throw TypeError(node->line, "l-value must be given to a var procedure call");

            // check for type promotion
            int elem_num;
            auto param_tuple_type = std::dynamic_pointer_cast<TupleType>(
                tup_access_arg_symbol->type);
            if (auto elem_alias = std::dynamic_pointer_cast<IdNode>(tup_access_arg->element)) {
                elem_num = findFirstInstanceStringVector(param_tuple_type->element_names, elem_alias->id);
            }
            else
                elem_num = std::dynamic_pointer_cast<IntNode>(tup_access_arg->element)->val;
            auto proc_tuple_type = std::dynamic_pointer_cast<TupleType>(symbol->orderedArgs[i]->type);
            if (param_tuple_type->element_types[elem_num] != proc_tuple_type->element_types[elem_num])
                throw TypeError(node->line, "l-value must be given to a var procedure call");
        }
        might_be_lvalue = false;
    }

    return {};
}


std::any ArgumentMutabilityErrorWalker::visitIdNode(std::shared_ptr<IdNode> node) {
    might_be_lvalue = true;
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {
    might_be_lvalue = true;
    return {};
}


std::any ArgumentMutabilityErrorWalker::visitIndexNode(std::shared_ptr<IndexNode> node) {
    if (std::dynamic_pointer_cast<IdNode>(node->collection))
        might_be_lvalue = true;
    return {};
}


int ArgumentMutabilityErrorWalker::findFirstInstanceStringVector(std::vector<std::string> vector, std::string string) {

    for (int i = 0; i < static_cast<int>(vector.size()); i++) {
        if (vector[i].find(string) != std::string::npos) {
            return i+1;
        }
    }
    return -1;
}