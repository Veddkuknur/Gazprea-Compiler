#include "ASTXMLWalker.h"

#include <iostream>


void XMLBuilder::openFileStream() {
  f_stream.open(f_name, std::ios::out);
}


void XMLBuilder::closeFileStream() {
  f_stream.close();
}


void XMLBuilder::openTag(const std::string& tag_name) {
  addIndent();
  indent_depth++;
  f_stream << '<' << tag_name << '>' << std::endl;
}


void XMLBuilder::closeTag(const std::string& tag_name) {
  indent_depth--;
  addIndent();
  f_stream << "</" << tag_name << '>' << std::endl;
}


void XMLBuilder::selfClosingTag(const std::string& tag_name) {
  addIndent();
  f_stream << "<" << tag_name << "/>" << std::endl;
}


void XMLBuilder::addIndent() {
  for (int i = 0; i < indent_depth; ++i) {
    f_stream << '\t';
  }
}


void XMLBuilder::addDeclLeft(bool is_constant, const std::shared_ptr<Type>& type_obj, const std::string& id) {

  std::string type_str;

  type_obj == nullptr ? type_str = "inferred" : type_str = type_obj->getTypeAsString();
  std::string mutability;

  is_constant == true ? mutability = "const" : mutability = "var";
  addIndent();
  f_stream << "<" << "declare"
                  << " mutability=\""  << mutability << '\"'
                  << " type=\""        << type_str   << '\"'
                  << " id=\""          << id         << '\"'
                  << "/>"              << std::endl;
}


void XMLBuilder::addParameter(std::tuple<bool, std::shared_ptr<Type>, std::string> param) {

  std::string mutability;
  std::string type;
  std::string id;

  std::get<0>(param) == true ? mutability = "const" : mutability = "var";
  type = std::get<1>(param)->getTypeAsString();
  id = std::get<2>(param);

  addIndent();
  f_stream << "<" << "parameter"
                  << " mutability=\""  << mutability << '\"'
                  << " type=\""        << type       << '\"'
                  << " id=\""          << id         << '\"'
                  << "/>" << std::endl;
}


void XMLBuilder::addParameter(std::tuple<std::shared_ptr<Type>, std::string> param) {

  const std::string mutability = "const";
  const std::string type = std::get<0>(param)->getTypeAsString();
  const std::string id = std::get<1>(param);

  addIndent();
  f_stream << "<" << "parameter"
                  << " mutability=\""  << mutability << '\"'
                  << " type=\""        << type       << '\"'
                  << " id=\""          << id         << '\"'
                  << "/>" << std::endl;
}


void XMLBuilder::addReturnType(const std::shared_ptr<Type>& type_obj) {

  addIndent();
  if (type_obj) {
    std::string type = type_obj->getTypeAsString();
    f_stream << "<return_type type=\"" << type << "\"/>" << std::endl;
  }
  else
    f_stream << "<return_type type=\"no return\"/>" << std::endl;
}


void XMLBuilder::addInt(int val) {
  addIndent();
  f_stream << "<integer val=\"" << val << "\"/>" << std::endl;
}


void XMLBuilder::addReal(float val) {
  addIndent();
  f_stream << "<real val=\"" << val << "\"/>" << std::endl;

}


void XMLBuilder::addBool(bool val) {
  std::string bool_str;
  val == 1 ? bool_str = "true" : bool_str = "false";
  addIndent();
  f_stream << "<boolean val=\"" << bool_str << "\"/>" << std::endl;
}


void XMLBuilder::addChar(char val) {
  addIndent();
  f_stream << "<character val=\"" << val << "\"/>" << std::endl;
}


void XMLBuilder::addId(const std::string& id) {
  addIndent();
  f_stream << "<id name=\"" << id << "\"/>" << std::endl;
}


std::any ASTXMLWalker::visitRootNode(std::shared_ptr<RootNode> node) {

  xml_builder.openFileStream();
  xml_builder.openTag("global_scope");
  node->global_block->accept(*this);
  xml_builder.closeTag("global_scope");
  xml_builder.closeFileStream();
  return {};
}


std::any ASTXMLWalker::visitDeclNode(std::shared_ptr<DeclNode> node) {

  xml_builder.openTag("declaration");

  xml_builder.openTag("left_side");
  xml_builder.addDeclLeft(node->constant, node->type, node->id);
  xml_builder.closeTag("left_side");

  xml_builder.openTag("right_side");
  if (node->expr)
    node->expr->accept(*this);
  xml_builder.closeTag("right_side");

  xml_builder.closeTag("declaration");
  return {};
}


