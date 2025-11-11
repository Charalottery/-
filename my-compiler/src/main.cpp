#include "frontend/lexer/Lexer.hpp"
#include "frontend/lexer/TokenType.hpp"
#include "frontend/lexer/TokenPrinter.hpp"
#include "error/ErrorRecorder.hpp"
#include "error/ErrorType.hpp"
#include "frontend/parser/Parser.hpp"
#include "midend/symbol/SemanticAnalyzer.hpp"
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

    // build token stream and parse
    TokenStream ts(lexer.GetTokenList());
    Parser parser(ts);
    auto tree = parser.ParseCompUnit();
    // After the whole compilation flow (lexing + parsing), if any errors were recorded
    // write them to error.txt sorted by line number. Otherwise write parser output.
    if (ErrorRecorder::HasErrors()) {
        auto errors = ErrorRecorder::GetErrors();
        std::sort(errors.begin(), errors.end(), [](const Error &a, const Error &b){ return a.line < b.line; });
        std::ofstream ef("error.txt");
        for (const auto &e : errors) {
            // write: ÐÐºÅ ´íÎóÀà±ðÂë
            // Map ErrorType -> spec code: illegal symbol -> 'a', missing semicolon -> 'i', missing ')' -> 'j', missing ']' -> 'k'
            std::string code;
            switch (e.type) {
                case ErrorType::ILLEGAL_SYMBOL: code = "a"; break;
                case ErrorType::MISS_SEMICN: code = "i"; break;
                case ErrorType::MISS_RPARENT: code = "j"; break;
                case ErrorType::MISS_RBRACK: code = "k"; break;
                default: code = "?"; break;
            }
            ef << e.line << " " << code << "\n";
        }
        // ensure no stale parser output remains
        std::remove("parser.txt");
        return 0;
    }

    // semantic analysis (build symbol table) and then write parser output as post-order traversal of AST (no errors present)
    SemanticAnalyzer analyzer;
    analyzer.Analyze(tree.get());
    std::remove("error.txt");
    std::remove("parser.txt");
    std::ofstream pf("parser.txt");
    if (tree) tree->PostOrderPrint(pf);

    return 0;
}
