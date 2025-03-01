#include "Scope.h"

#include "ScopedSymbol.h"

#include "Symbol.h"

#include <sstream>
#include <string>


std::shared_ptr<Scope> BaseScope::inLoopScope(){

  if (getScopeName() == "loop"){
        return shared_from_this();
    }
    if (enclosingScope != nullptr) {
        return getEnclosingScope()->inLoopScope();
    }
    return nullptr;
}


std::shared_ptr<Symbol> BaseScope::resolve(const std::string &name, bool current_only) {


  auto find_symbol = symbols.find(name);
  if ( find_symbol != symbols.end()) { //If symbol is found in map then access the symbol inside
        return find_symbol->second;
  }
  if (getScopeName() == "tuple"){
      return nullptr;
  }
  // if not here, check any enclosing
  if (not current_only){
      if ( enclosingScope != nullptr ) {
        return enclosingScope->resolve(name); //Otherwise go onto the enclosing scope.
      }
    }
  return nullptr; // not found
}



std::shared_ptr<Symbol> BaseScope::resolveProcFunc(const std::string &name, bool current_only) {

  //Find name of the symbol.
  auto find_symbol = symbols.find(name);
  if ( find_symbol != symbols.end() and std::dynamic_pointer_cast<ScopedSymbol>(find_symbol->second)) { //If symbol is found in map then access the symbol inside
    return find_symbol->second;
  }
  if (getScopeName() == "tuple"){
    return nullptr;
  }
  // if not here, check any enclosing
  if (not current_only){
    if ( enclosingScope != nullptr ) {
      return enclosingScope->resolveProcFunc(name, current_only); //Otherwise go onto the enclosing scope.
    }
  }
  return nullptr; // not found
}




void BaseScope::define(std::shared_ptr<Symbol> sym) {
  symbols.emplace(sym->name, sym);
  sym->scope = shared_from_this(); // track the scope in each symbol
}


std::shared_ptr<Scope> BaseScope::getEnclosingScope() {
  //Return enclosing scope of hash map
  return enclosingScope;
}


void BaseScope::setEnclosingScope(std::shared_ptr<Scope> scope) {
  enclosingScope = scope;
}


std::string BaseScope::toString() {
  std::stringstream str;
  str << "Scope " << getScopeName() << " {";
  for (auto iter = symbols.begin(); iter != symbols.end(); iter++) {
    std::shared_ptr<Symbol> sym = iter->second;
    if ( iter != symbols.begin() ) str << ", ";
    str <<  sym->toString();
  }
  str << "}";
  return str.str();
}

Scope::~Scope() {}
