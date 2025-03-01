#include "ASTBuilder.h"


std::shared_ptr<RootNode> ASTBuilder::getRoot() {
    return ast_root;
}


std::shared_ptr<Type> ASTBuilder::stringToBaseType(const std::string& string_to_convert) {

    PrimitiveType prim_type;
    if (string_to_convert == "boolean") {
        prim_type.base_type = Type::BaseType::boolean;
    }
    else if (string_to_convert == "character" or string_to_convert == "string") {
        prim_type.base_type = Type::BaseType::character;
        if (string_to_convert == "string") {
            // variable size '*' is represented by a null ASTNode pointer
            std::shared_ptr<ASTNode> size_expr = nullptr;
            auto strType = std::make_shared<VectorType>(prim_type.base_type, size_expr);
            strType->isString = true;
            return strType;
        }
    }
    else if (string_to_convert == "integer") {
        prim_type.base_type = Type::BaseType::integer;
    }
    else if (string_to_convert == "real") {
        prim_type.base_type = Type::BaseType::real;
    }
    else if (typedefMap.find(string_to_convert) != typedefMap.end()){
        // check if the type is an existing typedef
        return typedefMap[string_to_convert];
    }
    else {
        // no matching type
        return nullptr;
    }
    return std::make_shared<PrimitiveType>(prim_type);
}




std::shared_ptr<Type> ASTBuilder::declTypeToType(GazpreaParser::DeclTypeContext* ctx, int line) {

    std::shared_ptr<Type> type_to_return;

    // matrix case
    if (ctx->declSize(1)) {
        Type::BaseType base_type;
        if (ctx->TYPE())
            base_type = stringToBaseType(ctx->TYPE()->getText())->getBaseType();
        else
            base_type = stringToBaseType(ctx->ID()->getText())->getBaseType();

        // variable size '*' is represented by a null ASTNode pointer
        std::shared_ptr<ASTNode> row_expr = nullptr, col_expr = nullptr;
        if (ctx->declSize(0)->expr())
            row_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->declSize(0)->expr()));
        if (ctx->declSize(1)->expr())
            col_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->declSize(1)->expr()));
        type_to_return = std::make_shared<MatrixType>(base_type, row_expr, col_expr);
    }

    // vector case
    else if (ctx->declSize(0)) {
        Type::BaseType base_type;
        if (ctx->TYPE()) {
            base_type = stringToBaseType(ctx->TYPE()->getText())->getBaseType();
        }
        else {
            base_type = stringToBaseType(ctx->ID()->getText())->getBaseType();

        }
        // variable size '*' is represented by a null ASTNode pointer
        std::shared_ptr<ASTNode> size_expr = nullptr;
        if (ctx->declSize(0)->expr())
            size_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->declSize(0)));
        type_to_return = std::make_shared<VectorType>(base_type, size_expr);
    }

    // tuple case
    else if (ctx->KW_TUPLE()) {
        std::vector<std::shared_ptr<Type>> type_vec;
        std::vector<std::string> name_vec;

        // get the types (and names if applicable) of each element in the tuple
        for (auto tuple_elem : ctx->tupleElemType()) {
            auto elem_type = stringToBaseType(tuple_elem->declType()->getText());


            if (std::dynamic_pointer_cast<TupleType>(elem_type))
                throw TypeError(line, "Cannot nest tuples inside of each other");
            //If element is not a tuple type then we actually get the type again
            elem_type = declTypeToType(tuple_elem->declType(), line);

            if (elem_type == nullptr)
                throw TypeError(line, "Unsupported base type for user defined type");


            std::string elem_name;
            if (tuple_elem->ID())
                elem_name = tuple_elem->ID()->getText();
            name_vec.push_back(elem_name);
            type_vec.push_back(elem_type);
        }
        type_to_return = std::make_shared<TupleType>(type_vec, name_vec);
    }

    // primitive case
    else {
        type_to_return = stringToBaseType(ctx->getText());
        if (type_to_return == nullptr)
            throw TypeError(line, "Unsupported base type for user defined type");
    }

    return type_to_return;
}


std::any ASTBuilder::visitFile(GazpreaParser::FileContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // store the statement nodes in the global block node
    auto global_block = std::any_cast<std::shared_ptr<BlockNode>>(visitBlock(ctx->block()));

    // store the global block in the ast_root
    ast_root = std::make_shared<RootNode>(std::move(global_block), line, col);
    return {};
}


std::any ASTBuilder::visitBlock(GazpreaParser::BlockContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // create a vector to store out block statements
    auto stat_nodes = std::vector<std::shared_ptr<ASTNode>>();

    // visit each statement in the block and store it in a vector of statement nodes
    for (auto stat: ctx->stat()) {
        auto node = std::any_cast<std::shared_ptr<ASTNode>>(visit(stat));
        stat_nodes.push_back(std::move(node));
    }
    // create a block node and store the statement vector inside
    auto block = std::make_shared<BlockNode>(std::move(stat_nodes), line, col);
    return block;
}


