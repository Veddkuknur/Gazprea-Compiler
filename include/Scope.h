#ifndef SCOPE_H
#define SCOPE_H

#include <map>
#include <memory>
#include <string>
#include <mlir/IR/Block.h>

class Symbol;
class Scope: public std::enable_shared_from_this<Scope> {
public:

    virtual std::string getScopeName() = 0;

    /** Set the enclosing scope */
    virtual void setEnclosingScope(std::shared_ptr<Scope> scope) = 0;

    /** Return the enclosing scope */
    virtual std::shared_ptr<Scope> getEnclosingScope() = 0;

    /** Define a symbol in the current scope */
    virtual void define(std::shared_ptr<Symbol> sym) = 0;

    /** Look up name in this scope or in enclosing scope if not here */
    virtual std::shared_ptr<Symbol> resolve(const std::string &name,bool current_only = false)=0;
    virtual std::shared_ptr<Symbol> resolveProcFunc(const std::string &name, bool current_only = false) =0 ;

    virtual std::shared_ptr<Scope> inLoopScope() = 0;

    virtual std::string toString() = 0;
    virtual ~Scope();
};


//Base scope implements our abstract class scope. It contains virtual
class BaseScope : public Scope {
public:

    std::shared_ptr<Scope> enclosingScope; // nullptr if global (outermost) scope
    std::map<std::string, std::shared_ptr<Symbol>> symbols;
    std::vector<std::shared_ptr<Symbol>> symbols_to_free;
    // std::vector<std::shared_ptr<Symbol>> global_symbols_to_free;

    BaseScope() : enclosingScope(nullptr) {}

    std::shared_ptr<Symbol> resolve(const std::string &name, bool current_only) override;
    std::shared_ptr<Symbol> resolveProcFunc(const std::string &name, bool current_only) override;
    //We set these to virtual methods to allow for future expansion
    //Virtual method define might change depending on the type of scope, however for vcalc they are the exact same
    virtual void define(std::shared_ptr<Symbol> sym) override;
    std::shared_ptr<Scope> getEnclosingScope() override;
    virtual void setEnclosingScope(std::shared_ptr<Scope> scope) override;
    virtual std::string toString() override;
    std::shared_ptr<Scope> inLoopScope() override;

};


//Create class for global scope and Local scope, global scope gets created automatically when once starts a symbol table
class GlobalScope : public BaseScope {
public:
    GlobalScope() {}
    std::string getScopeName() override {
        return "global";
    }
};
//Local scopes get defined during the
class LocalScope : public BaseScope {
public:
    LocalScope() {}
    std::string getScopeName() override { return "local"; }
};


//Local scopes get defined during the
class TupleScope : public BaseScope {
public:
    TupleScope() {}
    std::string getScopeName() override { return "tuple"; }
};

class LoopScope : public LocalScope {
public:

    mlir::Block * header_block;
    mlir::Block * continue_block;
    LoopScope() {}
    std::string getScopeName() override { return "loop"; }
};



#endif //SCOPE_H


