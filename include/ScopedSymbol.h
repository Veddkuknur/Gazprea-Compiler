#ifndef GAZPREABASE_SCOPEDSYMBOL_H
#define GAZPREABASE_SCOPEDSYMBOL_H

#include "Symbol.h"
#include "BackEnd.h"

class ScopedSymbol : public Symbol, public BaseScope {
protected:
    std::shared_ptr<Scope> enclosingScope;
public:
    ScopedSymbol(std::string name, std::shared_ptr<Type> return_type, std::shared_ptr<Scope> enclosing_scope);
    std::shared_ptr<Symbol> resolve(const std::string &name, bool current_scope);
    void define(std::shared_ptr<Symbol> sym);
    std::shared_ptr<Scope> getEnclosingScope();
    std::string getScopeName();
    virtual std::string toString() = 0;

    std::vector<std::shared_ptr<Symbol>> orderedArgs;
};


class FunctionSymbol : public ScopedSymbol {
public:
    bool prototype = false;
    std::vector<std::shared_ptr<Type>> param_types;
    
    FunctionSymbol(std::string name, std::shared_ptr<Type> retType, std::shared_ptr<Scope> enclosingScope): ScopedSymbol(name, retType, enclosingScope){};


    std::string toString() override;

};
class ProcedureSymbol : public ScopedSymbol {
public:
    bool prototype = false;
    std::vector<std::shared_ptr<Type>> param_types;

    ProcedureSymbol(std::string name, std::shared_ptr<Type> retType, std::shared_ptr<Scope> enclosingScope): ScopedSymbol(name, retType, enclosingScope){};
    std::string toString() override;

};

#endif //GAZPREABASE_SCOPEDSYMBOL_H
