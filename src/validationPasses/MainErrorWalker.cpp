#include "MainErrorWalker.h"


std::any MainErrorWalker::visitRootNode(std::shared_ptr<RootNode> node) {
    bool has_main_proc =  std::any_cast<bool>(node->global_block->accept(*this));
    if (not has_main_proc)
      throw MainError(0, "program does not have a main procedure");
    return{};
}


std::any MainErrorWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

    for (auto stat : node->stat_nodes) {

        // only visit the procedure definition statement nodes
        if (std::dynamic_pointer_cast<ProcDefNode>(stat)) {
            // if the node we just visited was main, we can exit early
            bool was_main = std::any_cast<bool>(stat->accept(*this));

            if (was_main)
                return was_main;
        }
    }
    return false;
}


std::any MainErrorWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

    bool is_main = false;

    // check if the name of the procedure is "main", if not, return early
    if (node->proc_id != "main")
        return is_main;

    is_main = true;

    if (!node->params.empty())
        throw MainError(node->line, "main procedure has invalid signature (expects no parameters)");

    // the return type should be an integer,
    if (node->return_type == nullptr)
        throw MainError(node->line, "main procedure has invalid signature (missing return type)");
    if (auto type_ptr = std::dynamic_pointer_cast<PrimitiveType>(node->return_type)) {
        if (type_ptr->base_type != Type::BaseType::integer)
            throw MainError(node->line, "main procedure has invalid signature (invalid return type)");
    }
    return is_main;
}
