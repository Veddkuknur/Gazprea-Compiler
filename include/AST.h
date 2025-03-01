#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include <string>
#include <any>

#include "Type.h"
#include "Scope.h"


// forward declaration
class ASTWalker;

class ASTNode {
  /* Generic abstract AST node, parent to all other node types */
public:
  explicit ASTNode(int line = 0, int column = 0)
    : line(line), column(column) {}
  virtual ~ASTNode() = default;
  virtual std::any accept(ASTWalker& walker) = 0;
  std::shared_ptr<Scope> containing_scope; //Every AST has a scope that contains it, it will be defined during the define pass.
  std::shared_ptr<Type> type; //Every AST has a scope that contains it, it will be defined during the define pass.

  int line;
  int column;
};

class DeclNode: public ASTNode, public std::enable_shared_from_this<DeclNode> {
public:
  /* Declaration node */

  DeclNode(const bool constant, std::shared_ptr<Type> type, std::string declType, std::string id, std::shared_ptr<ASTNode> expr, int line, int col)
    : ASTNode(line, col), constant(constant), declType(std::move(declType)), id(std::move(id)), expr(std::move(expr)) {this->type = std::move(type);}

  bool constant; // true if the variable is declared as constant
  std::string declType; // Contains the type as a string, this is used to simplify code in one of the ast passes, ask Hridyansh
  std::string id;
  std::shared_ptr<ASTNode> expr;

  std::any accept(ASTWalker& walker) override;
};


class IdAssignNode: public ASTNode, public std::enable_shared_from_this<IdAssignNode> {
  /* Id assignment nodes are statement nodes. They contain a variable id and an expression node */
public:

  IdAssignNode(std::string id, std::shared_ptr<ASTNode> expr, int line, int col)
    : ASTNode(line, col), id(std::move(id)), expr(std::move(expr)) {}

  std::shared_ptr<Type> id_type = nullptr; // we don't know what type the id is until we've done a pass over the AST
  std::string id;
  std::shared_ptr<ASTNode> expr;

  std::any accept(ASTWalker& walker) override;
};


class VecMatTupAssignNode : public ASTNode, public std::enable_shared_from_this<VecMatTupAssignNode> {
  /* Vector/Matrix/Tuple assignment node. For assigning an expression result to an element in a vector, matrix, or tuple.
   * holds an expression to assign to (index or tuple access), and an expression to assign with (any expression) */
public:
  VecMatTupAssignNode(std::shared_ptr<ASTNode> expr_assign_to, std::shared_ptr<ASTNode> expr_assign_with, int line, int col)
    : ASTNode(line, col), expr_assign_to(std::move(expr_assign_to)), expr_assign_with(std::move(expr_assign_with)) {}

  // we can get the element type from the expression we are assigning to.
  std::shared_ptr<ASTNode> expr_assign_to;
  std::shared_ptr<ASTNode> expr_assign_with;

  std::any accept(ASTWalker& walker) override;
};


class TypeDefNode: public ASTNode, public std::enable_shared_from_this<TypeDefNode> {
  /* Typedef node, can give a type an alternative alias at the global scope */
public:

  TypeDefNode(std::shared_ptr<Type> type, std::string type_alias, int line, int col)
    : ASTNode(line, col) , type_alias(std::move(type_alias)) {
    this->type = std::move(type);
  }

  //std::shared_ptr<Type> type;
  std::string type_alias;

  std::any accept(ASTWalker& walker) override;
};


class BlockNode: public ASTNode, public std::enable_shared_from_this<BlockNode> {
  /* Block nodes can contain declarations and non-declarations statements. If a block node contains declarations:
   *     - they must appear before any other statements */
public:

  explicit BlockNode(std::vector<std::shared_ptr<ASTNode>> stat_nodes, int line, int col)
    : ASTNode(line, col), stat_nodes(std::move(stat_nodes)) {}

  std::vector<std::shared_ptr<ASTNode>> stat_nodes;

  std::any accept(ASTWalker& walker) override;
};