std::any ASTBuilder::visitBlockStat(GazpreaParser::BlockStatContext *ctx) {

    // get the block node
    std::shared_ptr<ASTNode> block_node = std::any_cast<std::shared_ptr<BlockNode>>(visit(ctx->block()));
    return block_node;
}


std::any ASTBuilder::visitParens(GazpreaParser::ParensContext* ctx) {

    // get the expression inside the parentheses
    std::shared_ptr<ASTNode> expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));
    return expr;
}


std::any ASTBuilder::visitBool(GazpreaParser::BoolContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    bool bool_val = false;

    // get the bool value string
    if (const std::string str_val = ctx->BOOL()->getText(); str_val == "true") {
        bool_val = true;
    }
    // create bool node
    std::shared_ptr<ASTNode> bool_node = std::make_shared<BoolNode>(bool_val, line, col);
    return bool_node;
}


std::any ASTBuilder::visitString(GazpreaParser::StringContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    std::string str_literal = ctx->STRING()->getText();

    // remove the leading and trailing quotes
    str_literal.erase(0, 1); str_literal.erase(str_literal.size()-1, 1);

    // turn each character in the string into a CharNode and store it in a vector
    std::vector<std::shared_ptr<ASTNode>> elements;
    char character;
    for (int i=0;  i<str_literal.size();i++) {
        if (str_literal[i] =='\\' ) {
            const char checkChar = str_literal[i+1];
            char val;
            switch (checkChar) {
                case '0':  val = '\0'; break;
                case 'a':  val = '\a'; break;
                case 'b':  val = '\b'; break;
                case 't':  val = '\t'; break;
                case 'n':  val = '\n'; break;
                case 'r':  val = '\r'; break;
                case '"':  val = '"';  break;
                case '\'': val = '\''; break;
                case '\\': val = '\\'; break;
                default:
                    throw TypeError(line,"Invalid character");
            }
            character = val;
            i+=1;
        }else {
            character = str_literal[i];
        }
        std::shared_ptr<ASTNode> char_node = std::make_shared<CharNode>(character, line, col);
        elements.push_back(char_node);
    }
    // create the char vector (string) node
    const auto string_node = std::make_shared<VecMatNode>(elements, true, line, col);
    string_node->is_string = true;
    return  std::dynamic_pointer_cast<ASTNode>(string_node);
}


std::any ASTBuilder::visitDot(GazpreaParser::DotContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the tuple access string
    std::string tuple_access_str = ctx->TUPL_ACCESS()->getText();

    // split it into left and right sides at the '.'
    size_t dot_pos = tuple_access_str.find('.');
    std::string tuple_id_str = tuple_access_str.substr(0, dot_pos); // left side
    std::string tuple_elem_str = tuple_access_str.substr(dot_pos + 1);

    // create the IdNode for the tuple
    auto tuple_id_node = std::make_shared<IdNode>(tuple_id_str, line, col);

    // create an int node if the right side is an int
    std::shared_ptr<ASTNode> tuple_element_node;
    if (std::isdigit(tuple_elem_str[0])) {

        // if the first character is a digit, we know its not an id
        tuple_element_node = std::make_shared<IntNode>(std::stoi(tuple_elem_str), line, col);
    }
    // create an id node if the right side is an id
    else {
        tuple_element_node = std::make_shared<IdNode>(tuple_elem_str, line, col);
    }

    // create the tuple access node
    std::shared_ptr<ASTNode> tuple_access_node = std::make_shared<TupleAccessNode>(tuple_id_node, tuple_element_node, line, col);
    return tuple_access_node;
}


std::any ASTBuilder::visitRange(GazpreaParser::RangeContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the lower and upper bound expressions
    auto lower_bound = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto upper_bound = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create Range node
    std::shared_ptr<ASTNode> range_node = std::make_shared<RangeNode>(lower_bound, upper_bound, line, col);
    return range_node;
}


std::any ASTBuilder::visitRangeRealStartRealEnd(GazpreaParser::RangeRealStartRealEndContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // extract the real from the int_range string ("<int>." + ".")
    auto int_range = ctx->INT_RANGE()->getText();
    auto pos = int_range.rfind('.');
    int_range.erase(pos, 1);
    float real_val;

    try {
        real_val = std::stof(int_range);
    }
    catch (const std::out_of_range&) {
        real_val = std::numeric_limits<float>::infinity();
    }
    // create the real lower bound node
    std::shared_ptr<ASTNode> lower_bound = std::make_shared<RealNode>(real_val, line, col);

    // create a real string from the int string ("." + ".<int>")
    auto real_str = ctx->INT()->getText();
    real_str = '.' + real_str;
    real_val = std::stof(real_str);

    // create the real upper bound node
    std::shared_ptr<ASTNode> upper_bound = std::make_shared<RealNode>(real_val, line, col);

    // create the Range node
    std::shared_ptr<ASTNode> range_node = std::make_shared<RangeNode>(lower_bound, upper_bound, line, col);
    return range_node;
}