std::any ASTXMLWalker::visitIdAssignNode(std::shared_ptr<IdAssignNode> node) {

  xml_builder.openTag("id_assignment");

  xml_builder.openTag("left_side");
  xml_builder.selfClosingTag("id name=\"" + node->id + "\"");
  xml_builder.closeTag("left_side");

  xml_builder.openTag("right_side");
  node->expr->accept(*this);
  xml_builder.closeTag("right_side");

  xml_builder.closeTag("id_assignment");
  return {};
}


std::any ASTXMLWalker::visitVecMatTupAssignNode(std::shared_ptr<VecMatTupAssignNode> node) {

  xml_builder.openTag("vec_mat_tup_assignment");

  xml_builder.openTag("left_side");
  node->expr_assign_to->accept(*this);
  xml_builder.closeTag("left_side");

  xml_builder.openTag("right_side");
  node->expr_assign_with->accept(*this);
  xml_builder.closeTag("right_side");

  xml_builder.closeTag("vec_mat_tup_assignment");
  return {};
}


std::any ASTXMLWalker::visitUnpackNode(std::shared_ptr<UnpackNode> node) {

  xml_builder.openTag("unpack_assignment");

  xml_builder.openTag("left_side");
  for (const auto& i_value : node->ivalues) {
    i_value->accept(*this);
  }
  xml_builder.closeTag("left_side");

  xml_builder.openTag("right_side");
  node->tuple->accept(*this);
  xml_builder.closeTag("right_side");

  xml_builder.closeTag("unpack_assignment");
  return {};
}


std::any ASTXMLWalker::visitTypeDefNode(std::shared_ptr<TypeDefNode> node) {
  xml_builder.openTag("typedef");

  xml_builder.selfClosingTag("true_type type=\"" + node->type->getTypeAsString() + "\"");
  xml_builder.selfClosingTag("alias name=\"" + node->type_alias + "\"");

  xml_builder.closeTag("typedef");
  return {};
}


std::any ASTXMLWalker::visitBlockNode(std::shared_ptr<BlockNode> node) {
  for (const auto& stat : node->stat_nodes) {
    stat->accept(*this);
  }
  return {};
}


std::any ASTXMLWalker::visitBoolNode(std::shared_ptr<BoolNode> node) {
  xml_builder.addBool(node->val);
  return {};
}


std::any ASTXMLWalker::visitCharNode(std::shared_ptr<CharNode> node) {
  xml_builder.addChar(node->val);
  return {};
}


std::any ASTXMLWalker::visitIntNode(std::shared_ptr<IntNode> node) {
  xml_builder.addInt(node->val);
  return {};
}


std::any ASTXMLWalker::visitRealNode(std::shared_ptr<RealNode> node) {
  xml_builder.addReal(node->val);
  return {};
}


std::any ASTXMLWalker::visitTupleNode(std::shared_ptr<TupleNode> node) {

  xml_builder.openTag("tuple_literal");
  for (const auto& elem : node->tuple) {
    elem->accept(*this);
  }
  xml_builder.closeTag("tuple_literal");
  return {};
}


std::any ASTXMLWalker::visitVecMatNode(std::shared_ptr<VecMatNode> node) {
  xml_builder.openTag("vector_matrix_literal");
  xml_builder.openTag("elements");
  for (const auto& elem : node->elements) {
    elem->accept(*this);
  }
  xml_builder.closeTag("elements");
  xml_builder.closeTag("vector_matrix_literal");
  return {};
}



std::any ASTXMLWalker::visitUnaryOpNode(std::shared_ptr<UnaryOpNode> node) {

  xml_builder.openTag("unary_op");
  xml_builder.selfClosingTag("operator op=\"" + node->op + "\"");
  node->expr->accept(*this);
  xml_builder.closeTag("unary_op");
  return {};
}

std::any ASTXMLWalker::visitBinaryOpNode(std::shared_ptr<BinaryOpNode> node) {

  xml_builder.openTag("binary_op");
  node->l_expr->accept(*this);
  xml_builder.selfClosingTag("operator op=\"" + node->op + "\"");
  node->r_expr->accept(*this);
  xml_builder.closeTag("binary_op");
  return {};
}


std::any ASTXMLWalker::visitTypeCastNode(std::shared_ptr<TypeCastNode> node) {

  xml_builder.openTag("typecast");

  xml_builder.selfClosingTag("cast_as type=\"" + node->cast_type->getTypeAsString() + '\"');
  node->expr->accept(*this);

  xml_builder.closeTag("typecast");
  return {};
}