class RootNode : public ASTNode, public std::enable_shared_from_this<RootNode> {
  /* Root of AST, contains uniq_ptr to the global block */
public:

  explicit RootNode(std::shared_ptr<BlockNode> global_block, int line, int col)
    : ASTNode(line, col), global_block(std::move(global_block)) {}

  std::shared_ptr<BlockNode> global_block;

  std::any accept(ASTWalker& walker) override;
};


class BoolNode : public ASTNode, public std::enable_shared_from_this<BoolNode> {
  /* Boolean node, holds a constant boolean value */
public:

  explicit BoolNode(const bool val, int line, int col)
    : ASTNode(line, col), val(val) {
    type = std::make_shared<PrimitiveType>(Type::BaseType::boolean);
  }

  bool val;

  std::any accept(ASTWalker& walker) override;
};


class RangeNode : public ASTNode, public std::enable_shared_from_this<RangeNode> {
  /* Range expression node, holds a lower bound expression and an upper bound expression*/
public:

  explicit RangeNode(std::shared_ptr<ASTNode> lower_bound, std::shared_ptr<ASTNode> upper_bound, int line, int col)
    : ASTNode(line, col), lower_bound(std::move(lower_bound)), upper_bound(std::move(upper_bound)) {
    type = std::make_shared<VectorType>(Type::BaseType::integer, nullptr);
  }

  std::shared_ptr<ASTNode> lower_bound, upper_bound;

  std::any accept(ASTWalker& walker) override;
};


class CharNode : public ASTNode, public std::enable_shared_from_this<CharNode> {
  /* Character node, holds a constant character value */
public:

  explicit CharNode(const char val, int line, int col)
    : ASTNode(line, col), val(val) {type = std::make_shared<PrimitiveType>(Type::BaseType::character);}

  char val;

  std::any accept(ASTWalker& walker) override;
};


class IntNode : public ASTNode, public std::enable_shared_from_this<IntNode> {
  /* Integer node, holds a constant integer value */
public:

  explicit IntNode(const int val, int line, int col)
    : ASTNode(line, col), val(val) {type = std::make_shared<PrimitiveType>(Type::BaseType::integer);}

  int val;

  std::any accept(ASTWalker& walker) override;
};


class RealNode : public ASTNode, public std::enable_shared_from_this<RealNode> {
  /* Real (float) node, holds a constant float value */
public:

  explicit RealNode(const float val, int line, int col)
    : ASTNode(line, col), val(val) {type = std::make_shared<PrimitiveType>(Type::BaseType::real);}

  float val;

  std::any accept(ASTWalker& walker) override;
};


class TupleNode : public ASTNode, public std::enable_shared_from_this<TupleNode> {
  /* Tuple node, holds a tuple literal object (vector of expression nodes) */
public:

  explicit TupleNode(std::vector<std::shared_ptr<ASTNode>> tuple, int line, int col)
    : ASTNode(line, col), tuple(std::move(tuple)) {type = std::make_shared<TupleType>(TupleType()); }

  // we set type to empty at instantiation.
  std::vector<std::shared_ptr<ASTNode>> tuple;

  std::any accept(ASTWalker& walker) override;
};


class UnpackNode : public ASTNode, public std::enable_shared_from_this<UnpackNode> {
  /* Tuple unpack node. For unpacking a tuple's elements into 2 or more I-values */
public:

  UnpackNode(std::vector<std::shared_ptr<ASTNode>> ivalues, std::shared_ptr<ASTNode> tuple, int line, int col)
    : ASTNode(line, col), ivalues(std::move(ivalues)), tuple(std::move(tuple)) {}

  std::vector<std::shared_ptr<ASTNode>> ivalues; // these hold the expression representation of the I-values (e.g. index, id, tuple.access)
  std::shared_ptr<ASTNode> tuple; // the tuple we want to unpack from

  std::any accept(ASTWalker& walker) override;
};


class VecMatNode : public ASTNode, public std::enable_shared_from_this<VecMatNode> {
  /* Vector/Matrix literal node. A container that holds a collection of items of the same type (matrices hold a collection of vectors) */
public:

  explicit VecMatNode(std::vector<std::shared_ptr<ASTNode>> elements, bool is_string, int line, int col)
    : ASTNode(line, col), elements(std::move(elements)), is_string(is_string) {}

  std::vector<std::shared_ptr<ASTNode>> elements;
  bool is_string;

  std::any accept(ASTWalker& walker) override;
};


class UnaryOpNode : public ASTNode, public std::enable_shared_from_this<UnaryOpNode> {
  /* Unary operation nodes consist of an operator and a single expression node */
public:

  UnaryOpNode(std::string op, std::shared_ptr<ASTNode> expr, int line, int col)
    : ASTNode(line, col), op(std::move(op)), expr(std::move(expr)) {}

  std::string op; // enum?
  std::shared_ptr<ASTNode> expr;

  std::any accept(ASTWalker& walker) override;
};


class BinaryOpNode : public ASTNode, public std::enable_shared_from_this<BinaryOpNode> {
  /* Binary operation nodes consist of an operator and left and right expression nodes */
public:

  BinaryOpNode(std::string op, std::shared_ptr<ASTNode> l_expr, std::shared_ptr<ASTNode> r_expr, int line, int col)
    : ASTNode(line, col), op(std::move(op)), l_expr(std::move(l_expr)), r_expr(std::move(r_expr)) {}

  std::string op; // enum?
  std::shared_ptr<ASTNode> l_expr;
  std::shared_ptr<ASTNode> r_expr;

  std::any accept(ASTWalker& walker) override;
};


class TypeCastNode : public ASTNode, public std::enable_shared_from_this<TypeCastNode> {
  /* Typecast node, holds casting type and the value to be cast */
public:

  TypeCastNode(std::shared_ptr<Type> cast_type, std::shared_ptr<ASTNode> expr, int line, int col)
    : ASTNode(line, col), cast_type(std::move(cast_type)), expr(std::move(expr)) {}

  std::shared_ptr<Type> cast_type;
  std::shared_ptr<ASTNode> expr;

  std::any accept(ASTWalker& walker) override;
};


class InfLoopNode : public ASTNode, public std::enable_shared_from_this<InfLoopNode> {
  /* Infinite loop node, holds the loop's inner block*/
public:

  explicit InfLoopNode(std::shared_ptr<BlockNode> loop_block, int line, int col)
    : ASTNode(line, col), loop_block(std::move(loop_block)) {}

  std::shared_ptr<BlockNode> loop_block;

  std::any accept(ASTWalker& walker) override;
};


class PredLoopNode : public ASTNode, public std::enable_shared_from_this<PredLoopNode> {
  /* Predicated loop node, loop is either pre-predicated or post-predicated, holds control expression and loop's
   * inner block */
public:

  PredLoopNode(bool pre_predicated, std::shared_ptr<ASTNode> contr_expr, std::shared_ptr<BlockNode> loop_block, int line, int col)
    : ASTNode(line, col), pre_predicated(pre_predicated), contr_expr(std::move(contr_expr)), loop_block(std::move(loop_block)) {}

  bool pre_predicated;
  std::shared_ptr<ASTNode> contr_expr;
  std::shared_ptr<BlockNode> loop_block;

  std::any accept(ASTWalker& walker) override;
};


class IterLoopNode : public ASTNode, public std::enable_shared_from_this<IterLoopNode> {
  /* Iterator loop node, holds loop's domain expression (vector and domain variable), and loop's inner block */
public:

  IterLoopNode(std::string dom_id, std::shared_ptr<ASTNode> dom_expr, std::shared_ptr<BlockNode> loop_block,
    int line, int col)
    : ASTNode(line, col), dom_id(std::move(dom_id)), dom_expr(std::move(dom_expr)),
      loop_block(std::move(loop_block)) {}

  std::string dom_id;
  std::shared_ptr<ASTNode> dom_expr;
  std::shared_ptr<BlockNode> loop_block;

  std::any accept(ASTWalker& walker) override;
};


class CondNode : public ASTNode, public std::enable_shared_from_this<CondNode> {
  /* Conditional (if-else) node, conditional either has a matching else statement, or it doesn't. holds if's
   * control expression and inner block, and else's inner block. */
public:

