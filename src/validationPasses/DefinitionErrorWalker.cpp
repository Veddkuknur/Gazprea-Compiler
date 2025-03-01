#include "DefinitionErrorWalker.h"

std::any DefinitionErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    node->global_block->accept(*this);

    // ensure that all subroutines have definitions
    for (const auto& proc_func : proc_func_map) {
        if (proc_func.second.has_definition == false)
            throw DefinitionError(proc_func.second.line, "Subroutine with name \'" +
                proc_func.first + "\' doesn't have a definition");
    }
    return {};
}


std::any DefinitionErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

  for (const auto& stat : node->stat_nodes)
      stat->accept(*this);
  return {};
}


std::any DefinitionErrorWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {

    // if we are seeing this subroutine id for the first time, mark it as a function
    if (proc_func_map.find(node->func_id) == proc_func_map.end()) {
        proc_func_map[node->func_id].is_func = true;
    }
    // prototype case
    if (!node->func_block) {

        // if a subroutine with this id already has a prototype or definition, throw an error
        if (proc_func_map[node->func_id].has_prototype or proc_func_map[node->func_id].has_definition)
            throw DefinitionError(node->line, "Forward Declaration " + node->func_id + " is already declared");

        proc_func_map[node->func_id].line = node->line;
        proc_func_map[node->func_id].is_func = true;
        proc_func_map[node->func_id].has_prototype = true;
        proc_func_map[node->func_id].return_type = node->return_type;
        return {};
    }
    // definition case
    if (proc_func_map[node->func_id].has_definition)
        throw DefinitionError(node->line, "Function " + node->func_id + " is already defined");

    // check if this function already has a prototype, and that the prototype is a function, not a procedure
    if (proc_func_map[node->func_id].has_prototype and not proc_func_map[node->func_id].is_func)
        throw DefinitionError(node->line, "Subroutine is already defined with a different type");

    // check if this function already has a prototype, and that the prototype's return value matches the definition's
    if (proc_func_map[node->func_id].has_prototype and
        not proc_func_map[node->func_id].return_type->isEqualType(node->return_type))
        throw DefinitionError(node->line, "Subroutine is already defined with a different type");

    proc_func_map[node->func_id].line = node->line;
    proc_func_map[node->func_id].is_func = true;
    proc_func_map[node->func_id].has_definition = true;
    // we don't need to store the return type at this point, an error will be thrown if another def/proto is attempted
    return {};
}


std::any DefinitionErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

    // if we are seeing this subroutine id for the first time, mark it as a procedure
    if (proc_func_map.find(node->proc_id) == proc_func_map.end()) {
        proc_func_map[node->proc_id].is_func = false;
    }
    // prototype case
    if (!node->proc_block) {

        // if a subroutine with this id already has a prototype or definition, throw an error
        if (proc_func_map[node->proc_id].has_prototype or proc_func_map[node->proc_id].has_definition)
            throw DefinitionError(node->line, "Forward Declaration " + node->proc_id + " is already declared");

        proc_func_map[node->proc_id].line = node->line;
        proc_func_map[node->proc_id].is_func = false;
        proc_func_map[node->proc_id].has_prototype = true;
        proc_func_map[node->proc_id].return_type = node->return_type;
        for (const auto& param : node->params) {
            proc_func_map[node->proc_id].param_qualifiers.push_back(std::get<0>(param));
        }
        return {};
    }
    // definition case
    if (proc_func_map[node->proc_id].has_definition)
        throw DefinitionError(node->line, "Procedure " + node->proc_id + " is already defined");

    // handle cases where prototype exists
    if (proc_func_map[node->proc_id].has_prototype) {

        // ensure that the prototype is a procedure, not a function
        if (proc_func_map[node->proc_id].is_func)
            throw DefinitionError(node->line, "Subroutine is already defined with a different type");

        // does the return type in the prototype match the return type in the definition
        if (proc_func_map[node->proc_id].return_type == nullptr) {
            if (node->return_type != nullptr)
                throw DefinitionError(node->line, "Subroutine is already defined with a different type");
        }
        else if (not proc_func_map[node->proc_id].return_type->isEqualType(node->return_type))
                throw DefinitionError(node->line, "Subroutine is already defined with a different type");

        // ensure parameter qualifiers match
        for (int i = 0; i < static_cast<int>(node->params.size()); i++) {
            if (std::get<0>(node->params[i]) != proc_func_map[node->proc_id].param_qualifiers[i])
                throw TypeError(node->line, "Procedure " + node->proc_id +
                    " is already defined with a different type");
        }
    }

    proc_func_map[node->proc_id].line = node->line;
    proc_func_map[node->proc_id].is_func = false;
    proc_func_map[node->proc_id].has_definition = true;
    // we don't need to store the return type at this point, an error will be thrown if another def/proto is attempted
    return {};
}