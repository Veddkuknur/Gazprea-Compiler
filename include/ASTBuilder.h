#ifndef ASTBUILDER_H
#define ASTBUILDER_H

#include "GazpreaBaseVisitor.h"
#include "AST.h"
#include "Type.h"
#include "CompileTimeExceptions.h"

using namespace gazprea;

class ASTBuilder : public GazpreaBaseVisitor {
public:
    std::shared_ptr<RootNode> ast_root;
    std::map<std::string, std::shared_ptr<Type>> typedefMap;
    std::shared_ptr<RootNode> getRoot();
    std::shared_ptr<Type> stringToBaseType(const std::string& string_to_convert);
    std::shared_ptr<Type> declTypeToType(GazpreaParser::DeclTypeContext* ctx, int line);

    static std::shared_ptr<ASTNode> createLengthCallNode(const std::shared_ptr<ASTNode>& vector, int line, int col);
    static std::shared_ptr<ASTNode> createRowsCallNode(const std::shared_ptr<ASTNode>& matrix, int line, int col);
    static std::shared_ptr<ASTNode> createColumnsCallNode(const std::shared_ptr<ASTNode>& matrix, int line, int col);
    static std::shared_ptr<ASTNode> createFormatNode(const std::shared_ptr<ASTNode>& scalar, int line, int col);
    static std::shared_ptr<ASTNode> createStreamStateNode(int line, int col);
    static std::shared_ptr<ASTNode> createReverseNode(const std::shared_ptr<ASTNode>& vector, int line, int col);

    std::any visitFile(GazpreaParser::FileContext* ctx) override;
    std::any visitBlock(GazpreaParser::BlockContext* ctx) override;
    std::any visitBlockStat(GazpreaParser::BlockStatContext *ctx) override;
    std::any visitParens(GazpreaParser::ParensContext* ctx) override;
    std::any visitBool(GazpreaParser::BoolContext* ctx) override;
    std::any visitString(GazpreaParser::StringContext* ctx) override;
    std::any visitDot(GazpreaParser::DotContext* ctx) override;
    std::any visitRange(GazpreaParser::RangeContext* ctx) override;
    std::any visitRangeRealStartRealEnd(GazpreaParser::RangeRealStartRealEndContext *ctx) override;
    std::any visitRangeIntStart(GazpreaParser::RangeIntStartContext *ctx) override;
    std::any visitUnary(GazpreaParser::UnaryContext* ctx) override;
    std::any visitTupl(GazpreaParser::TuplContext *ctx) override;
    std::any visitFuncProcCall(GazpreaParser::FuncProcCallContext* ctx) override;
    std::any visitGen(GazpreaParser::GenContext* ctx) override;
    std::any visitAnd(GazpreaParser::AndContext* ctx) override;
    std::any visitBy(GazpreaParser::ByContext *ctx) override;
    std::any visitVecMatLit(GazpreaParser::VecMatLitContext* ctx) override;
    std::any visitId(GazpreaParser::IdContext* ctx) override;
    std::any visitExp(GazpreaParser::ExpContext* ctx) override;
    std::any visitFilt(GazpreaParser::FiltContext* ctx) override;
    std::any visitBuiltInFunc(GazpreaParser::BuiltInFuncContext* ctx) override;
    std::any visitLtGt(GazpreaParser::LtGtContext* ctx) override;
    std::any visitOrXor(GazpreaParser::OrXorContext* ctx) override;
    std::any visitIndex(GazpreaParser::IndexContext* ctx) override;
    std::any visitAddSub(GazpreaParser::AddSubContext* ctx) override;
    std::any visitReal(GazpreaParser::RealContext* ctx) override;
    std::any visitConcat(GazpreaParser::ConcatContext* ctx) override;
    std::any visitEqNe(GazpreaParser::EqNeContext* ctx) override;
    std::any visitInt(GazpreaParser::IntContext* ctx) override;
    std::any visitTypeCast(GazpreaParser::TypeCastContext* ctx) override;
    std::any visitChar(GazpreaParser::CharContext *ctx) override;
    std::any visitMultDivRem(GazpreaParser::MultDivRemContext *ctx) override;
    std::any visitDecl(GazpreaParser::DeclContext *ctx) override;
    std::any visitTypedef(GazpreaParser::TypedefContext *ctx) override;
    std::any visitIdAssign(GazpreaParser::IdAssignContext *ctx) override;
    std::any visitVecMatTupAssign(GazpreaParser::VecMatTupAssignContext *ctx) override;
    std::any visitTuplUnpackAssign(GazpreaParser::TuplUnpackAssignContext *ctx) override;
    std::any visitCond(GazpreaParser::CondContext *ctx) override;
    std::any visitFuncDef(GazpreaParser::FuncDefContext *ctx) override;
    std::any visitProcDef(GazpreaParser::ProcDefContext *ctx) override;
    std::any visitInfPostPredLoop(GazpreaParser::InfPostPredLoopContext *ctx) override;
    std::any visitPrePredLoop(GazpreaParser::PrePredLoopContext *ctx) override;
    std::any visitIterLoop(GazpreaParser::IterLoopContext *ctx) override;
    std::any visitOutStream(GazpreaParser::OutStreamContext *ctx) override;
    std::any visitInStream(GazpreaParser::InStreamContext *ctx) override;
    std::any visitBuiltIn(GazpreaParser::BuiltInContext *ctx) override;
    std::any visitReturn(GazpreaParser::ReturnContext *ctx) override;
    std::any visitBreak(GazpreaParser::BreakContext *ctx) override;
    std::any visitContinue(GazpreaParser::ContinueContext *ctx) override;
    std::any visitRawProcCall(GazpreaParser::RawProcCallContext *ctx) override;
};

#endif //ASTBUILDER_H
