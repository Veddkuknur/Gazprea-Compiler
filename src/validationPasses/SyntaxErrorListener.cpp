#include "SyntaxErrorListener.h"


void SyntaxErrorListener::syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
  size_t charPositionInLine, const std::string &msg, std::exception_ptr e) {

  throw SyntaxError(line, msg);
}
