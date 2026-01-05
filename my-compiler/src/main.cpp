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
#include "optimize/PassManager.hpp"
#include "optimize/Mem2Reg.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <string>

int main(int argc, char **argv)
{
    // Optimization switches (managed in main.cpp)
    // Master switch
    bool enableOpt = true;

    // Per-pass switches
    bool enableMem2Reg = true;

    // Optional CLI flags for local debugging (defaults keep judge behavior unchanged)
    //   --no-opt       : disable all optimizations
    //   --no-mem2reg   : disable mem2reg pass (requires enableOpt=true)
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i] ? argv[i] : "";
        if (arg == "--no-opt")
        {
            enableOpt = false;
        }
        else if (arg == "--no-mem2reg")
        {
            enableMem2Reg = false;
        }
    }

    // read source from testfile.txt in current directory
    const char *infile = "testfile.txt";
    std::ifstream fin(infile);
    if (!fin)
    {
        std::cerr << "Cannot open input file: " << infile << "\n";
        return 1;
    }
    std::string input((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());

    // remove UTF-8 BOM if present (three bytes 0xEF 0xBB 0xBF)
    if (input.size() >= 3 && static_cast<unsigned char>(input[0]) == 0xEF && static_cast<unsigned char>(input[1]) == 0xBB && static_cast<unsigned char>(input[2]) == 0xBF)
    {
        input.erase(0, 3);
    }

    // remove any previous outputs before running analysis
    std::remove("lexer.txt");
    std::remove("parser.txt");
    std::remove("symbol.txt");
    std::remove("error.txt");
    std::remove("llvm_ir.txt");
    std::remove("llvm_ir_before.txt");
    std::remove("llvm_ir_after.txt");
    std::remove("mips.txt");
    std::remove("mips_before.txt");
    std::remove("mips_after.txt");

    Lexer lexer(input);
    // Let the lexer produce the full token list itself.
    lexer.GenerateTokenList();

    // build token stream and parse
    TokenStream ts(lexer.GetTokenList());
    Parser parser(ts);
    auto tree = parser.ParseCompUnit();

    // run semantic analysis (build symbol table) and optionally dump symbol.txt
    midend::SemanticAnalyzer::SetDumpSymbols(true);
    if (tree)
        midend::SemanticAnalyzer::Analyze(tree.get());

    // After semantic analysis, if semantic errors were recorded write them to error.txt
    if (ErrorRecorder::HasErrors())
    {
        // on error: only emit error.txt
        ErrorRecorder::DumpErrors("error.txt");
        // ensure other outputs are not present
        std::remove("lexer.txt");
        std::remove("parser.txt");
        std::remove("symbol.txt");
        std::remove("llvm_ir.txt");
        std::remove("llvm_ir_before.txt");
        std::remove("llvm_ir_after.txt");
        std::remove("mips.txt");
        std::remove("mips_before.txt");
        std::remove("mips_after.txt");
        return 0;
    }

    // no errors: emit lexer and parser outputs and keep symbol.txt (if produced)
    {
        std::ofstream lf("lexer.txt");
        for (const auto &t : lexer.GetTokenList())
            lf << t << "\n";
    }
    if (tree)
    {
        std::ofstream pf("parser.txt");
        tree->PostOrderPrint(pf);
    }

    // Generate LLVM IR
    if (tree)
    {
        SymbolTable *rootTable = SymbolManager::GetRoot();
        IRGenerator generator(tree.get(), rootTable);
        generator.generate();

        if (enableOpt)
        {
            // Dump unoptimized LLVM + MIPS
            {
                std::ofstream llvmBefore("llvm_ir_before.txt");
                generator.module->print(llvmBefore);
            }
            {
                std::ofstream mipsBefore("mips_before.txt");
                MipsGenerator mipsGenBefore(generator.module, mipsBefore);
                mipsGenBefore.generate();
            }

            // Run optimization pipeline (extendable)
            optimize::PassManager pm;
            if (enableMem2Reg)
            {
                pm.addPass(std::make_unique<optimize::Mem2RegPass>());
            }
            pm.run(generator.module);

            // Dump optimized LLVM
            {
                std::ofstream llvmAfter("llvm_ir_after.txt");
                generator.module->print(llvmAfter);
            }
            {
                std::ofstream llvmFile("llvm_ir.txt");
                generator.module->print(llvmFile);
            }

            // Dump optimized MIPS
            {
                std::ofstream mipsAfter("mips_after.txt");
                MipsGenerator mipsGenAfter(generator.module, mipsAfter);
                mipsGenAfter.generate();
            }
            {
                std::ofstream mipsFile("mips.txt");
                MipsGenerator mipsGen(generator.module, mipsFile);
                mipsGen.generate();
            }
        }
        else
        {
            // Original behavior: emit only llvm_ir.txt and mips.txt
            std::ofstream llvmFile("llvm_ir.txt");
            generator.module->print(llvmFile);

            std::ofstream mipsFile("mips.txt");
            MipsGenerator mipsGen(generator.module, mipsFile);
            mipsGen.generate();
        }
    }

    std::remove("error.txt");

    return 0;
}
