#include "frontend/lexer/Lexer.hpp"
#include "frontend/lexer/TokenType.hpp"
#include "frontend/lexer/TokenPrinter.hpp"
#include "error/ErrorRecorder.hpp"
#include "error/ErrorType.hpp"
#include "frontend/parser/Parser.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdio>

int main() {
    // read source from testfile.txt in current directory
    const char *infile = "testfile.txt";
    std::ifstream fin(infile);
    if (!fin) {
        std::cerr << "Cannot open input file: " << infile << "\n";
        return 1;
    }
    std::string input((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());

    // remove UTF-8 BOM if present (three bytes 0xEF 0xBB 0xBF)
    if (input.size() >= 3 && static_cast<unsigned char>(input[0]) == 0xEF
        && static_cast<unsigned char>(input[1]) == 0xBB
        && static_cast<unsigned char>(input[2]) == 0xBF) {
        input.erase(0, 3);
    }

    Lexer lexer(input);
    // Let the lexer produce the full token list itself.
    lexer.GenerateTokenList();

    // if errors exist, write error.txt sorted by line number
    if (ErrorRecorder::HasErrors()) {
        // remove stale lexer output if exists
        std::remove("lexer.txt");

        auto errors = ErrorRecorder::GetErrors();
        std::sort(errors.begin(), errors.end(), [](const Error &a, const Error &b){ return a.line < b.line; });
        std::ofstream ef("error.txt");
        for (const auto &e : errors) {
        // write: ÐÐºÅ ´íÎóÀà±ðÂë
        // Per spec: illegal symbol -> category code 'a'
        std::string code = (e.type == ErrorType::ILLEGAL_SYMBOL) ? std::string("a") : std::string("?");
        ef << e.line << " " << code << "\n";
        }
        return 0;
    }

    // otherwise write lexer.txt with tokens in reading order (exclude EOF)
    // remove stale error file if exists
    std::remove("error.txt");
    // build token stream and parse
    TokenStream ts(lexer.GetTokenList());
    Parser parser(ts);
    auto tree = parser.ParseCompUnit();

    // write parser output as post-order traversal of AST
    std::remove("parser.txt");
    std::ofstream pf("parser.txt");
    if (tree) tree->PostOrderPrint(pf);

    return 0;
}
