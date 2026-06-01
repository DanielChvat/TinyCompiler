#include "Parser.h"
#include <stdexcept>
#include <utility>
#include <variant>

Parser::Parser(Lexer lexer) : lexer(std::move(lexer)){
    advance();
}

void Parser::advance(){
    if (hasLookAhead) {
        currentToken = nextToken;
        hasLookAhead = false;
        return;
    }

    currentToken = lexer.next();
}

token_t Parser::peekToken(){
    if (!hasLookAhead){
        nextToken = lexer.next();
        hasLookAhead = true;
    }

    return nextToken;
}

void Parser::expect(TokenType type, const char *message){
    if (currentToken.type != type) {
        throw std::runtime_error(std::string(message) + ", got '" + currentToken.source_text + "'");
    }

    advance();
}

bool Parser::isTypeToken(TokenType type) const {
    return type == TokenType::SHORT ||
           type == TokenType::INT ||
           type == TokenType::FLOAT ||
           type == TokenType::DOUBLE ||
           type == TokenType::VOID;
}

bool Parser::isValueTypeToken(TokenType type) const {
    return type == TokenType::SHORT ||
           type == TokenType::INT ||
           type == TokenType::FLOAT ||
           type == TokenType::DOUBLE;
}

TypeKind Parser::parseReturnType() {
    if (currentToken.type == TokenType::VOID) {
        advance();
        return TypeKind::Void;
    }

    return parseValueType();
}

TypeKind Parser::parseValueType() {
    if (currentToken.type == TokenType::SHORT) {
        advance();
        return TypeKind::Short;
    }

    if (currentToken.type == TokenType::INT) {
        advance();
        return TypeKind::Int;
    }

    if (currentToken.type == TokenType::FLOAT) {
        advance();
        return TypeKind::Float;
    }

    if (currentToken.type == TokenType::DOUBLE) {
        advance();
        return TypeKind::Double;
    }

    throw std::runtime_error(
        "Expected value type, got '" + currentToken.source_text + "'"
    );
}

Program Parser::parseProgram() {
    Program program;

    while (currentToken.type != TokenType::END) {
        program.functions.push_back(parseFunction());
    }

    return program;
}

std::unique_ptr<Function> Parser::parseFunction() {
    TypeKind returnType = parseReturnType();

    if (currentToken.type != TokenType::Identifier) {
        throw std::runtime_error("Expected function name");
    }

    std::string name = currentToken.source_text;
    advance();
    expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<Param> params = parseParams();
    expect(TokenType::RPAREN, "Expected ')' after parameter list");
    auto body = parseBlock();
    return std::make_unique<Function>(
        returnType,
        name,
        std::move(params),
        std::move(body)
    );
}

std::vector<Param> Parser::parseParams() {
    std::vector<Param> params;

    if (currentToken.type == TokenType::RPAREN) {
        return params;
    }

    while (true) {
        TypeKind type = parseValueType();

        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected parameter name");
        }

        std::string name = currentToken.source_text;
        advance();
        params.emplace_back(type, name);
        if (currentToken.type == TokenType::COMMA) {
            advance();
            continue;
        }

        break;
    }
    return params;
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    expect(TokenType::LBRACE, "Expected '{'");
    auto block = std::make_unique<BlockStmt>();

    while (currentToken.type != TokenType::RBRACE) {
        if (currentToken.type == TokenType::END) {
            throw std::runtime_error("Unexpected EOF inside block");
        }

        block->statements.push_back(parseStatement());
    }

    expect(TokenType::RBRACE, "Expected '}'");

    return block;
}