  CondNode(std::shared_ptr<ASTNode> contr_expr, std::shared_ptr<BlockNode> if_block,
           std::shared_ptr<BlockNode> else_block, int line, int col)
    : ASTNode(line, col), contr_expr(std::move(contr_expr)), if_block(std::move(if_block)), else_block(std::move(else_block)) {}

  std::shared_ptr<ASTNode> contr_expr;
  std::shared_ptr<BlockNode> if_block;
  std::shared_ptr<BlockNode> else_block; // nullptr if there isn't a corresponding else block
  bool is_terminal = false; // does visiting this conditional guarantee a return? (determined with AST pass)

  std::any accept(ASTWalker& walker) override;
};


class OutStreamNode : public ASTNode, public std::enable_shared_from_this<OutStreamNode> {
  /* Output stream node, holds the expression whose value we want written to std_output */
public:

  explicit OutStreamNode(std::shared_ptr<ASTNode> expr, int line, int col)
    : ASTNode(line, col), expr(std::move(expr)) {}

  std::shared_ptr<ASTNode> expr;
  std::any accept(ASTWalker& walker) override;
};


class InStreamNode: public ASTNode, public std::enable_shared_from_this<InStreamNode> {
  /* Id assignment nodes are statement nodes. They contain a variable id and an expression node */
public:

  InStreamNode(std::shared_ptr<ASTNode> l_value, int  line, int col)
    : ASTNode(line, col), l_value(std::move(l_value)) {}

  std::shared_ptr<ASTNode> l_value;

  std::any accept(ASTWalker& walker) override;
};

class FuncDefNode : public ASTNode, public std::enable_shared_from_this<FuncDefNode> {
  /* Function definition node, holds function alias, it's parameters, it's return value, and it's inner block.
   * Functions can be prototypes (block -> null_ptr) */
public:

  FuncDefNode(std::string func_id, std::vector<std::tuple<std::shared_ptr<Type>, std::string>> params,
              std::shared_ptr<Type> return_type, std::shared_ptr<BlockNode> func_block, int line, int col)
    :  ASTNode(line, col), func_id(std::move(func_id)), params(std::move(params)), return_type(std::move(return_type)),
       func_block(std::move(func_block)) {}

  std::string func_id;
  // a vector of parameters (ptr_to_type, param_id)
  std::vector<std::tuple<std::shared_ptr<Type>, std::string>> params;
  std::shared_ptr<Type> return_type;
  std::shared_ptr<BlockNode> func_block;

  std::any accept(ASTWalker& walker) override;
};


class ProcDefNode : public ASTNode, public std::enable_shared_from_this<ProcDefNode> {
  /* Procedure definition node, holds procedure alias, it's parameters, it's return value, and it's inner block.
   *     - Procedures can be prototypes (block -> null_ptr)
   *     - Procedures can have user defined qualifiers
   *     - Procedures don't need to return anything */
public:

  ProcDefNode(std::string proc_id, std::vector<std::tuple<bool, std::shared_ptr<Type>, std::string>> params,
              std::shared_ptr<Type> return_type, std::shared_ptr<BlockNode> proc_block, int line, int col)
    : ASTNode(line, col), proc_id(std::move(proc_id)), params(std::move(params)), return_type(std::move(return_type)),
      proc_block(std::move(proc_block)) {}

  std::string proc_id;
  // a vector of parameters (qualifier, ptr_to_type, param_id)
  std::vector<std::tuple<bool, std::shared_ptr<Type>, std::string>> params; // bool is true if param is const
  std::shared_ptr<Type> return_type; // return values are optional for procedures
  std::shared_ptr<BlockNode> proc_block;

  std::any accept(ASTWalker& walker) override;
};


class FuncProcCallNode : public ASTNode, public std::enable_shared_from_this<FuncProcCallNode> {
  /* Function/Procedure call node, holds func/proc id, and it's parameters */
public:

