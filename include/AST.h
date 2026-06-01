#pragma once
#include <memory>
#include <string>
#include <vector>

enum class TypeKind {
    Short,
    Int,
    Float,
    Double,
    Void
};

struct Expr;
struct Stmt;
struct BlockStmt;
struct Function;
struct Program;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct Expr {
    virtual ~Expr() = default;
};

struct IntegerLiteralExpr : Expr {
    long long value;

    explicit IntegerLiteralExpr(long long value) : value(value) {}
};

struct FloatingLiteralExpr : Expr {
    double value;

    explicit FloatingLiteralExpr(double value) : value(value) {}
};

struct VariableExpr : Expr {
    std::string name;

    explicit VariableExpr(std::string name) : name(std::move(name)) {}
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr lhs;
    ExprPtr rhs;

    BinaryExpr(std::string op, ExprPtr lhs, ExprPtr rhs) : op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
};

struct CallExpr : Expr {
    std::string caller_name;
    std::vector<ExprPtr> args;

    CallExpr(std::string caller_name, std::vector<ExprPtr> args) : caller_name(std::move(caller_name)), args(std::move(args)) {}
};

struct Stmt {
    virtual ~Stmt() = default;
};

struct VarDeclStmt : Stmt{
    TypeKind type;
    std::string name;
    ExprPtr init;

    VarDeclStmt(TypeKind type, std::string name, ExprPtr init) : type(type), name(std::move(name)), init(std::move(init)) {}
};

struct AssignStmt : Stmt {
    std::string name;
    ExprPtr value;

    AssignStmt(std::string name, ExprPtr value): name(std::move(name)), value(std::move(value)) {}
};

struct ReturnStmt : Stmt {
    ExprPtr value;

    explicit ReturnStmt(ExprPtr value) : value(std::move(value)) {}
};

struct ExprStmt : Stmt {
    ExprPtr expr;

    explicit ExprStmt (ExprPtr expr): expr(std::move(expr)) {}
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
};

struct IfStmt : Stmt {
    ExprPtr cond;
    std::unique_ptr<BlockStmt> thenBlock;
    std::unique_ptr<BlockStmt> elseBlock;

    IfStmt(ExprPtr cond, std::unique_ptr<BlockStmt> thenBlock, std::unique_ptr<BlockStmt> elseBlock) :
        cond(std::move(cond)), thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {}
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    std::unique_ptr<BlockStmt> body;

    WhileStmt(ExprPtr cond, std::unique_ptr<BlockStmt> body) : cond(std::move(cond)), body(std::move(body)) {}
};

// Function Param i.e. int x
struct Param {
    TypeKind type;
    std::string name;
    
    Param(TypeKind type, std::string name) : type(type), name(std::move(name)) {}
};

struct Function {
    TypeKind returnType;
    std::string name;
    std::vector<Param> params;
    std::unique_ptr<BlockStmt> body;

    Function(TypeKind returnType, std::string name, std::vector<Param> params, std::unique_ptr<BlockStmt> body) :
        returnType(returnType), name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
};

struct Program {
    std::vector<std::unique_ptr<Function>> functions;
};