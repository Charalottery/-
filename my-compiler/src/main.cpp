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
    // ===== Compile pipeline switches (managed in main.cpp) =====
    // Stop after this stage.
    // - Lexer  -> lexer.txt
    // - Parser -> lexer.txt + parser.txt
    // - Symbol -> lexer.txt + parser.txt + symbol.txt
    // - Llvm   -> lexer.txt + parser.txt + symbol.txt + llvm_ir.txt
    // - Mips   -> lexer.txt + parser.txt + symbol.txt + llvm_ir.txt + mips.txt
    enum class CompileStage : int
    {
        Lexer = 0,
        Parser = 1,
        Symbol = 2,
        Llvm = 3,
        Mips = 4,
    };

    const CompileStage stopAfter = CompileStage::Mips;

    // ===== Optimization switches (managed in main.cpp) =====
    // Only relevant when stopAfter == Mips.
    const bool enableOpt = true;     // master switch
    const bool enableMem2Reg = true; // per-pass switch

    (void)argc;
    (void)argv;

    auto stageAtLeast = [&](CompileStage s) -> bool
    {
        return static_cast<int>(stopAfter) >= static_cast<int>(s);
    };

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

    auto dumpErrorOnlyAndExit = [&]() -> int
    {
        ErrorRecorder::DumpErrors("error.txt");
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
    };

    // Stage checkpoint: Lexer
    if (stopAfter == CompileStage::Lexer)
    {
        if (ErrorRecorder::HasErrors())
        {
            // only lexical-stage errors are available here
            return dumpErrorOnlyAndExit();
        }

        std::ofstream lf("lexer.txt");
        for (const auto &t : lexer.GetTokenList())
        {
            if (t.type == TokenType::EOF_T)
                continue;
            lf << t << "\n";
        }
        std::remove("error.txt");
        return 0;
    }

    // build token stream and parse
    TokenStream ts(lexer.GetTokenList());
    Parser parser(ts);
    auto tree = parser.ParseCompUnit();

    // Stage checkpoint: Parser
    if (stopAfter == CompileStage::Parser)
    {
        // At parser stage, output *all* errors detected so far (lexer + parser).
        if (ErrorRecorder::HasErrors())
        {
            return dumpErrorOnlyAndExit();
        }

        {
            std::ofstream lf("lexer.txt");
            for (const auto &t : lexer.GetTokenList())
            {
                if (t.type == TokenType::EOF_T)
                    continue;
                lf << t << "\n";
            }
        }
        if (tree)
        {
            std::ofstream pf("parser.txt");
            tree->PostOrderPrint(pf);
        }
        std::remove("error.txt");
        return 0;
    }

    // Stage checkpoint: Symbol
    // Match download behavior: still run semantic analysis even if lexer/parser already recorded errors.
    if (stopAfter == CompileStage::Symbol)
    {
        midend::SemanticAnalyzer::SetDumpSymbols(true);
        if (tree)
            midend::SemanticAnalyzer::Analyze(tree.get());

        if (ErrorRecorder::HasErrors())
        {
            return dumpErrorOnlyAndExit();
        }

        // No errors: emit lexer.txt + parser.txt; SemanticAnalyzer already emitted symbol.txt.
        {
            std::ofstream lf("lexer.txt");
            for (const auto &t : lexer.GetTokenList())
            {
                if (t.type == TokenType::EOF_T)
                    continue;
                lf << t << "\n";
            }
        }
        if (tree)
        {
            std::ofstream pf("parser.txt");
            tree->PostOrderPrint(pf);
        }

        std::remove("error.txt");
        return 0;
    }

    // For later stages, do not continue if lexer/parser already reported errors.
    if (ErrorRecorder::HasErrors())
    {
        return dumpErrorOnlyAndExit();
    }

    // run semantic analysis (build symbol table) and optionally dump symbol.txt
    midend::SemanticAnalyzer::SetDumpSymbols(stageAtLeast(CompileStage::Symbol));
    if (tree)
        midend::SemanticAnalyzer::Analyze(tree.get());

    // After semantic analysis, if errors were recorded write only error.txt
    if (ErrorRecorder::HasErrors())
    {
        return dumpErrorOnlyAndExit();
    }

    // no errors so far: emit lexer and parser outputs
    // (symbol.txt is emitted by SemanticAnalyzer when enabled)
    {
        std::ofstream lf("lexer.txt");
        for (const auto &t : lexer.GetTokenList())
        {
            if (t.type == TokenType::EOF_T)
                continue;
            lf << t << "\n";
        }
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

        if (stopAfter == CompileStage::Llvm)
        {
            std::ofstream llvmFile("llvm_ir.txt");
            generator.module->print(llvmFile);
            std::remove("error.txt");
            return 0;
        }

        // stopAfter == Mips

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
