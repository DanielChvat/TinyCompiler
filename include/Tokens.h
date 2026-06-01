#pragma once

#include <string>
#include <variant>

enum class TokenType {
    END,                // END of File/Input
    INT,                // Keyword: int
    SHORT,              // Keyword: short
    FLOAT,              // Keyword: float
    DOUBLE,             // Keyword: double
    VOID,               // Keyword: void
    RETURN,             // Keyword: return
    IF,                 // Keyword: if
    ELSE,               // Keyword: else
    WHILE,              // Keyword: while
    Identifier,         // User defined
    IntegerLiteral,     // I64 Literal Val
    FloatingLiteral,    // F64 Literal Val
    LPAREN,             // (
    RPAREN,             // )
    LBRACE,             // {
    RBRACE,             // }
    SEMICOLON,          // ;
    COMMA,              // ,
    PLUS,               // +
    MINUS,              // -
    STAR,               // *
    SLASH,              // /
    ASSIGNMENT,         // =
    EQUALS,             // ==
    NOTEQUAL,           // !=
    LESS,               // <
    LESSEQUAL,          // <=
    GREATER,            // >
    GREATEREQUAL,       // >=
};

using TokenValue_T = std::variant<
    std::monostate,
    long long,      // I64 Literal
    double,         // F64 Literal
    std::string     // Identifier Name
>;

struct token_t {
    TokenType type;
    std::string source_text;
    TokenValue_T val; 
};