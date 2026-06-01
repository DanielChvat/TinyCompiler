#include "Lexer.h"
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <utility>

Lexer::Lexer(std::string source): src(std::move(source)){}

char Lexer::peek(size_t offset) const {
    size_t index = pos + offset;
    
    if (index >= src.size())
        return '\0';

    return src[index];
}

char Lexer::get(){
    if (pos >= src.size())
        return '\0';

    return src[pos++];
}

void Lexer::skipIgnored() {
    while (true){
        while (std::isspace(static_cast<unsigned char>(peek()))){
            get();
        }
        
        if (peek() == '/' && peek(1) == '/') {
            while (peek() != '\n' && peek() != '\0'){
                get();
            }
            continue;
        }
        break;
    }
}

bool Lexer::startsFloatingLiteral() const {
    if (std::isdigit(static_cast<unsigned char>(peek()))){
        size_t i = 0;

        while (std::isdigit(static_cast<unsigned char>(peek(i)))){
            ++i;
        }
        char following = peek(i);
        if (following == '.' || following == 'e' || following == 'E') return true;

        return false;
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) return true;

    return false;
}

token_t Lexer::next(){
    skipIgnored();

    char c = peek();

    if (c == '\0') return {TokenType::END, "", std::monostate{}};

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_'){
        std::string source_text;

        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_'){
            source_text.push_back(get());
        }

        if (source_text == "short") return {TokenType::SHORT, source_text, std::monostate{}};
        else if (source_text == "int") return {TokenType::INT, source_text, std::monostate{}};
        else if (source_text == "float") return {TokenType::FLOAT, source_text, std::monostate{}};
        else if (source_text == "double") return {TokenType::DOUBLE, source_text, std::monostate{}};
        else if (source_text == "void") return {TokenType::VOID, source_text, std::monostate{}};
        else if (source_text == "return") return {TokenType::RETURN, source_text, std::monostate{}};
        else if (source_text == "if") return {TokenType::IF, source_text, std::monostate{}};
        else if (source_text == "else") return {TokenType::ELSE, source_text, std::monostate{}};
        else if (source_text == "while") return {TokenType::WHILE, source_text, std::monostate{}};
        else return {TokenType::Identifier, source_text, std::monostate{}};
    }

    if (startsFloatingLiteral()){
        std::string source_text;

        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            source_text.push_back(get());
        }

        if (peek() == '.') {
            source_text.push_back(get());

            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                source_text.push_back(get());
            }
        }

        if (peek() == 'e' || peek() == 'E') {
            source_text.push_back(get());

            // Exponent may have explicit sign.
            if (peek() == '+' || peek() == '-') {
                source_text.push_back(get());
            }

            // Require at least one digit after e/E or e+/e-.
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                throw std::runtime_error("Malformed floating literal: " + source_text);
            }

            // Consume exponent digits.
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                source_text.push_back(get());
            }
        }

        double value = std::strtod(source_text.c_str(), nullptr);

        return {TokenType::FloatingLiteral, source_text, value};
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        std::string source_text;

        // Consume all digits.
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            source_text.push_back(get());
        }

        // Convert spelling to long long.
        long long value = std::strtoll(source_text.c_str(), nullptr, 10);

        // Return integer literal token.
        return {TokenType::IntegerLiteral, source_text, value};
    }

    switch (get()) {
        case '(':
            return {TokenType::LPAREN, "(", std::monostate{}};
        case ')':
            return {TokenType::RPAREN, ")", std::monostate{}};
        case '{':
            return {TokenType::LBRACE, "{", std::monostate{}};
        case '}':
            return {TokenType::RBRACE, "}", std::monostate{}};
        case ';':
            return {TokenType::SEMICOLON, ";", std::monostate{}};
        case ',':
            return {TokenType::COMMA, ",", std::monostate{}};
        case '+':
            return {TokenType::PLUS, "+", std::monostate{}};
        case '-':
            return {TokenType::MINUS, "-", std::monostate{}};
        case '*':
            return {TokenType::STAR, "*", std::monostate{}};
        case '/':
            return {TokenType::SLASH, "/", std::monostate{}};
        case '=':
            if (peek() == '=') {
                get();
                return {TokenType::EQUALS, "==", std::monostate{}};
            }
            return {TokenType::ASSIGNMENT, "=", std::monostate{}};

        case '!':
            // If next character is =, consume it and return !=.
            if (peek() == '=') {
                get();
                return {TokenType::NOTEQUAL, "!=", std::monostate{}};
            }

            // Plain ! falls through to error.
            break;

        case '<':
            if (peek() == '=') {
                get();
                return {TokenType::LESSEQUAL, "<=", std::monostate{}};
            }

            return {TokenType::LESS, "<", std::monostate{}};

        case '>':
            if (peek() == '=') {
                get();
                return {TokenType::GREATEREQUAL, ">=", std::monostate{}};
            }

            return {TokenType::GREATER, ">", std::monostate{}};
    }

    throw std::runtime_error(std::string("Unexpected character: ") + c);
}