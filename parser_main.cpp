#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include "lexer.c++"
#include "ast_parser.hpp"

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <file.brik>\n";
            std::cerr << "Example: " << argv[0] << " tetris.brik\n";
            return 1;
        }

        const char* path = argv[1];
        std::ifstream f(path, std::ios::in | std::ios::binary);
        if (!f) {
            std::cerr << "Could not open: " << path << "\n";
            return 1;
        }

        std::stringstream buf; buf << f.rdbuf();
        std::string source = buf.str();

        Lexer lex(source);
        std::vector<Token> raw = lex.tokenize();
        std::vector<PToken> pt = make_ptokens(raw);

        Parser parser(std::move(pt));
        std::unique_ptr<Program> ast = parser.parseProgram();

        dumpAST(ast.get(), std::cout);

        std::cerr << "\n[Syntax OK] " << path << "\n";
        return 0;

    } catch (const ParseError& e) {
        std::cerr << "[Syntax error] " << e.what()
                  << "  at line " << e.line << ", col " << e.col << "\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "[Exception] " << e.what() << "\n";
        return 3;
    }
}
