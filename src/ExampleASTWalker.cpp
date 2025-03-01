#include "ExampleASTWalker.h"
#include "iostream"


std::any ExampleASTWalker::visitRootNode(std::shared_ptr<RootNode> node) {

    std::cout << "visiting RootNode!" << std::endl;
    node_counter++;
    auto result = node->global_block->accept(*this);

    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from RootNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {

    std::cout << "visiting DeclNode!" << std::endl;
    node_counter++;
    auto result = node->expr->accept(*this);

    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from DeclNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {

    std::cout << "visiting IdAssignNode!" << std::endl;
    node_counter++;
    node->expr->accept(*this);
    std::string return_val = "return value from IdAssignNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {

    std::cout << "visiting VecMatTupAssignNode!" << std::endl;
    node_counter++;
    node->expr_assign_to->accept(*this);
    node->expr_assign_with->accept(*this);
    std::string return_val = "return value from VecMatTupAssignNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {

    std::cout << "visiting UnpackNode!" << std::endl;
    node_counter++;

    // visit the i-values to unpack to
    for (auto ivalue : node->ivalues) {
        auto result = ivalue->accept(*this);
        std::cout << std::any_cast<std::string>(result) << std::endl;
    }
    // visit the tuple
    auto result = node->tuple->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from UnpackNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitTypeDefNode(std::shared_ptr<TypeDefNode> node) {

    std::cout << "visiting TypeDefNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from TypeDefNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {

    std::cout << "visiting BlockNode!" << std::endl;
    node_counter++;
    for (auto stat : node->stat_nodes) {
        auto result = stat->accept(*this);
        std::cout << std::any_cast<std::string>(result) << std::endl;
    }
    std::string return_val = "return value from BlockNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitBoolNode(std::shared_ptr<BoolNode> node) {

    std::cout << "visiting BoolNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from BoolNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitCharNode(std::shared_ptr<CharNode> node) {

    std::cout << "visiting CharNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from CharNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitIntNode(std::shared_ptr<IntNode> node) {

    std::cout << "visiting IntNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from IntNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitRealNode(std::shared_ptr<RealNode> node) {

    std::cout << "visiting RealNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from RealNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitTupleNode(std::shared_ptr<TupleNode> node) {

    std::cout << "visiting TupleNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from TupleNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {

    std::cout << "visiting UnaryOpNode!" << std::endl;
    node_counter++;

    auto result = node->expr->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from UnaryOpNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {

    std::cout << "visiting BinaryOpNode!" << std::endl;
    node_counter++;

    auto result = node->l_expr->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    result = node->r_expr->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from BinaryOpNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {

    std::cout << "visiting TypeCastNode!" << std::endl;
    node_counter++;
    auto result = node->expr->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;
    std::string return_val = "return value from TypeCastNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {

    std::cout << "visiting InfLoopNode!" << std::endl;
    node_counter++;

    // visit the loop block
    auto result = node->loop_block->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from InfLoopNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

    std::cout << "visiting PredLoopNode!" << std::endl;
    node_counter++;

    // visit the control expression
    auto result = node->contr_expr->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    // visit the loop block
    result = node->loop_block->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from PredLoopNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {

    std::cout << "visiting IterLoopNode!" << std::endl;
    node_counter++;

    // visit the domain expression
    auto result = node->dom_expr->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    // visit the loop block
    result = node->loop_block->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from IterLoopNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitCondNode(std::shared_ptr<CondNode> node) {

    std::cout << "visiting CondNode!" << std::endl;
    node_counter++;

    // visit control expression
    auto result = node->contr_expr->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    // visit "if" block
    result = node->if_block->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    // visit "else" block (if it exists)
    if (node->else_block != nullptr) {
        std::cout << "hi!" << std::endl;
        result = node->else_block->accept(*this);
        std::cout << std::any_cast<std::string>(result) << std::endl;
    }

    std::string return_val = "return value from CondNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitOutStreamNode(std::shared_ptr<OutStreamNode> node) {

    std::cout << "visiting OutStreamNode!" << std::endl;
    node_counter++;

    // visit the expression to be printed
    auto result = node->expr->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from OutStreamNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitInStreamNode(std::shared_ptr<InStreamNode> node) {

    std::cout << "visiting IdInStreamNode!" << std::endl;
    auto result = node->l_value->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;
    std::string return_val = "return value from InStreamNode!";
    return {return_val};
};

std::any ExampleASTWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {

    std::cout << "visiting FuncDefNode!" << std::endl;
    node_counter++;
    if (node->func_block) {
        auto result = node->func_block->accept(*this);
        std::cout << std::any_cast<std::string>(result) << std::endl;
    }
    std::string return_val = "return value from FuncDefNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

    std::cout << "visiting ProcDefNode!" << std::endl;
    node_counter++;
    if (node->proc_block) {
        auto result = node->proc_block->accept(*this);
        std::cout << std::any_cast<std::string>(result) << std::endl;
    }
    std::string return_val = "return value from ProcDefNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {

    std::cout << "visiting FuncProcCallNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from FuncProcCallNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {

    std::cout << "visiting ReturnNode!" << std::endl;
    node_counter++;
    if (node->expr) {
        auto result = node->expr->accept(*this);
        std::cout << std::any_cast<std::string>(result) << std::endl;
    }
    std::string return_val = "return value from ReturnNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitIdNode(std::shared_ptr<IdNode> node) {

    std::cout << "visiting IdNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from IdNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitBreakNode(std::shared_ptr<BreakNode> node) {

    std::cout << "visiting BreakNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from BreakNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitContinueNode(std::shared_ptr<ContinueNode> node) {

    std::cout << "visiting ContinueNode!" << std::endl;
    node_counter++;
    std::string return_val = "return value from ContinueNode!";
    return {return_val};
}


std::any ExampleASTWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {

    std::cout << "visiting TupleAccessNode!" << std::endl;
    node_counter++;

    // visit the id node
    auto result = node->tuple_id->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    // visit the tuple element node
    result = node->element->accept(*this);
    std::cout << std::any_cast<std::string>(result) << std::endl;

    std::string return_val = "return value from TupleAccessNode!";
    return {return_val};
}
