//
// Created by judafer2000 on 10/31/24.
//

#include <sstream>
#include "ScopedSymbol.h"


ScopedSymbol::ScopedSymbol( std::string name, std::shared_ptr<Type> retType,
                            std::shared_ptr<Scope> enclosingScope)
        : Symbol(name, retType), enclosingScope(enclosingScope) {}

std::shared_ptr<Symbol> ScopedSymbol::resolve(const std::string &name, bool current_scope) {
    for ( auto sym : orderedArgs ) {
        if ( sym->getName() == name ) {
            return sym;
        }
    }
    // if not here, check any enclosing scope
    if ( getEnclosingScope() != nullptr ) {
        return getEnclosingScope()->resolve(name);
    }
    return nullptr; // not found
}


void ScopedSymbol::define(std::shared_ptr<Symbol> sym) {
    orderedArgs.push_back(sym);
    sym->scope = shared_from_this();
}


std::shared_ptr<Scope> ScopedSymbol::getEnclosingScope() {
    return enclosingScope;
}


std::string ScopedSymbol::getScopeName() {
    return name;
}


std::string FunctionSymbol::toString() {
    std::stringstream str;
    str << "Function" << Symbol::toString() << ":{";
    for (auto iter = orderedArgs.begin(); iter != orderedArgs.end(); iter++) {
        std::shared_ptr<Symbol> sym = *iter;
        if ( iter != orderedArgs.begin() ) str << ", ";
        str << sym->toString();
    }
    str << "}";
    return str.str();
}


std::string ProcedureSymbol::toString() {
    std::stringstream str;
    str << "Procedure " << Symbol::toString() << ":{";
    for (auto iter = orderedArgs.begin(); iter != orderedArgs.end(); iter++) {
        std::shared_ptr<Symbol> sym = *iter;
        if ( iter != orderedArgs.begin() ) str << ", ";
        str << sym->toString();
    }
    str << "}";
    return str.str();
}

