#ifndef CONSTANTFOLDINGWALKER_H
#define CONSTANTFOLDINGWALKER_H

#include "BaseASTWalker.h"


class ConstantFoldingWalker : public BaseASTWalker {
public:

    std::any visitRootNode(std::shared_ptr<RootNode> node) override;
    std::any visitDeclNode(std::shared_ptr<DeclNode> node) override;
//    std::any visitIdAssignNode(std::shared_ptr<IdAssignNode> node) override;
//    std::any visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) override;
//    std::any visitUnpackNode(std::shared_ptr<UnpackNode> node) override { return {}; }
//    std::any visitTypeDefNode(std::shared_ptr<TypeDefNode> node) override { return {}; }
    std::any visitBlockNode(std::shared_ptr<BlockNode> node) override;
//    std::any visitBoolNode(std::shared_ptr<BoolNode> node) override { return {}; }
//    std::any visitCharNode(std::shared_ptr<CharNode> node) override { return {}; }
    std::any visitIntNode(std::shared_ptr<IntNode> node) override;
//    std::any visitRealNode(std::shared_ptr<RealNode> node) override;
//    std::any visitTupleNode(std::shared_ptr<TupleNode> node) override;
//    std::any visitVecMatNode(std::shared_ptr<VecMatNode> node) override;
    std::any visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) override;
    std::any visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) override;
    std::any visitTypeCastNode(std::shared_ptr<TypeCastNode> node) override;
    std::any visitInfLoopNode(std::shared_ptr<InfLoopNode> node) override;
    std::any visitPredLoopNode(std::shared_ptr<PredLoopNode> node) override;
    std::any visitIterLoopNode(std::shared_ptr<IterLoopNode> node) override;
    std::any visitCondNode(std::shared_ptr<CondNode> node) override;
//    std::any visitOutStreamNode(std::shared_ptr<OutStreamNode> node) override { return {}; }
//    std::any visitInStreamNode(std::shared_ptr<InStreamNode> node) override { return {}; }
    std::any visitFuncDefNode(std::shared_ptr<FuncDefNode> node) override;
    std::any visitProcDefNode(std::shared_ptr<ProcDefNode> node) override;
//    std::any visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) override { return {}; }
//    std::any visitLengthCallNode(std::shared_ptr<LengthCallNode> node) override { return {}; }
//    std::any visitRowsCallNode(std::shared_ptr<RowsCallNode> node) override { return {}; }
//    std::any visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node) override { return {}; }
//    std::any visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) override { return {}; }
//    std::any visitFormatCallNode(std::shared_ptr<FormatCallNode> node) override { return {}; }
//    std::any visitStreamStateCallNode(std::shared_ptr<StreamStateCallNode> node) override { return {}; }
//    std::any visitReturnNode(std::shared_ptr<ReturnNode> node) override { return {}; }
//    std::any visitIdNode(std::shared_ptr<IdNode> node) override { return {}; }
//    std::any visitBreakNode(std::shared_ptr<BreakNode> node) override { return {}; }
//    std::any visitContinueNode(std::shared_ptr<ContinueNode> node) override { return {}; }
//    std::any visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) override { return {}; }
//    std::any visitIndexNode(std::shared_ptr<IndexNode> node) override;
//    std::any visitRangeNode(std::shared_ptr<RangeNode> node) override;
//    std::any visitGeneratorNode(std::shared_ptr<GeneratorNode> node) override { return {}; }
//    std::any visitFilterNode(std::shared_ptr<FilterNode> node) override { return {}; }
//    std::any visitFreeNode(std::shared_ptr<FreeNode> node) override { return {}; }
};

#endif //CONSTANTFOLDINGWALKER_H
