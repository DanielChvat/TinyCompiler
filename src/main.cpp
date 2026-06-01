#include "CodeGen.h"
#include "Lexer.h"
#include "Parser.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

static std::string readFile(const std::string& path) {
    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("Could not open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: toycc <file.toy>\n";
            return 1;
        }

        std::string source = readFile(argv[1]);

        Lexer lexer(source);
        Parser parser(std::move(lexer));

        Program program = parser.parseProgram();

        CodeGen codegen;
        codegen.generate(program);
        codegen.printIR();

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
}