  FuncProcCallNode(std::string id, std::vector<std::shared_ptr<ASTNode>> params, int line, int col)
    : ASTNode(line, col), id(std::move(id)), params(std::move(params)) {}

  std::string id;
  // a vector of parameters (each parameter is an expression)
  std::vector<std::shared_ptr<ASTNode>> params;
  bool call_used = false;

  std::any accept(ASTWalker& walker) override;
};


class LengthCallNode : public ASTNode, public std::enable_shared_from_this<LengthCallNode> {
  /* Function call node for built-in function: length(). holds the vector we want to know the length of */
public:

  LengthCallNode(std::shared_ptr<ASTNode> vector, int line, int col)
    : ASTNode(line, col), vector(std::move(vector)) {}

  std::shared_ptr<ASTNode> vector;
  std::shared_ptr<VectorType> type;

  std::any accept(ASTWalker& walker) override;
};


class RowsCallNode : public ASTNode, public std::enable_shared_from_this<RowsCallNode> {
  /* Function call node for built-in function: rows(). holds the matrix we want to know the number of rows of */
public:

  RowsCallNode(std::shared_ptr<ASTNode> matrix, int line, int col)
    : ASTNode(line, col), matrix(std::move(matrix)) {}

  std::shared_ptr<ASTNode> matrix;

  std::any accept(ASTWalker& walker) override;
};


class ColumnsCallNode : public ASTNode, public std::enable_shared_from_this<ColumnsCallNode> {
  /* Function call node for built-in function: columns(). holds the matrix we want to know the number of columns of */
public:

  ColumnsCallNode(std::shared_ptr<ASTNode> matrix, int line, int col)
    : ASTNode(line, col), matrix(std::move(matrix)) {}

  std::shared_ptr<ASTNode> matrix;

  std::any accept(ASTWalker& walker) override;
};


class ReverseCallNode : public ASTNode, public std::enable_shared_from_this<ReverseCallNode> {
  /* Function call node for built-in function: reverse(). holds the vector whose elements we want to reverse */
public:

  ReverseCallNode(std::shared_ptr<ASTNode> vector, int line, int col)
    : ASTNode(line, col), vector(std::move(vector)) {}

  std::shared_ptr<ASTNode> vector;
  std::shared_ptr<Type> type;
  std::any accept(ASTWalker& walker) override;
};


class FormatCallNode : public ASTNode, public std::enable_shared_from_this<FormatCallNode> {
  /* Function call node for built-in function: format(). holds the scalar that we want to print */
public:

  FormatCallNode(std::shared_ptr<ASTNode> scalar, int line, int col)
    : ASTNode(line, col), scalar(std::move(scalar)) {}

  std::shared_ptr<ASTNode> scalar;
  std::shared_ptr<Type> type;
  std::any accept(ASTWalker& walker) override;
};


class StreamStateCallNode : public ASTNode, public std::enable_shared_from_this<StreamStateCallNode> {
  /* Function call node for built-in function: stream_state(). holds nothing, we are just checking the state of the
   * input_stream */
public:

  StreamStateCallNode(int line, int col)
    : ASTNode(line, col) {}

  std::any accept(ASTWalker& walker) override;
};


class IdNode : public ASTNode, public std::enable_shared_from_this<IdNode> {
  /* id node, holds the name of an id */
public:

  explicit IdNode(std::string id, int line, int col)
    : ASTNode(line, col), id(std::move(id)) {}

  std::string id;
  std::any accept(ASTWalker& walker) override;
};

class BreakNode : public ASTNode, public std::enable_shared_from_this<BreakNode> {
  /* break node, doesn't hold anything, just represents a break statement */
public:
  BreakNode(int line, int col)
    : ASTNode(line, col) {}

  std::any accept(ASTWalker& walker) override;
};

class ContinueNode : public ASTNode, public std::enable_shared_from_this<ContinueNode> {
  /* continue node, doesn't hold anything, just represents a continue statement */
public:
  ContinueNode(int line, int col)
    : ASTNode(line, col) {}

  std::any accept(ASTWalker& walker) override;
};