std::any ASTBuilder::visitRangeIntStart(GazpreaParser::RangeIntStartContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // extract the int from the int_range string ("<int>" + "..")
    auto int_range = ctx->INT_RANGE()->getText();
    auto pos = int_range.find("..");
    int_range.erase(pos, 2);
    int int_val;

    try {
        int_val = std::stoi(int_range);
    }
    catch (const std::out_of_range&) {
        throw LiteralError(line, "Integer exceeds maximum 32-bit integer");
    }
    // create the int lower bound node
    std::shared_ptr<ASTNode> lower_bound = std::make_shared<IntNode>(int_val, line, col);

    // get the upper bound expression
    auto upper_bound = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));\

    // create Range node
    std::shared_ptr<ASTNode> range_node = std::make_shared<RangeNode>(lower_bound, upper_bound, line, col);

    return range_node;
}


std::any ASTBuilder::visitUnary(GazpreaParser::UnaryContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the unary operator and the expression node
    std::string op = ctx->op->getText();
    auto expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    // create UnaryOp node
    std::shared_ptr<ASTNode> unary_op_node = std::make_shared<UnaryOpNode>(op, expr, line, col);
    return unary_op_node;
}


std::any ASTBuilder::visitTupl(GazpreaParser::TuplContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());
    // create vector that will hold the elements of the tuple literal
    auto tuple_elements = std::vector<std::shared_ptr<ASTNode>>();

    // visit each expr in the tuple and store them in the vector
    for (auto expr : ctx->expr()) {
        auto node = std::any_cast<std::shared_ptr<ASTNode>>(visit(expr));
        tuple_elements.push_back(std::move(node));
    }
    // create the tuple literal node
    std::shared_ptr<ASTNode> tuple_node = std::make_shared<TupleNode>(std::move(tuple_elements), line, col);
    return tuple_node;
}


std::any ASTBuilder::visitFuncProcCall(GazpreaParser::FuncProcCallContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the function/procedure's id
    std::string id = ctx->ID()->getText();

    // create vector that will hold the function/procedure call parameters
    auto params = std::vector<std::shared_ptr<ASTNode>>();

    // visit each parameter in the function/procedure call and store it in the vector
    for (auto expr : ctx->expr()) {
        auto node = std::any_cast<std::shared_ptr<ASTNode>>(visit(expr));
        params.push_back(std::move(node));
    }
    // create the func/proc call node
    std::shared_ptr<ASTNode> func_proc_call_node = std::make_shared<FuncProcCallNode>(id, params, line, col);
    return func_proc_call_node;
}


std::any ASTBuilder::visitGen(GazpreaParser::GenContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());
    auto id1_name = ctx->ID(0)->getText();

    auto id1 = std::make_shared<IdNode>(id1_name, line, col);

    std::shared_ptr<ASTNode> id2;
    if (ctx->ID(1))
        id2 =std::make_shared<IdNode>(ctx->ID(1)->getText(), line, col); //std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->ID(1)));

    auto vec_expr1 = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    std::shared_ptr<ASTNode> vec_expr2;
    if (ctx->expr(1) and ctx->expr(2))
        vec_expr2 = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    std::shared_ptr<ASTNode> rhs_expr;
    if (ctx->expr(2))
        rhs_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(2)));
    else
        rhs_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the generator node
    std::shared_ptr<ASTNode> gen_node = std::make_shared<GeneratorNode>(id1, id2, vec_expr1, vec_expr2, rhs_expr,
        line, col);
    return gen_node;
}


std::any ASTBuilder::visitAnd(GazpreaParser::AndContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    const std::string op = "and";

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the binary "AND" operation node
    std::shared_ptr<ASTNode> and_op_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return and_op_node;
}


std::any ASTBuilder::visitVecMatLit(GazpreaParser::VecMatLitContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    std::vector<std::shared_ptr<ASTNode>> elements;
    for (auto expr : ctx->expr()) {
        auto element = std::any_cast<std::shared_ptr<ASTNode>>(visit(expr));
        elements.push_back(element);
    }
    // create the VecMat node
    std::shared_ptr<ASTNode> vec_mat_node = std::make_shared<VecMatNode>(elements, false, line, col);
    return vec_mat_node;
}


std::any ASTBuilder::visitBy(GazpreaParser::ByContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    const std::string op = "by";

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the binary "BY" operation node
    std::shared_ptr<ASTNode> by_op_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return by_op_node;
}


