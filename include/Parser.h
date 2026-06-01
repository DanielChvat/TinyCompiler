#pragma once
#include "AST.h"
#include "Lexer.h"

class Parser {
public:
    explicit Parser(Lexer lexer);

    Program parseProgram();
private:
    Lexer lexer;
    token_t currentToken;
    token_t nextToken;
    bool hasLookAhead = false;
    void advance();
    token_t peekToken();
    void expect(TokenType type, const char *message);
    bool isTypeToken(TokenType type) const;
    bool isValueTypeToken(TokenType type) const;
    TypeKind parseReturnType();
    TypeKind parseValueType();
    std::unique_ptr<Function> parseFunction();
    std::vector<Param> parseParams();
    std::unique_ptr<BlockStmt> parseBlock();
    StmtPtr parseStatement();
    ExprPtr parseExpression(int minPrec = 0);
    ExprPtr parsePrimary();
    int precedence(TokenType type) const;
    std::string opText(TokenType type) const;
};