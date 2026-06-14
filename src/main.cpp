#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "value.h"
#include "interpreter.h"

int main(int argc, char* argv[]) {
    std::string source;

    if (argc >= 2) {
        // Read from file
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file '" << argv[1] << "'" << std::endl;
            return 1;
        }
        std::ostringstream oss;
        oss << file.rdbuf();
        source = oss.str();
    } else {
        // Read from stdin
        std::ostringstream oss;
        oss << std::cin.rdbuf();
        source = oss.str();
    }

    if (source.empty()) {
        std::cerr << "Error: No input provided." << std::endl;
        return 1;
    }

    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        auto ast = parser.parse();

        Interpreter interp;
        interp.run(ast);

    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}