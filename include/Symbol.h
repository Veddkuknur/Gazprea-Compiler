#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <memory>
#include <any>

#include "Type.h"
#include "Scope.h"

class Scope; // forward declaration of Scope to resolve circular dependency

class Symbol { // A generic programming language symbol
public:
    std::string name;               // Name of the symbol
    std::shared_ptr<Type> type;     // Type of the symbols, all our symbols are typed.
    std::shared_ptr<Scope> scope;   // All symbols know what scope contains them. They are defined inside of the BaseScope.define method

    std::any value = nullptr;                 // for interpreter: could be C++ int or vector
    bool mutability = false;
    Symbol(const std::string& name);
    Symbol(const std::string &name, std::shared_ptr<Type> type);
    Symbol(const std::string &name, std::shared_ptr<Type> type, bool mutability);

    Symbol(std::string name, std::shared_ptr<Type> type, std::shared_ptr<Scope> scope);
    virtual std::string getName();

    virtual std::string toString();
    virtual ~Symbol();
};


class VariableSymbol : public Symbol {
public:
    VariableSymbol(std::string name, std::shared_ptr<Type> type, bool mutability) //Variable symbols contain a name and a type.
        : Symbol(name, type, mutability) {}
};


class TypedSymbol: public Symbol {
public:
    TypedSymbol(std::string name, std::shared_ptr<Type> sym_type): Symbol(name, std::move(sym_type)){};
    std::string getName();
};

#endif //SYMBOLS_H