StmtPtr Parser::parseStatement() {
    // Variable declaration:
    if (isValueTypeToken(currentToken.type)) {
        TypeKind type = parseValueType();

        if (currentToken.type != TokenType::Identifier) {
            throw std::runtime_error("Expected variable name");
        }

        std::string name = currentToken.source_text;
        advance();

        expect(TokenType::ASSIGNMENT, "Expected '=' in variable declaration");
        auto init = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after declaration");
        return std::make_unique<VarDeclStmt>(
            type,
            name,
            std::move(init)
        );
    }

    // Return statement:
    if (currentToken.type == TokenType::RETURN) {
        advance();
        auto value = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after return");
        return std::make_unique<ReturnStmt>(std::move(value));
    }

    // If statement:
    if (currentToken.type == TokenType::IF) {
        advance();
        expect(TokenType::LPAREN, "Expected '(' after if");
        auto cond = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after if condition");
        auto thenBlock = parseBlock();
        std::unique_ptr<BlockStmt> elseBlock;
        if (currentToken.type == TokenType::ELSE) {
            advance();
            elseBlock = parseBlock();
        }

        return std::make_unique<IfStmt>(
            std::move(cond),
            std::move(thenBlock),
            std::move(elseBlock)
        );
    }

    // While loop:
    if (currentToken.type == TokenType::WHILE) {
        advance();
        expect(TokenType::LPAREN, "Expected '(' after while");
        auto cond = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after while condition");
        auto body = parseBlock();

        return std::make_unique<WhileStmt>(
            std::move(cond),
            std::move(body)
        );
    }

    // Assignment statement:
    if (currentToken.type == TokenType::Identifier && peekToken().type == TokenType::ASSIGNMENT) {
        std::string name = currentToken.source_text;
        advance();
        expect(TokenType::ASSIGNMENT, "Expected '=' in assignment");
        auto value = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after assignment");

        return std::make_unique<AssignStmt>(
            name,
            std::move(value)
        );
    }

    // Otherwise parse expression statement:
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr));
}

int Parser::precedence(TokenType type) const {
    switch (type) {
        // Comparisons have low precedence.
        case TokenType::EQUALS:
        case TokenType::NOTEQUAL:
        case TokenType::LESS:
        case TokenType::LESSEQUAL:
        case TokenType::GREATER:
        case TokenType::GREATEREQUAL:
            return 10;

        // Addition/subtraction have medium precedence.
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 20;

        // Multiplication/division have high precedence.
        case TokenType::STAR:
        case TokenType::SLASH:
            return 30;

        // Non-operators have no precedence.
        default:
            return -1;
    }
}

std::string Parser::opText(TokenType type) const {
    switch (type) {
        case TokenType::PLUS:
            return "+";
        case TokenType::MINUS:
            return "-";
        case TokenType::STAR:
            return "*";
        case TokenType::SLASH:
            return "/";
        case TokenType::EQUALS:
            return "==";
        case TokenType::NOTEQUAL:
            return "!=";
        case TokenType::LESS:
            return "<";
        case TokenType::LESSEQUAL:
            return "<=";
        case TokenType::GREATER:
            return ">";
        case TokenType::GREATEREQUAL:
            return ">=";
        default:
            return "?";
    }
}

ExprPtr Parser::parseExpression(int minPrec) {
    auto lhs = parsePrimary();

    while (true) {
        int prec = precedence(currentToken.type);

        if (prec < minPrec) {
            break;
        }

        std::string op = opText(currentToken.type);
        advance();
        auto rhs = parseExpression(prec + 1);

        lhs = std::make_unique<BinaryExpr>(
            op,
            std::move(lhs),
            std::move(rhs)
        );
    }

    return lhs;
}

ExprPtr Parser::parsePrimary() {
    if (currentToken.type == TokenType::IntegerLiteral) {
        long long value = std::get<long long>(currentToken.val);
        advance();

        return std::make_unique<IntegerLiteralExpr>(value);
    }

    if (currentToken.type == TokenType::FloatingLiteral) {
        double value = std::get<double>(currentToken.val);
        advance();

        return std::make_unique<FloatingLiteralExpr>(value);
    }

    if (currentToken.type == TokenType::Identifier) {
        std::string name = currentToken.source_text;
        advance();

        if (currentToken.type == TokenType::LPAREN) {
            advance();
            std::vector<ExprPtr> args;

            if (currentToken.type != TokenType::RPAREN) {
                while (true) {
                    args.push_back(parseExpression());

                    if (currentToken.type == TokenType::COMMA) {
                        advance();
                        continue;
                    }

                    break;
                }
            }
            expect(TokenType::RPAREN, "Expected ')' after call arguments");

            return std::make_unique<CallExpr>(
                name,
                std::move(args)
            );
        }

        return std::make_unique<VariableExpr>(name);
    }

    if (currentToken.type == TokenType::LPAREN) {
        advance();
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }

    throw std::runtime_error(
        "Expected expression, got '" + currentToken.source_text + "'"
    );
}


