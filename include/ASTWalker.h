#ifndef ASTWALKER_H
#define ASTWALKER_H

#include "AST.h"

#include <any>

class ASTWalker {
public:
  virtual ~ASTWalker() = default;

  virtual std::any visitRootNode(std::shared_ptr<RootNode> node) = 0;
  virtual std::any visitDeclNode(std::shared_ptr<DeclNode> node) = 0;
  virtual std::any visitIdAssignNode(std::shared_ptr<IdAssignNode> node) = 0;
  virtual std::any visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) = 0;
  virtual std::any visitUnpackNode(std::shared_ptr<UnpackNode> node) = 0;
  virtual std::any visitTypeDefNode(std::shared_ptr<TypeDefNode> node) = 0;
  virtual std::any visitBlockNode(std::shared_ptr<BlockNode> node) = 0;
  virtual std::any visitBoolNode(std::shared_ptr<BoolNode> node) = 0;
  virtual std::any visitCharNode(std::shared_ptr<CharNode> node) = 0;
  virtual std::any visitIntNode(std::shared_ptr<IntNode> node) = 0;
  virtual std::any visitRealNode(std::shared_ptr<RealNode> node) = 0;
  virtual std::any visitTupleNode(std::shared_ptr<TupleNode> node) = 0;
  virtual std::any visitVecMatNode(std::shared_ptr<VecMatNode> node) = 0;
  virtual std::any visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) = 0;
  virtual std::any visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) = 0;
  virtual std::any visitTypeCastNode(std::shared_ptr<TypeCastNode> node) = 0;
  virtual std::any visitInfLoopNode(std::shared_ptr<InfLoopNode> node) = 0;
  virtual std::any visitPredLoopNode(std::shared_ptr<PredLoopNode> node) = 0;
  virtual std::any visitIterLoopNode(std::shared_ptr<IterLoopNode> node) = 0;
  virtual std::any visitCondNode(std::shared_ptr<CondNode> node) = 0;
  virtual std::any visitOutStreamNode(std::shared_ptr<OutStreamNode> node) = 0;
  virtual std::any visitInStreamNode(std::shared_ptr<InStreamNode> node) = 0;
  virtual std::any visitFuncDefNode(std::shared_ptr<FuncDefNode> node) = 0;
  virtual std::any visitProcDefNode(std::shared_ptr<ProcDefNode> node) = 0;
  virtual std::any visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) = 0;
  virtual std::any visitLengthCallNode(std::shared_ptr<LengthCallNode> node) = 0;
  virtual std::any visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) = 0;
  virtual std::any visitFormatCallNode(std::shared_ptr<FormatCallNode> node) = 0;
  virtual std::any visitRowsCallNode(std::shared_ptr<RowsCallNode> node) = 0;
  virtual std::any visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node) = 0;
  virtual std::any visitStreamStateCallNode(std::shared_ptr<StreamStateCallNode> node) = 0;
  virtual std::any visitReturnNode(std::shared_ptr<ReturnNode> node) = 0;
  virtual std::any visitIdNode(std::shared_ptr<IdNode> node) = 0;
  virtual std::any visitBreakNode(std::shared_ptr<BreakNode> node) = 0;
  virtual std::any visitContinueNode(std::shared_ptr<ContinueNode> node) = 0;
  virtual std::any visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) = 0;
  virtual std::any visitIndexNode(std::shared_ptr<IndexNode> node) = 0;
  virtual std::any visitRangeNode(std::shared_ptr<RangeNode> node) = 0;
  virtual std::any visitGeneratorNode(std::shared_ptr<GeneratorNode> node) = 0;
  virtual std::any visitFilterNode(std::shared_ptr<FilterNode> node) = 0;
  virtual std::any visitFreeNode(std::shared_ptr<FreeNode> node) = 0;
};

#endif //ASTWALKER_H
