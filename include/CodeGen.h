#pragma once
#include "AST.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <map>
#include <memory>
#include <string>

struct TypedValue {
    llvm::Value *value;
    TypeKind type;
};

struct VariableInfo {
    TypeKind type;
    llvm::AllocaInst *alloca;
};

class CodeGen {
public:
    CodeGen();

    void generate(const Program &program);

    void printIR();
private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::map<std::string, VariableInfo> namedValues;
    std::map<std::string, TypeKind> functionReturnTypes;
    TypeKind currentReturnType = TypeKind::Void;
    llvm::Type* llvmType(TypeKind type);
    llvm::Constant* zeroValue(TypeKind type);
    bool isInteger(TypeKind type) const;
    bool isFloating(TypeKind type) const;
    TypeKind commonType(TypeKind lhs, TypeKind rhs) const;
    TypedValue castTo(TypedValue input, TypeKind targetType);
    llvm::AllocaInst* createEntryBlockAlloca(
        llvm::Function* function,
        const std::string& name,
        llvm::Type* type
    );
    llvm::Function* declareFunction(const Function& fn);
    void codegenFunctionBody(const Function& fn);
    void codegenBlock(const BlockStmt& block);
    void codegenStmt(const Stmt& stmt);
    TypedValue codegenExpr(const Expr& expr);
    llvm::Value* truthy(TypedValue value);
};