class TupleAccessNode : public ASTNode, public std::enable_shared_from_this<TupleAccessNode> {
  /* tuple access node, holds the tuple id and the element to access (id node, or int node) */
public:
  TupleAccessNode(std::shared_ptr<IdNode> tuple_id, std::shared_ptr<ASTNode> element, int line, int col)
    : ASTNode(line, col), tuple_id(std::move(tuple_id)), element(std::move(element)) {}

  std::shared_ptr<IdNode> tuple_id;
  std::shared_ptr<ASTNode> element;
  std::any accept(ASTWalker& walker) override;
};


class IndexNode : public ASTNode, public std::enable_shared_from_this<IndexNode> {
  /* index node, holds a collection object (vector or matrix), and the index expression(s) */
public:
  IndexNode(std::shared_ptr<ASTNode> collection, std::shared_ptr<ASTNode> index1, std::shared_ptr<ASTNode> index2, int line, int col)
    : ASTNode(line, col), collection(std::move(collection)), index1(std::move(index1)), index2(std::move(index2)) {}

  std::shared_ptr<ASTNode> collection, index1, index2;

  std::any accept(ASTWalker& walker) override;
};


class GeneratorNode : public ASTNode, public std::enable_shared_from_this<GeneratorNode> {
  /* Generator node. holds domain expression index(s), domain expression(s), and expression to the right of the '|'  */
public:

  GeneratorNode(std::shared_ptr<ASTNode> id1, std::shared_ptr<ASTNode> id2, std::shared_ptr<ASTNode> vec_expr1,
    std::shared_ptr<ASTNode> vec_expr2, std::shared_ptr<ASTNode> rhs_expr, int line, int col)
      : ASTNode(line, col), id1(std::move(id1)), id2(std::move(id2)), vec_expr1(std::move(vec_expr1)),
          vec_expr2(std::move(vec_expr2)), rhs_expr(std::move(rhs_expr)) {}

  std::shared_ptr<ASTNode> id1, id2;
  std::shared_ptr<ASTNode> vec_expr1, vec_expr2;
  std::shared_ptr<ASTNode> rhs_expr;

  std::any accept(ASTWalker& walker) override;
};


class FilterNode : public ASTNode, public std::enable_shared_from_this<FilterNode> {
  /* Filter node. holds domain expression index, domain expression, and expression(s) to the right of the '&'  */
public:

  FilterNode(std::shared_ptr<ASTNode> id,  std::shared_ptr<ASTNode> vec_expr,
    std::vector<std::shared_ptr<ASTNode>> pred_exprs, int line, int col)
      : ASTNode(line, col), id(std::move(id)), vec_expr(std::move(vec_expr)),
          pred_exprs(std::move(pred_exprs)) {}

  std::shared_ptr<ASTNode> id;
  std::shared_ptr<ASTNode> vec_expr;
  std::vector<std::shared_ptr<ASTNode>> pred_exprs;

  std::any accept(ASTWalker& walker) override;
};


class FreeNode : public ASTNode, public std::enable_shared_from_this<FreeNode> {
  /* Free node. Signals to code gen pass to free unused memory allocations at within current scope
   * (added during AST pass) */
public:

  FreeNode() : ASTNode(0,0) {}

  std::shared_ptr<Scope> scope_to_free_up_to = nullptr; // free this scope, as well as all the scopes up to and
                                                        // including the one stored in this attribute
  unsigned short scope_id; // tells us in a future pass which scope we want to store in the above attribute
                           // (0 for current scope, 1 for func/proc, 2 for loop, 3 for main proc)

  std::any accept(ASTWalker&) override;
};


class ReturnNode : public ASTNode, public std::enable_shared_from_this<ReturnNode> {
  /* Return statement node, holds the expression to be returned in a function or procedure */
public:

  explicit ReturnNode(std::shared_ptr<ASTNode> expr, int line, int col)
    : ASTNode(line, col), expr(std::move(expr)) {}

  std::shared_ptr<FreeNode> free_node;
  std::shared_ptr<ASTNode> expr;

  std::any accept(ASTWalker& walker) override;
};

#endif //AST_H