std::any ASTBuilder::visitId(GazpreaParser::IdContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the id string
    std::string id = ctx->ID()->getText();

    // create the id node
    std::shared_ptr<ASTNode> id_node = std::make_shared<IdNode>(id, line, col);
    return id_node;
}


std::any ASTBuilder::visitExp(GazpreaParser::ExpContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    const std::string op = "^";

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the binary "^" operation node
    std::shared_ptr<ASTNode> exp_op_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return exp_op_node;
}


std::any ASTBuilder::visitFilt(GazpreaParser::FiltContext* ctx) {
    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    auto id_name = ctx->ID()->getText();

    auto id = std::make_shared<IdNode>(id_name, line, col);

    auto vec_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));

    std::vector<std::shared_ptr<ASTNode>> pred_exprs;
    for (int i = 1; i < static_cast<int>(ctx->expr().size()); i++)
        pred_exprs.push_back(std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(i))));

    // create the filter node
    std::shared_ptr<ASTNode> filt_node = std::make_shared<FilterNode>(id, vec_expr, pred_exprs, line, col);
    return filt_node;
}


std::any ASTBuilder::visitBuiltInFunc(GazpreaParser::BuiltInFuncContext* ctx) {
    return std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->builtIn()));
}


std::any ASTBuilder::visitLtGt(GazpreaParser::LtGtContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the op (either "<" or ">")
    const std::string op = ctx->op->getText();

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the binary "> / <" operation node
    std::shared_ptr<ASTNode> ltgt_op_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return ltgt_op_node;
}


std::any ASTBuilder::visitOrXor(GazpreaParser::OrXorContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the op (either "OR" or "XOR")
    const std::string op = ctx->op->getText();

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the binary "OR / XOR" operation node
    std::shared_ptr<ASTNode> orxor_op_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return orxor_op_node;
}


std::any ASTBuilder::visitIndex(GazpreaParser::IndexContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the collection node (vector or matrix)
    auto collection = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));

    // get the first index expression
    auto index1 = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // get the second index expression (if it exists)
    std::shared_ptr<ASTNode> index2 = nullptr;
    if (ctx->expr(2)) {
        index2 = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(2)));
    }
    // create the index node
    std::shared_ptr<ASTNode> index_node = std::make_shared<IndexNode>(collection, index1, index2, line, col);

    return index_node;
}


std::any ASTBuilder::visitAddSub(GazpreaParser::AddSubContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the op (either "+" or "-")
    const std::string op = ctx->op->getText();

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the binary "+ \ -" operation node
    std::shared_ptr<ASTNode> addsub_op_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return addsub_op_node;
}


std::any ASTBuilder::visitReal(GazpreaParser::RealContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the real value string
    auto real_str = ctx->REAL()->getText();

    // we have to handle the case where the real string is out of range for std::stof
    float val;
    try {
        val = std::stof(real_str);
    } catch (const std::out_of_range& e) {
         val = std::numeric_limits<float>::infinity();
    }
    // create the real node
    std::shared_ptr<ASTNode> real_node = std::make_shared<RealNode>(val, line, col);
    return real_node;
}


std::any ASTBuilder::visitConcat(GazpreaParser::ConcatContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    const std::string op = "||";

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    std::shared_ptr<ASTNode> concat_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return concat_node;
}


std::any ASTBuilder::visitEqNe(GazpreaParser::EqNeContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the op (either "==" or "!=")
    const std::string op = ctx->op->getText();

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the binary "== \ !=" operation node
    std::shared_ptr<ASTNode> eqne_op_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return eqne_op_node;
}


std::any ASTBuilder::visitInt(GazpreaParser::IntContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the integer value string
    const int val = std::stoi(ctx->INT()->getText());

    // create the integer node
    std::shared_ptr<ASTNode> int_node = std::make_shared<IntNode>(val, line, col);
    return int_node;
}


std::any ASTBuilder::visitTypeCast(GazpreaParser::TypeCastContext* ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the type to cast to
    std::shared_ptr<Type> cast_to = nullptr;
    std::shared_ptr<Type> node_type = nullptr;
    if(ctx->declType()) {
        cast_to = declTypeToType(ctx->declType(), line);
        node_type = declTypeToType(ctx->declType(), line);
    }
    // get the expression we want to cast
    auto expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    // create the typecast node

    std::shared_ptr<ASTNode> typecast_node = std::make_shared<TypeCastNode>(cast_to, expr, line, col);
    typecast_node->type = node_type; //Fucked solution but it works


    return typecast_node;
}


