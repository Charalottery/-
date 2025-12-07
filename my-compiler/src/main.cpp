#include "frontend/lexer/Lexer.hpp"
#include "frontend/lexer/TokenType.hpp"
#include "frontend/lexer/TokenPrinter.hpp"
#include "error/ErrorRecorder.hpp"
#include "error/ErrorType.hpp"
#include "frontend/parser/Parser.hpp"
#include "midend/analysis/SemanticAnalyzer.hpp"
#include "midend/symbol/SymbolManager.hpp"
#include "midend/irgen/IRGenerator.hpp"
#include "backend/MipsGenerator.hpp"
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

    // remove any previous outputs before running analysis
    std::remove("lexer.txt");
    std::remove("parser.txt");
    std::remove("symbol.txt");
    std::remove("error.txt");
    std::remove("llvm_ir.txt");
    std::remove("mips.txt");

    Lexer lexer(input);
    // Let the lexer produce the full token list itself.
    lexer.GenerateTokenList();

    // build token stream and parse
    TokenStream ts(lexer.GetTokenList());
    Parser parser(ts);
    auto tree = parser.ParseCompUnit();

    // run semantic analysis (build symbol table) and optionally dump symbol.txt
    midend::SemanticAnalyzer::SetDumpSymbols(true);
    if (tree) midend::SemanticAnalyzer::Analyze(tree.get());

    // After semantic analysis, if semantic errors were recorded write them to error.txt
    if (ErrorRecorder::HasErrors()) {
        // on error: only emit error.txt
        ErrorRecorder::DumpErrors("error.txt");
        // ensure other outputs are not present
        std::remove("lexer.txt");
        std::remove("parser.txt");
        std::remove("symbol.txt");
        std::remove("llvm_ir.txt");
        std::remove("mips.txt");
        return 0;
    }

    // no errors: emit lexer and parser outputs and keep symbol.txt (if produced)
    {
        std::ofstream lf("lexer.txt");
        for (const auto &t : lexer.GetTokenList()) lf << t << "\n";
    }
    if (tree) {
        std::ofstream pf("parser.txt");
        tree->PostOrderPrint(pf);
    }

    // Generate LLVM IR
    if (tree) {
        SymbolTable* rootTable = SymbolManager::GetRoot();
        IRGenerator generator(tree.get(), rootTable);
        generator.generate();
        std::ofstream llvmFile("llvm_ir.txt");
        generator.module->print(llvmFile);

        // Generate MIPS
        std::ofstream mipsFile("mips.txt");
        MipsGenerator mipsGen(generator.module, mipsFile);
        mipsGen.generate();
    }

    std::remove("error.txt");

    return 0;
}
