#include "Lexer.h"
#include "Parser.h"
#include "AST.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

static std::string typeToString(TypeKind type) {
    switch (type) {
        case TypeKind::Short:
            return "short";
        case TypeKind::Int:
            return "int";
        case TypeKind::Float:
            return "float";
        case TypeKind::Double:
            return "double";
        case TypeKind::Void:
            return "void";
    }

    return "<unknown type>";
}

static std::string readFile(const std::string& path) {
    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

static void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) {
        std::cout << "  ";
    }
}

static void printExpr(const Expr& expr, int indent);
static void printStmt(const Stmt& stmt, int indent);

static void printExpr(const Expr& expr, int indent) {
    if (auto* e = dynamic_cast<const IntegerLiteralExpr*>(&expr)) {
        printIndent(indent);
        std::cout << "IntegerLiteral(value=" << e->value << ")\n";
        return;
    }

    if (auto* e = dynamic_cast<const FloatingLiteralExpr*>(&expr)) {
        printIndent(indent);
        std::cout << "FloatingLiteral(value=" << e->value << ")\n";
        return;
    }

    if (auto* e = dynamic_cast<const VariableExpr*>(&expr)) {
        printIndent(indent);
        std::cout << "Variable(name=" << e->name << ")\n";
        return;
    }

    if (auto* e = dynamic_cast<const BinaryExpr*>(&expr)) {
        printIndent(indent);
        std::cout << "BinaryExpr(op=\"" << e->op << "\")\n";

        printIndent(indent + 1);
        std::cout << "lhs:\n";
        printExpr(*e->lhs, indent + 2);

        printIndent(indent + 1);
        std::cout << "rhs:\n";
        printExpr(*e->rhs, indent + 2);

        return;
    }

    if (auto* e = dynamic_cast<const CallExpr*>(&expr)) {
        printIndent(indent);
        std::cout << "CallExpr(callee=" << e->caller_name << ")\n";

        printIndent(indent + 1);
        std::cout << "args:\n";

        for (const auto& arg : e->args) {
            printExpr(*arg, indent + 2);
        }

        return;
    }

    printIndent(indent);
    std::cout << "<unknown expr>\n";
}

static void printBlock(const BlockStmt& block, int indent) {
    printIndent(indent);
    std::cout << "Block\n";

    for (const auto& stmt : block.statements) {
        printStmt(*stmt, indent + 1);
    }
}

static void printStmt(const Stmt& stmt, int indent) {
    if (auto* s = dynamic_cast<const VarDeclStmt*>(&stmt)) {
        printIndent(indent);
        std::cout << "VarDecl(type=" << typeToString(s->type)
                  << ", name=" << s->name << ")\n";

        printIndent(indent + 1);
        std::cout << "init:\n";
        printExpr(*s->init, indent + 2);
        return;
    }

    if (auto* s = dynamic_cast<const AssignStmt*>(&stmt)) {
        printIndent(indent);
        std::cout << "Assign(name=" << s->name << ")\n";

        printIndent(indent + 1);
        std::cout << "value:\n";
        printExpr(*s->value, indent + 2);
        return;
    }

    if (auto* s = dynamic_cast<const ReturnStmt*>(&stmt)) {
        printIndent(indent);
        std::cout << "Return\n";

        printIndent(indent + 1);
        std::cout << "value:\n";
        printExpr(*s->value, indent + 2);
        return;
    }

    if (auto* s = dynamic_cast<const ExprStmt*>(&stmt)) {
        printIndent(indent);
        std::cout << "ExprStmt\n";
        printExpr(*s->expr, indent + 1);
        return;
    }

    if (auto* s = dynamic_cast<const IfStmt*>(&stmt)) {
        printIndent(indent);
        std::cout << "IfStmt\n";

        printIndent(indent + 1);
        std::cout << "cond:\n";
        printExpr(*s->cond, indent + 2);

        printIndent(indent + 1);
        std::cout << "then:\n";
        printBlock(*s->thenBlock, indent + 2);

        if (s->elseBlock) {
            printIndent(indent + 1);
            std::cout << "else:\n";
            printBlock(*s->elseBlock, indent + 2);
        }

        return;
    }

    if (auto* s = dynamic_cast<const WhileStmt*>(&stmt)) {
        printIndent(indent);
        std::cout << "WhileStmt\n";

        printIndent(indent + 1);
        std::cout << "cond:\n";
        printExpr(*s->cond, indent + 2);

        printIndent(indent + 1);
        std::cout << "body:\n";
        printBlock(*s->body, indent + 2);

        return;
    }

    printIndent(indent);
    std::cout << "<unknown stmt>\n";
}

static void printFunction(const Function& fn, int indent) {
    printIndent(indent);
    std::cout << "Function(name=" << fn.name
              << ", returnType=" << typeToString(fn.returnType) << ")\n";

    printIndent(indent + 1);
    std::cout << "params:\n";

    for (const Param& param : fn.params) {
        printIndent(indent + 2);
        std::cout << "Param(type=" << typeToString(param.type)
                  << ", name=" << param.name << ")\n";
    }

    printIndent(indent + 1);
    std::cout << "body:\n";

    printBlock(*fn.body, indent + 2);
}

static void printProgram(const Program& program) {
    std::cout << "Program\n";

    for (const auto& fn : program.functions) {
        printFunction(*fn, 1);
    }
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: tinycc <file.c>\n";
            return 1;
        }

        std::string source = readFile(argv[1]);

        Lexer lexer(source);
        Parser parser(std::move(lexer));

        Program program = parser.parseProgram();

        printProgram(program);

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
}