std::any ASTBuilder::visitChar(GazpreaParser::CharContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    const std::string charString = ctx->CHAR()->getText();

    // get the character value string
    if (charString.size()==3) {
        const char val = charString[1];
        std::shared_ptr<ASTNode> char_node = std::make_shared<CharNode>(val, line, col);
        return char_node;
    }
    if (charString.size()==4 && charString[1]=='\\' ) {
        const char checkChar = charString[2];
        char val;
        switch (checkChar) {
            case '0':  val = '\0'; break;
            case 'a':  val = '\a'; break;
            case 'b':  val = '\b'; break;
            case 't':  val = '\t'; break;
            case 'n':  val = '\n'; break;
            case 'r':  val = '\r'; break;
            case '"':  val = '"';  break;
            case '\'': val = '\''; break;
            case '\\': val = '\\'; break;
            default:
                throw std::invalid_argument("Invalid character");
        }
        std::shared_ptr<ASTNode> char_node = std::make_shared<CharNode>(val, line, col);
        return char_node;
    }
    throw SyntaxError(line,"token recognition error at: '"+charString+" '");
}


std::any ASTBuilder::visitMultDivRem(GazpreaParser::MultDivRemContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the op (either "*", "/", "%", or "**")
    const std::string op = ctx->op->getText();

    // get the left and right expressions
    auto l_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));
    auto r_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the binary "* \ / \ % \ **" operation node
    std::shared_ptr<ASTNode> multdivrem_op_node = std::make_shared<BinaryOpNode>(op, l_expr, r_expr, line, col);
    return multdivrem_op_node;
}


std::any ASTBuilder::visitDecl(GazpreaParser::DeclContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the qualifier (if it exists)
    bool constant = false;
    if (ctx->QUALIFIER() != nullptr and ctx->QUALIFIER()->getText() == "const") {
        constant = true;
    }
    // get the id
    std::string id = ctx->ID()->getText();

    //get the type if it exists else infer it
    std::shared_ptr<Type> type = nullptr;
    std::string stringType;
    if(ctx->declType()) {
        type = declTypeToType(ctx->declType(), line);
        stringType = ctx->declType()->getText();
    }
    // get the assign expression if it exists
    std::shared_ptr<ASTNode> expr = nullptr;
    if(ctx->expr()){
      expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));
    }
    // create the declaration node
    std::shared_ptr<ASTNode> decl_node = std::make_shared<DeclNode>(constant, type, stringType, id, expr, line, col);
    return decl_node;
}


std::any ASTBuilder::visitTypedef(GazpreaParser::TypedefContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the id of the new type
    std::string id = ctx->ID()->getText();
    std::shared_ptr<Type> type;
    if (typedefMap.find(id) != typedefMap.end()) {
        throw SymbolError(line,"Duplicate typedefs");
    }
    type = declTypeToType(ctx->declType(), line);

    typedefMap[id] = type;
    // create the Typedef node
    std::shared_ptr<ASTNode> typedef_node = std::make_shared<TypeDefNode>(type, id, line, col);
    return typedef_node;
}


std::any ASTBuilder::visitIdAssign(GazpreaParser::IdAssignContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the id/expr
    std::string id = ctx->ID()->getText();

    auto expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));
    std::shared_ptr<ASTNode> id_assign_node = std::make_shared<IdAssignNode>(id, expr, line, col);

    return id_assign_node;
}


std::any ASTBuilder::visitVecMatTupAssign(GazpreaParser::VecMatTupAssignContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the expression to be assigned to (vec/mat index or tuple element)
    // if it's not one of those expressions, an AST pass will detect it.
    auto expr_assign_to = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(0)));

    // get the expression that will be assigned
    auto expr_assign_with = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr(1)));

    // create the assign node
    std::shared_ptr<ASTNode> vecmattup_assign_node = std::make_shared<VecMatTupAssignNode>(
        expr_assign_to, expr_assign_with, line, col);
    return vecmattup_assign_node;
}


std::any ASTBuilder::visitTuplUnpackAssign(GazpreaParser::TuplUnpackAssignContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the i-values we want to assign to and store them in a vector
    std::vector<std::shared_ptr<ASTNode>> ivalues;
    for (auto expr : ctx->expr()) {
        auto node = std::any_cast<std::shared_ptr<ASTNode>>(visit(expr));
        ivalues.push_back(node);
    }

    // get the tuple
    auto tuple = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->tupleExpr()));

    // make the unpack node
    std::shared_ptr<ASTNode> unpack_node = std::make_shared<UnpackNode>(ivalues, tuple, line, col);
    return unpack_node;
}


