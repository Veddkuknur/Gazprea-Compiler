#include "AST.h"
#include "ASTWalker.h"


std::any DeclNode::accept(ASTWalker& walker) {

    return walker.visitDeclNode(shared_from_this());

}


std::any IdAssignNode::accept(ASTWalker& walker) {
    return walker.visitIdAssignNode(shared_from_this());
}


std::any VecMatTupAssignNode::accept(ASTWalker &walker) {
    return walker.visitVecMatTupAssignNode(shared_from_this());
}


std::any UnpackNode::accept(ASTWalker &walker) {
    return walker.visitUnpackNode(shared_from_this());
}


std::any VecMatNode::accept(ASTWalker &walker) {
    return walker.visitVecMatNode(shared_from_this());
}


std::any TypeDefNode::accept(ASTWalker& walker) {
    return walker.visitTypeDefNode(shared_from_this());
}


std::any BlockNode::accept(ASTWalker& walker) {
    return walker.visitBlockNode(shared_from_this());
}


std::any RootNode::accept(ASTWalker& walker) {
    return walker.visitRootNode(shared_from_this());
}


std::any BoolNode::accept(ASTWalker& walker) {
    return walker.visitBoolNode(shared_from_this());
}


std::any RangeNode::accept(ASTWalker& walker) {
    return walker.visitRangeNode(shared_from_this());
}


std::any CharNode::accept(ASTWalker& walker) {
    return walker.visitCharNode(shared_from_this());
}


std::any IntNode::accept(ASTWalker& walker) {
    return walker.visitIntNode(shared_from_this());
}


std::any RealNode::accept(ASTWalker& walker) {
    return walker.visitRealNode(shared_from_this());
}


std::any TupleNode::accept(ASTWalker& walker) {
    return walker.visitTupleNode(shared_from_this());
}


std::any UnaryOpNode::accept(ASTWalker& walker) {
    return walker.visitUnaryOpNode(shared_from_this());
}


std::any BinaryOpNode::accept(ASTWalker& walker) {
    return walker.visitBinaryOpNode(shared_from_this());
}


std::any TypeCastNode::accept(ASTWalker& walker) {
    return walker.visitTypeCastNode(shared_from_this());
}


std::any InfLoopNode::accept(ASTWalker& walker) {
    return walker.visitInfLoopNode(shared_from_this());
}


std::any PredLoopNode::accept(ASTWalker& walker) {
    return walker.visitPredLoopNode(shared_from_this());
}


std::any IterLoopNode::accept(ASTWalker& walker) {
    return walker.visitIterLoopNode(shared_from_this());
}


std::any CondNode::accept(ASTWalker& walker) {
    return walker.visitCondNode(shared_from_this());
}


std::any OutStreamNode::accept(ASTWalker& walker) {
    return walker.visitOutStreamNode(shared_from_this());
}


std::any InStreamNode::accept(ASTWalker& walker) {
    return walker.visitInStreamNode(shared_from_this());
}


std::any FuncDefNode::accept(ASTWalker& walker) {
    return walker.visitFuncDefNode(shared_from_this());
}


std::any ProcDefNode::accept(ASTWalker& walker) {
    return walker.visitProcDefNode(shared_from_this());
}


std::any FuncProcCallNode::accept(ASTWalker& walker) {
    return walker.visitFuncProcCallNode(shared_from_this());
}


std::any LengthCallNode::accept(ASTWalker& walker) {
    return walker.visitLengthCallNode(shared_from_this());
}


std::any RowsCallNode::accept(ASTWalker& walker) {
    return walker.visitRowsCallNode(shared_from_this());
}


std::any ColumnsCallNode::accept(ASTWalker& walker) {
    return walker.visitColumnsCallNode(shared_from_this());
}


std::any ReverseCallNode::accept(ASTWalker& walker) {
    return walker.visitReverseCallNode(shared_from_this());
}


std::any FormatCallNode::accept(ASTWalker& walker) {
    return walker.visitFormatCallNode(shared_from_this());
}


std::any StreamStateCallNode::accept(ASTWalker& walker) {
    return walker.visitStreamStateCallNode(shared_from_this());
}


std::any ReturnNode::accept(ASTWalker& walker) {
    return walker.visitReturnNode(shared_from_this());
}


std::any IdNode::accept(ASTWalker& walker) {
    return walker.visitIdNode(shared_from_this());
}


std::any BreakNode::accept(ASTWalker& walker) {
    return walker.visitBreakNode(shared_from_this());
}


std::any ContinueNode::accept(ASTWalker& walker) {
    return walker.visitContinueNode(shared_from_this());
}


std::any TupleAccessNode::accept(ASTWalker& walker) {
    return walker.visitTupleAccessNode(shared_from_this());
}


std::any IndexNode::accept(ASTWalker& walker) {
    return walker.visitIndexNode(shared_from_this());
}


std::any GeneratorNode::accept(ASTWalker& walker) {
    return walker.visitGeneratorNode(shared_from_this());
}


std::any FilterNode::accept(ASTWalker& walker) {
    return walker.visitFilterNode(shared_from_this());
}


std::any FreeNode::accept(ASTWalker& walker) {
    return walker.visitFreeNode(shared_from_this());
}