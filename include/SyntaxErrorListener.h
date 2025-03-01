#ifndef GAZPREABASE_SYNTAXERRORLISTENER_H
#define GAZPREABASE_SYNTAXERRORLISTENER_H

#include "antlr4-runtime.h"
#include "CompileTimeExceptions.h"

class SyntaxErrorListener : public antlr4::BaseErrorListener {
public:
  void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
    size_t charPositionInLine, const std::string &msg, std::exception_ptr e) override ;
};

#endif //GAZPREABASE_SYNTAXERRORLISTENER_H