std::any ASTBuilder::visitCond(GazpreaParser::CondContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the control expression
    auto contr_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    // check if the "if" block is a single line, or a block surrounded by braces
    std::shared_ptr<BlockNode> if_block;
    if (ctx->stat()->blockStat()) { // if its a block surrounded by braces
        // get the block
        if_block = std::any_cast<std::shared_ptr<BlockNode>>(visit(ctx->stat()->blockStat()->block()));
    }
    else { // its a single statement, turn it into a block with 1 statement
        auto stat = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->stat()));
        std::vector<std::shared_ptr<ASTNode>> stat_vector = {stat};
        if_block = std::make_shared<BlockNode>(stat_vector, ctx->stat()->getStart()->getLine(),
            ctx->stat()->getStart()->getCharPositionInLine());
    }
    // if there is an else block, we need to add that to the node, otherwise, its a null block ptr
    std::shared_ptr<BlockNode> else_block;
    if (ctx->condElse()) {
        // check if the "else" block is a single line, or a block surrounded by braces
        if (ctx->condElse()->stat()->blockStat()) { // if its a block surrounded by braces
            // get the block
            else_block = std::any_cast<std::shared_ptr<BlockNode>>(visit(ctx->condElse()->stat()->blockStat()->block()));
        }
        else { // it's a single statement, turn it into a block with 1 statement
            auto stat = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->condElse()->stat()));
            std::vector<std::shared_ptr<ASTNode>> stat_vector = {stat};
            else_block = std::make_shared<BlockNode>(stat_vector, ctx->condElse()->stat()->getStart()->getLine(),
                ctx->condElse()->stat()->getStart()->getCharPositionInLine());
        }
    }
    // create the conditional node
    std::shared_ptr<ASTNode> cond_node = std::make_shared<CondNode>(contr_expr, if_block, else_block, line, col);
    return cond_node;
}


std::any ASTBuilder::visitFuncDef(GazpreaParser::FuncDefContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the function id
    std::string id = ctx->ID()->getText();

    // populate the function parameters vector (each tuple in the vector is a parameter)
    std::vector<std::tuple<std::shared_ptr<Type>, std::string>> param_vector;
    for (auto param : ctx->funcParam()) {
        std::string param_id;

        // get the type object
        std::shared_ptr<Type> type = declTypeToType(param->declType(), line);
        if (type == nullptr)
            throw TypeError(line, "Unsupported base type for user defined type");

        // get the id, if it exists
        if (param->ID())
            param_id = param->ID()->getText();

        // add the parameter to the parameter list
        std::tuple<std::shared_ptr<Type>, std::string> complete_parameter(type, param_id);
        param_vector.push_back(complete_parameter);
    }
    // get the return type
    std::shared_ptr<Type> return_type = declTypeToType(ctx->declType(), line);
    if (return_type == nullptr)
        throw TypeError(line, "Unsupported base type for user defined type");

    // check if the function is a single line function, multi-line function, or just a prototype
    std::shared_ptr<BlockNode> block;
    if (ctx->expr()) { // single line, we need to create a block object with 1 return statement
        auto return_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));
        std::shared_ptr<ASTNode> return_stat = std::make_shared<ReturnNode>(return_expr,
            ctx->KW_RETURNS()->getSymbol()->getLine(),ctx->KW_RETURNS()->getSymbol()->getCharPositionInLine());
        std::vector<std::shared_ptr<ASTNode>> stat_vector = {return_stat};
        block = std::make_shared<BlockNode>(stat_vector, ctx->declType()->getStart()->getLine(),
            ctx->declType()->getStart()->getCharPositionInLine());
    }
    else if (ctx->blockStat()) { // multi line, the block is already made for us
        block = std::any_cast<std::shared_ptr<BlockNode>>(visit(ctx->blockStat()->block()));
    }
    // make the Function definition node
    std::shared_ptr<ASTNode> func_def_node = std::make_shared<FuncDefNode>(id, param_vector, return_type, block, line, col);
    return func_def_node;
}


std::any ASTBuilder::visitProcDef(GazpreaParser::ProcDefContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the procedure id
    std::string id = ctx->ID()->getText();

    // procedure parameters vector (each tuple in the vector is a parameter)
    std::vector<std::tuple<bool, std::shared_ptr<Type>, std::string>> param_vector;

    for (auto param : ctx->procParam()) {
        bool constant = true;
        std::string param_id;

        // set the qualifier (const is default)
        if (param->QUALIFIER() && param->QUALIFIER()->getText() == "var")
            constant = false;

        // get the type object
        std::shared_ptr<Type> type = declTypeToType(param->declType(), line);
        if (type == nullptr)
            throw TypeError(line, "Unsupported base type for user defined type");

        // get the id, if it exists
        if (param->ID())
            param_id = param->ID()->getText();

        // add the parameter to the parameter list
        std::tuple<bool, std::shared_ptr<Type>, std::string> complete_parameter(constant, type, param_id);
        param_vector.push_back(complete_parameter);
    }
    // get the return type, if it exists
    std::shared_ptr<Type> return_type;
    if (ctx->KW_RETURNS()){
      return_type = declTypeToType(ctx->declType(), line);
        if (return_type == nullptr)
            throw TypeError(line, "Unsupported base type for user defined type");
    }
    // get the procedure block, if it exists
    std::shared_ptr<BlockNode> block;
    if (ctx->blockStat())
        block = std::any_cast<std::shared_ptr<BlockNode>>(visit(ctx->blockStat()->block()));

    // if the procedure's block exists, and the procedure doesn't return anything, append an empty return statement
    // to the end of the procedure block in case it doesn't have one already. If it does, then this extra return
    // will be pruned in a later AST validation pass
    if (block and !return_type) {
      std::shared_ptr<ASTNode> expr;
      std::shared_ptr<ASTNode> return_node = std::make_shared<ReturnNode>(expr, line, col);
      block->stat_nodes.push_back(return_node);
    }
    // make the procedure declaration node
    std::shared_ptr<ASTNode> proc_def_node = std::make_shared<ProcDefNode>(id, param_vector, return_type, block, line, col);
    return proc_def_node;
}


