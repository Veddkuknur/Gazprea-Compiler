#include "Symbol.h"
#include "ScopedSymbol.h"


Symbol::Symbol(const std::string& name)
    : Symbol(name, nullptr) {}


Symbol::Symbol(const std::string &name, std::shared_ptr<Type> type)
    : name(name), type(type) {}

Symbol::Symbol(const std::string &name, std::shared_ptr<Type> type, bool mutability)
    : name(name), type(type), mutability(mutability){}


std::string Symbol::getName() {
    return name;
}

// Return string representation of symbol
std::string Symbol::toString() {
    std::string sname = (scope)? scope->getScopeName() + "::" : "";
    std::string mutability_str = mutability ? "var" : "const";

    if (type != nullptr) return '<' + sname + mutability_str + ":" + getName() + ":" + type->getTypeAsString() + '>';
    return getName();
}


std::string TypedSymbol::getName() {
    //Small Test
    return Symbol::getName();
}


Symbol::~Symbol() {}