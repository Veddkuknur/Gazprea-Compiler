#include "SymbolTable.h"

#include "Symbol.h"


void SymbolTable::initTypeSystem() const {

    //There is no need to initialize types with anything more complicated than a string, as we won't know what the
    //size of a collection will be until run time, hence we won't be able to detect any bugs regarding sizes until runtime
    //Initialize built in types
    globals->define(std::make_shared<TypedSymbol>("integer", std::make_shared<PrimitiveType>(Type::BaseType::integer)));
    globals->define(std::make_shared<TypedSymbol>("real", std::make_shared<PrimitiveType>(Type::BaseType::real)));
    globals->define(std::make_shared<TypedSymbol>("boolean", std::make_shared<PrimitiveType>(Type::BaseType::boolean)));
    globals->define(std::make_shared<TypedSymbol>("character", std::make_shared<PrimitiveType>(Type::BaseType::character)));

    //Collection types
    /*globals->define(std::make_shared<TypedSymbol>("matrix"));
    globals->define(std::make_shared<TypedSymbol>("vector"));
    globals->define(std::make_shared<TypedSymbol>("tuple"));
    globals->define(std::make_shared<TypedSymbol>("string"));*/

}


SymbolTable::SymbolTable() : globals(std::make_shared<GlobalScope>()) {
    //Initialize built in types
    initTypeSystem();
    //Set the current scope as the global scope
    currentScope = globals;
    //Push scope to scope table at initialization
    scopes.push_back(globals);
}


std::string SymbolTable::toString() {
    //Return string representation of symbol table
    std::string symtab = "SymbolTable\n";
    for (auto s : scopes) {
        symtab += s->toString() + "\n";
    }
    return symtab;
}
