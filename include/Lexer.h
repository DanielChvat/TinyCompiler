#pragma once

#include "Tokens.h"
#include <string>

class Lexer {
public:
    explicit Lexer(std::string source);
    token_t next();
private:
    std::string src;
    size_t pos = 0;
    char peek(size_t offset = 0) const;
    char get();
    bool startsFloatingLiteral() const;
    void skipIgnored();  
};