std::any ASTXMLWalker::visitInfLoopNode(std::shared_ptr<InfLoopNode> node) {
  xml_builder.openTag("infinite_loop");
  node->loop_block->accept(*this);
  xml_builder.closeTag("infinite_loop");
  return {};
}


std::any ASTXMLWalker::visitPredLoopNode(std::shared_ptr<PredLoopNode> node) {

  xml_builder.openTag("predicated_loop");

  if (node->pre_predicated) {
    xml_builder.openTag("control_expression");
    node->contr_expr->accept(*this);
    xml_builder.closeTag("control_expression");
    node->loop_block->accept(*this);
  }
  else {
    node->loop_block->accept(*this);
    xml_builder.openTag("control_expression");
    node->contr_expr->accept(*this);
    xml_builder.closeTag("control_expression");
  }
  xml_builder.closeTag("predicated_loop");
  return {};
}

// TODO:
std::any ASTXMLWalker::visitIterLoopNode(std::shared_ptr<IterLoopNode> node) {
  return {};
}


std::any ASTXMLWalker::visitCondNode(std::shared_ptr<CondNode> node) {
  xml_builder.openTag("conditional_statement");

  xml_builder.openTag("if");
  xml_builder.openTag("control_expression");
  node->contr_expr->accept(*this);
  xml_builder.closeTag("control_expression");
  xml_builder.openTag("if_block");
  node->if_block->accept(*this);
  xml_builder.closeTag("if_block");
  xml_builder.closeTag("if");

  if (node->else_block) {
    xml_builder.openTag("else_block");
    node->else_block->accept(*this);
    xml_builder.closeTag("else_block");
  }
  xml_builder.closeTag("conditional_statement");

  return {};
}


std::any ASTXMLWalker::visitOutStreamNode(std::shared_ptr<OutStreamNode> node) {

  xml_builder.openTag("out_stream");
  node->expr->accept(*this);
  xml_builder.closeTag("out_stream");
  return {};
}


std::any ASTXMLWalker::visitInStreamNode(std::shared_ptr<InStreamNode> node) {

  xml_builder.openTag("in_stream");
  node->l_value->accept(*this);
  xml_builder.closeTag("in_stream");
  return {};
}


std::any ASTXMLWalker::visitFuncDefNode(std::shared_ptr<FuncDefNode> node) {

  xml_builder.openTag("function_" + node->func_id);

  // add the parameters
  xml_builder.openTag("parameters");
  for (const auto& param : node->params) {
    xml_builder.addParameter(param);
  }
  xml_builder.closeTag("parameters");

  // add the return type
  xml_builder.addReturnType(node->return_type);

  // add the procedure block statement
  if (node->func_block)
    node->func_block->accept(*this);

  xml_builder.closeTag("function_" + node->func_id);
  return {};
}


std::any ASTXMLWalker::visitProcDefNode(std::shared_ptr<ProcDefNode> node) {

  xml_builder.openTag("procedure_" + node->proc_id);

  // add the parameters
  xml_builder.openTag("parameters");
  for (const auto& param : node->params) {
    xml_builder.addParameter(param);
  }
  xml_builder.closeTag("parameters");

  // add the return type
  xml_builder.addReturnType(node->return_type);

  // add the procedure block statement
  if (node->proc_block)
    node->proc_block->accept(*this);

  xml_builder.closeTag("procedure_" + node->proc_id);
  return {};
}


std::any ASTXMLWalker::visitFuncProcCallNode(std::shared_ptr<FuncProcCallNode> node) {

  xml_builder.openTag("func_proc_call");

  xml_builder.selfClosingTag("func_proc id=\"" + node->id + '\"');

  // add the arguments
  xml_builder.openTag("arguments");
  for (const auto& arg : node->params) {
    arg->accept(*this);
  }
  xml_builder.closeTag("arguments");

  xml_builder.closeTag("func_proc_call");
  return {};
}


std::any ASTXMLWalker::visitLengthCallNode(std::shared_ptr<LengthCallNode> node) {

  xml_builder.openTag("length_call");
  node->vector->accept(*this);
  xml_builder.closeTag("length_call");
  return {};
}


std::any ASTXMLWalker::visitRowsCallNode(std::shared_ptr<RowsCallNode> node) {

  xml_builder.openTag("rows_call");
  node->matrix->accept(*this);
  xml_builder.closeTag("rows_call");
  return {};
}


std::any ASTXMLWalker::visitColumnsCallNode(std::shared_ptr<ColumnsCallNode> node) {

  xml_builder.openTag("columns_call");
  node->matrix->accept(*this);
  xml_builder.closeTag("columns_call");
  return {};
}