std::any ASTBuilder::visitInfPostPredLoop(GazpreaParser::InfPostPredLoopContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the loop block: check if single statement or block statement
    std::shared_ptr<BlockNode> loop_block;
    if (ctx->stat()->blockStat()) { // its a block statement
        loop_block = std::any_cast<std::shared_ptr<BlockNode>>(visit(ctx->stat()->blockStat()->block()));
    }
    else { // its a single statement, turn it into a 1 statement block node
        auto stat = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->stat()));
        std::vector<std::shared_ptr<ASTNode>> stat_vector = {stat};
        loop_block = std::make_shared<BlockNode>(stat_vector, ctx->stat()->getStart()->getLine(),
            ctx->stat()->getStart()->getCharPositionInLine());
    }
    // if there is no while keyword present, we know its an infinite loop
    if (!ctx->KW_WHILE()) {
        // make the infinite loop node
        std::shared_ptr<ASTNode> inf_loop_node = std::make_shared<InfLoopNode>(loop_block, line, col);
        return inf_loop_node;
    }
    // if we make it here, we are dealing with a post predicated loop
    // get the control expression
    auto contr_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    // make the post-predicated loop node (the false just signals to the constructor that the node is not pre-predicated)
    std::shared_ptr<ASTNode> post_pred_loop_node = std::make_shared<PredLoopNode>(false, contr_expr, loop_block, line, col);
    return post_pred_loop_node;
}


std::any ASTBuilder::visitPrePredLoop(GazpreaParser::PrePredLoopContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the control expression
    auto contr_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    // get the loop block: check if single statement or block statement
    std::shared_ptr<BlockNode> loop_block;
    if (ctx->stat()->blockStat()) { // its a block statement
        loop_block = std::any_cast<std::shared_ptr<BlockNode>>(visit(ctx->stat()->blockStat()->block()));
    }
    else { // its a single statement, turn it into a 1 statement block node
        auto stat = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->stat()));
        std::vector<std::shared_ptr<ASTNode>> stat_vector = {stat};
        loop_block = std::make_shared<BlockNode>(stat_vector, ctx->stat()->getStart()->getLine(),
            ctx->stat()->getStart()->getCharPositionInLine());
    }
    // make the pre-predicated loop node (the true just signals to the constructor that the node is pre-predicated)
    std::shared_ptr<ASTNode> pre_pred_loop_node = std::make_shared<PredLoopNode>(true, contr_expr, loop_block, line, col);
    return pre_pred_loop_node;
}


std::any ASTBuilder::visitIterLoop(GazpreaParser::IterLoopContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the domain variable id
    auto id = std::any_cast<std::string>(ctx->ID()->getText());

    // get the domain expression
    auto dom_expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    // get the loop block: check if single statement or block statement
    std::shared_ptr<BlockNode> loop_block;
    if (ctx->stat()->blockStat()) { // its a block statement
        loop_block = std::any_cast<std::shared_ptr<BlockNode>>(visit(ctx->stat()->blockStat()->block()));
    }
    else { // its a single statement, turn it into a 1 statement block node
        auto stat = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->stat()));
        std::vector<std::shared_ptr<ASTNode>> stat_vector = {stat};
        loop_block = std::make_shared<BlockNode>(stat_vector, ctx->stat()->getStart()->getLine(),
            ctx->stat()->getStart()->getCharPositionInLine());
    }
    // make the iterator loop node
    std::shared_ptr<ASTNode> iter_loop_node = std::make_shared<IterLoopNode>(id, dom_expr, loop_block, line, col);
    return iter_loop_node;
}


std::any ASTBuilder::visitInStream(GazpreaParser::InStreamContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the l_value to write to
    auto l_value =  std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    if (not (std::dynamic_pointer_cast<IdNode>(l_value)
                or std::dynamic_pointer_cast<TupleAccessNode>(l_value)or
         std::dynamic_pointer_cast<IndexNode>(l_value))){
        throw TypeError(line, "Cannot assign a value from std_input to: " + ctx->expr()->getText());
    }
    std::shared_ptr<ASTNode> id_stream_node = std::make_shared<InStreamNode>(l_value, line, col);

    return id_stream_node;
}

