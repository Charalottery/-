#include "frontend/lexer/Lexer.hpp"
#include "frontend/lexer/TokenType.hpp"
#include "frontend/lexer/TokenPrinter.hpp"
#include "error/ErrorRecorder.hpp"
#include "error/ErrorType.hpp"
#include "frontend/parser/Parser.hpp"
#include "midend/SemanticAnalyzer.hpp"
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
    // continue to semantic analysis even if parsing recorded errors

    // remove any previous outputs before running analysis
    std::remove("error.txt");
    std::remove("symbol.txt");
    // run semantic analysis (build symbol table) and optionally dump symbol.txt
    midend::SemanticAnalyzer::SetDumpSymbols(true);
    if (tree) midend::SemanticAnalyzer::Analyze(tree.get());

    // After semantic analysis, if semantic errors were recorded write them to error.txt
    if (ErrorRecorder::HasErrors()) {
        // preserve detection order, keep only the first error per line
        auto all = ErrorRecorder::GetErrors();
        std::vector<Error> picked;
        std::unordered_set<int> seenLines;
        for (const auto &e : all) {
            if (seenLines.find(e.line) == seenLines.end()) {
                picked.push_back(e);
                seenLines.insert(e.line);
            }
        }
        // sort by line number ascending for output
        std::sort(picked.begin(), picked.end(), [](const Error &a, const Error &b){ return a.line < b.line; });
        std::ofstream ef("error.txt");
        for (const auto &e : picked) {
            std::string code;
            switch (e.type) {
                case ErrorType::ILLEGAL_SYMBOL: code = "a"; break;
                case ErrorType::MISS_SEMICN: code = "i"; break;
                case ErrorType::MISS_RPARENT: code = "j"; break;
                case ErrorType::MISS_RBRACK: code = "k"; break;
                case ErrorType::NAME_REDEFINE: code = "b"; break;
                case ErrorType::NAME_UNDEFINED: code = "c"; break;
                case ErrorType::FUNC_PARAM_COUNT_MISMATCH: code = "d"; break;
                case ErrorType::FUNC_PARAM_TYPE_MISMATCH: code = "e"; break;
                case ErrorType::RETURN_IN_VOID: code = "f"; break;
                case ErrorType::MISSING_RETURN: code = "g"; break;
                case ErrorType::ASSIGN_TO_CONST: code = "h"; break;
                case ErrorType::PRINTF_ARG_MISMATCH: code = "l"; break;
                case ErrorType::BAD_BREAK_CONTINUE: code = "m"; break;
                default: code = "?"; break;
            }
            ef << e.line << " " << code << "\n";
        }
        std::remove("parser.txt");
        return 0;
    }

    return 0;
}
