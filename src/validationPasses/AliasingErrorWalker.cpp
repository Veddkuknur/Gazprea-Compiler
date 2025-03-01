#include "AliasingErrorWalker.h"

#include <ScopedSymbol.h>

struct ParamCounter {
    int var_count = 0;
    int last_var_arg = -1;
};

std::any AliasingErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    node->global_block->accept(*this);
    return {};
}


std::any AliasingErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

    for (const auto& stat : node->stat_nodes)
        stat->accept(*this);
    return {};
}


std::any AliasingErrorWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any AliasingErrorWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any AliasingErrorWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
    node->loop_block->accept(*this);
    return {};
}


std::any AliasingErrorWalker::visitCondNode(std::shared_ptr<CondNode> node) {

    node->if_block->accept(*this);
    if (node->else_block)
        node->else_block->accept(*this);
    return {};
}


std::any AliasingErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {
    if (node->proc_block)
        node->proc_block->accept(*this);
    return {};
}


std::any AliasingErrorWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {

    auto symbol = std::dynamic_pointer_cast<ProcedureSymbol>(node->containing_scope->resolveProcFunc(node->id));
    if (symbol == nullptr)
        return {};

    std::unordered_map<std::string, ParamCounter> param_map;
    for (int i = 0; i < static_cast<int>(node->params.size()); i++) {

        std::string param_object_id;
        std::string tuple_id;

        bool check_for_aliasing = false;
        // id case
        if (auto id_param = std::dynamic_pointer_cast<IdNode>(node->params[i])) {
            check_for_aliasing = true;
            param_object_id = id_param->id;
        }
        // index case (vector/matrix)
        else if (auto index_param = std::dynamic_pointer_cast<IndexNode>(node->params[i])) {
            if (auto id_node = std::dynamic_pointer_cast<IdNode>(index_param->collection)) {
                check_for_aliasing = true;
                // check if we are passing an entire tuple object
                param_object_id = id_node->id;
            }
        }
        // tuple access case
        else if (auto tup_access_param = std::dynamic_pointer_cast<TupleAccessNode>(node->params[i])) {
            check_for_aliasing = true;
            tuple_id = tup_access_param->tuple_id->id; // save this in case the entire tuple was passed as an arg

            // get the access value (int or id)
            if (auto int_access_node = std::dynamic_pointer_cast<IntNode>(tup_access_param->element)) {
                param_object_id = std::to_string(int_access_node->val) + tup_access_param->tuple_id->id;
            }
            else { // id, get the tuple element number corresponding to the access id
                auto id_access_node = std::dynamic_pointer_cast<IdNode>(tup_access_param->element);
                auto tuple_symbol = node->containing_scope->resolve(tup_access_param->tuple_id->id);
                auto tuple_type = std::dynamic_pointer_cast<TupleType>(tuple_symbol->type);
                int elem_num = findFirstInstanceStringVector(tuple_type->element_names, id_access_node->id);
                param_object_id = std::to_string(elem_num) + tup_access_param->tuple_id->id;
            }
        }
        // aliasing check (only if necessary)
        if (check_for_aliasing)
            if (param_map[param_object_id].var_count > 0)
                throw AliasingError(node->line, "Argument " +
                    std::to_string(param_map[param_object_id].last_var_arg + 1) + " is aliasing with " +
                    symbol->orderedArgs[i]->name);
        if (param_map[tuple_id].var_count > 0)
            throw AliasingError(node->line, "Argument " +
                std::to_string(param_map[tuple_id].last_var_arg + 1) + " is aliasing with " +
                symbol->orderedArgs[i]->name);
        if (tuple_id.empty() and param_map['?' + tuple_id].var_count > 0)
            throw AliasingError(node->line, "Aliasing detected");

        if (symbol->orderedArgs[i]->mutability == true) {
            param_map[param_object_id].var_count++;
            param_map[param_object_id].last_var_arg = i;
            if (!tuple_id.empty()) {
                param_map['?' + tuple_id].var_count++;      // prefix '?' means that this tuple has had one of it's
                param_map['?' + tuple_id].last_var_arg = i; // elements passed as a var already
            }
        }
    }
    return {};
}


int AliasingErrorWalker::findFirstInstanceStringVector(std::vector<std::string> vector, std::string string) {

    for (int i = 0; i < static_cast<int>(vector.size()); i++) {
        if (vector[i].find(string) != std::string::npos) {
            return i+1;
        }
    }
    return -1;
}