std::any ASTBuilder::visitOutStream(GazpreaParser::OutStreamContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the expression to print
    auto expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    // make the output stream node
    std::shared_ptr<ASTNode> stream_node = std::make_shared<OutStreamNode>(expr, line, col);
    return stream_node;
}


std::any ASTBuilder::visitBuiltIn(GazpreaParser::BuiltInContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    std::shared_ptr<ASTNode> built_in_call_node;
    if (ctx->KW_LENGTH()) {
        built_in_call_node = createLengthCallNode(
            std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr())), line, col);
    }
    else if (ctx->KW_ROWS()) {
        built_in_call_node = createRowsCallNode(
            std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr())), line, col);
    }
    else if (ctx->KW_COLUMNS()) {
        built_in_call_node = createColumnsCallNode(
            std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr())), line, col);
    }
    else if (ctx->KW_FORMAT()) {
        built_in_call_node = createFormatNode(
            std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr())), line, col);
    }
    else if (ctx->KW_STREAM_STATE()) {
        built_in_call_node = createStreamStateNode(line, col);
    }
    else if (ctx->KW_REVERSE()) {
        built_in_call_node = createReverseNode(
            std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr())), line, col);
    }
    return built_in_call_node;
}


std::shared_ptr<ASTNode> ASTBuilder::createLengthCallNode(const std::shared_ptr<ASTNode>& vector, int line, int col) {

    std::shared_ptr<ASTNode> length_call_node = std::make_shared<LengthCallNode>(vector, line, col);
    return length_call_node;
}


std::shared_ptr<ASTNode> ASTBuilder::createRowsCallNode(const std::shared_ptr<ASTNode>& matrix, int line, int col) {

    std::shared_ptr<ASTNode> rows_call_node = std::make_shared<RowsCallNode>(matrix, line, col);
    return rows_call_node;
}


std::shared_ptr<ASTNode> ASTBuilder::createColumnsCallNode(const std::shared_ptr<ASTNode>& matrix, int line, int col) {

    std::shared_ptr<ASTNode> columns_call_node = std::make_shared<ColumnsCallNode>(matrix, line, col);
    return columns_call_node;
}


std::shared_ptr<ASTNode> ASTBuilder::createFormatNode(const std::shared_ptr<ASTNode>& scalar, int line, int col) {

    std::shared_ptr<ASTNode> format_call_node = std::make_shared<FormatCallNode>(scalar, line, col);
    return format_call_node;
}


std::shared_ptr<ASTNode> ASTBuilder::createStreamStateNode(int line, int col) {

    std::shared_ptr<ASTNode> stream_state_call_node = std::make_shared<StreamStateCallNode>(line, col);
    return stream_state_call_node;
}


std::shared_ptr<ASTNode> ASTBuilder::createReverseNode(const std::shared_ptr<ASTNode>& vector, int line, int col) {

    std::shared_ptr<ASTNode> reverse_call_node = std::make_shared<ReverseCallNode>(vector, line, col);
    return reverse_call_node;
}


std::any ASTBuilder::visitReturn(GazpreaParser::ReturnContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the return expr, if it exists
    std::shared_ptr<ASTNode> expr;
    if (ctx->expr())
        expr = std::any_cast<std::shared_ptr<ASTNode>>(visit(ctx->expr()));

    // create the return node
    std::shared_ptr<ASTNode> return_node = std::make_shared<ReturnNode>(expr, line, col);

    return return_node;
}


std::any ASTBuilder::visitBreak(GazpreaParser::BreakContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // create the break node
    std::shared_ptr<ASTNode> break_node = std::make_shared<BreakNode>(line, col);
    return break_node;
}


std::any ASTBuilder::visitContinue(GazpreaParser::ContinueContext *ctx) {

    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // create the continue node
    std::shared_ptr<ASTNode> continue_node = std::make_shared<ContinueNode>(line, col);
    return continue_node;
}


std::any ASTBuilder::visitRawProcCall(GazpreaParser::RawProcCallContext *ctx) {
    int line = static_cast<int>(ctx->getStart()->getLine());
    int col = static_cast<int>(ctx->getStart()->getCharPositionInLine());

    // get the procedure's id
    std::string id = ctx->ID()->getText();

    // create vector that will hold the function/procedure call parameters
    auto params = std::vector<std::shared_ptr<ASTNode>>();

    // visit each parameter in the function/procedure call and store it in the vector
    for (auto expr : ctx->expr()) {
        auto node = std::any_cast<std::shared_ptr<ASTNode>>(visit(expr));
        params.push_back(std::move(node));
    }
    // create the func/proc call node
    auto temp = std::make_shared<FuncProcCallNode>(id, params, line, col);
    temp->call_used = true;

    std::shared_ptr<ASTNode> proc_call_node = temp;
    return proc_call_node;
}
