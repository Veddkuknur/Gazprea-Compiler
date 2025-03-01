#ifndef GAZPREABASE_SYMBOLTABLE_H
#define GAZPREABASE_SYMBOLTABLE_H

#include <vector>
#include "Scope.h"

class SymbolTable {
protected:
    void initTypeSystem() const;
public:
    std::shared_ptr<GlobalScope> globals; //Global scope inside the table
    std::shared_ptr<Scope> currentScope; //Pointer to current scope the table is pointing at.

    std::vector<std::shared_ptr<Scope>> scopes; //Stack of scopes within the table

    SymbolTable();

    void push_scope(std::shared_ptr<Scope> child) {
        //Push a new scope inside the table, set the enclosing scope of the scope as the current scope.
        child->setEnclosingScope(currentScope);
        currentScope = child; //Set the new scope as the current scope
        scopes.push_back(child); //Push scope in the stack
    }

    void pop_scope() {
        //Go one step above in the stack, note that we do not remove all scopes from the table.
        currentScope = currentScope->getEnclosingScope();
    }

    std::string toString();
};

#endif //GAZPREABASE_SYMBOLTABLE_H