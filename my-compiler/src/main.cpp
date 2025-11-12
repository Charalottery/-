#include "frontend/lexer/Lexer.hpp"
#include "frontend/lexer/TokenType.hpp"
#include "frontend/lexer/TokenPrinter.hpp"
#include "error/ErrorRecorder.hpp"
#include "error/ErrorType.hpp"
#include "frontend/parser/Parser.hpp"
#include "midend/symbol/SemanticAnalyzer.hpp"
#include "error/ErrorUtils.hpp"
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
    // Semantic analysis (build symbol table). We attempt it if an AST exists.
    SemanticAnalyzer analyzer;
    if (tree) analyzer.Analyze(tree.get());

    // After the whole compilation flow (lexing + parsing + semantic), decide outputs:
    // - If any errors were recorded, write only `error.txt` (sorted) and remove
    //   any `symbol.txt`/`parser.txt`.
    // - Otherwise, write only `symbol.txt` representing the filled symbol tables.
    if (ErrorRecorder::HasErrors()) {
        auto errors = ErrorRecorder::GetErrors();
        // no debug prints here; write errors only to error.txt as intended
        std::sort(errors.begin(), errors.end(), [](const Error &a, const Error &b){ return a.line < b.line; });
        std::ofstream ef("error.txt");
        for (const auto &e : errors) {
            std::string code = ErrorTypeToCode(e.type);
            ef << e.line << " " << code << "\n";
        }
    // ensure only error.txt remains; remove any previous symbol output
    std::remove("symbol.txt");
        return 0;
    }

    // No errors: emit symbol.txt built from analyzer tables. Remove any previous error output.
    std::remove("error.txt");
    std::ofstream of("symbol.txt");
    const auto &tables = analyzer.GetTables();
    for (const auto &tbl : tables) {
        for (const auto &sym : tbl.symbols) {
            std::string tname;
            if (sym.kind == Symbol::Kind::FUNC) {
                tname = (sym.retype == 0) ? "VoidFunc" : "IntFunc";
            } else if (sym.kind == Symbol::Kind::ARRAY) {
                if (sym.isConst) tname = "ConstIntArray";
                else if (sym.isStatic) tname = "StaticIntArray";
                else tname = "IntArray";
            } else { // VAR
                if (sym.isConst) tname = "ConstInt";
                else if (sym.isStatic) tname = "StaticInt";
                else tname = "Int";
            }
            of << tbl.id << " " << sym.token << " " << tname << "\n";
        }
    }

    return 0;
}