std::any ASTXMLWalker::visitReverseCallNode(std::shared_ptr<ReverseCallNode> node) {

  xml_builder.openTag("reverse_call");
  node->vector->accept(*this);
  xml_builder.closeTag("reverse_call");
  return {};
}


std::any ASTXMLWalker::visitFormatCallNode(std::shared_ptr<FormatCallNode> node) {

  xml_builder.openTag("format_call");
  node->scalar->accept(*this);
  xml_builder.closeTag("format_call");
  return {};
}

std::any ASTXMLWalker::visitStreamStateCallNode(std::shared_ptr<StreamStateCallNode> node) {
  xml_builder.selfClosingTag("stream_state_call");
  return {};
}


std::any ASTXMLWalker::visitReturnNode(std::shared_ptr<ReturnNode> node) {
  xml_builder.openTag("return");
  if (node->free_node)
    node->free_node->accept(*this);
  if (node->expr)
    node->expr->accept(*this);
  xml_builder.closeTag("return");
  return {};
}


std::any ASTXMLWalker::visitIdNode(std::shared_ptr<IdNode> node) {
  xml_builder.addId(node->id);
  return {};
}


std::any ASTXMLWalker::visitBreakNode(std::shared_ptr<BreakNode> node) {
  xml_builder.selfClosingTag("break");
  return {};
}


std::any ASTXMLWalker::visitContinueNode(std::shared_ptr<ContinueNode> node) {
  xml_builder.selfClosingTag("continue");
  return {};
}


std::any ASTXMLWalker::visitTupleAccessNode(std::shared_ptr<TupleAccessNode> node) {

  xml_builder.openTag("tuple_access");

  xml_builder.openTag("tuple_id");
  node->tuple_id->accept(*this);
  xml_builder.closeTag("tuple_id");

  xml_builder.openTag("element");
  node->element->accept(*this);
  xml_builder.closeTag("element");

  xml_builder.closeTag("tuple_access");
  return {};
}


std::any ASTXMLWalker::visitIndexNode(std::shared_ptr<IndexNode> node) {
  xml_builder.openTag("index_expression");

  xml_builder.openTag("collection");
  node->collection->accept(*this);
  xml_builder.closeTag("collection");

  xml_builder.openTag("index1");
  node->index1->accept(*this);
  xml_builder.closeTag("index1");

  if (node->index2) {
    xml_builder.openTag("index2");
    node->index2->accept(*this);
    xml_builder.closeTag("index2");
  }
  xml_builder.closeTag("index_expression");
  return {};
}


std::any ASTXMLWalker::visitRangeNode(std::shared_ptr<RangeNode> node) {

  xml_builder.openTag("range");

  xml_builder.openTag("lower_bound");
  node->lower_bound->accept(*this);
  xml_builder.closeTag("lower_bound");

  xml_builder.openTag("upper_bound");
  node->upper_bound->accept(*this);
  xml_builder.closeTag("upper_bound");

  xml_builder.closeTag("range");
  return {};
}


std::any ASTXMLWalker::visitGeneratorNode(std::shared_ptr<GeneratorNode> node) {

  xml_builder.openTag("generator");

  xml_builder.openTag("domain_expr1");
  node->id1->accept(*this);
  node->vec_expr1->accept(*this);
  xml_builder.closeTag("domain_expr1");

  if (node->id2) {
    xml_builder.openTag("domain_expr2");
    node->id2->accept(*this);
    node->vec_expr2->accept(*this);
    xml_builder.closeTag("domain_expr2");
  }
  xml_builder.openTag("rhs_expr");
  node->rhs_expr->accept(*this);
  xml_builder.closeTag("rhs_expr");

  xml_builder.closeTag("generator");
  return {};
}



std::any ASTXMLWalker::visitFilterNode(std::shared_ptr<FilterNode> node) {

  xml_builder.openTag("filter");

  xml_builder.openTag("domain_expr");
  node->id->accept(*this);
  node->vec_expr->accept(*this);
  xml_builder.closeTag("domain_expr");

  xml_builder.openTag("predicate_exprs");
  for (auto expr : node->pred_exprs) {
    expr->accept(*this);
  }
  xml_builder.closeTag("predicate_exprs");

  xml_builder.closeTag("filter");
  return {};
}


std::any ASTXMLWalker::visitFreeNode(std::shared_ptr<FreeNode> node) {
  std::string free_up_to;
  if (node->scope_id == 0)
    free_up_to = "current";
  else if (node->scope_id == 1)
    free_up_to = "func/proc";
  else
    free_up_to = "loop";

  xml_builder.selfClosingTag("free up_to_scope=\"" + free_up_to + "\"");
  return {};
}


