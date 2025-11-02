#include "FrontendDriver.hpp"
#include "lexer/Lexer.hpp"
#include "parser/Parser.hpp"
#include "parser/ParserOutput.hpp"
#include "../error/ErrorRecorder.hpp"
#include "../error/ErrorType.hpp"
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <unordered_set>

int FrontendDriver::Process(const std::string &input, const std::string &tokensOutPath, const std::string &errorOutPath) {
    // 如果存在之前的错误输出文件，先删除，保证后续写入的是本次运行的内容
    std::remove(errorOutPath.c_str());

    // 1) 词法分析阶段：构建 Lexer 并不断产生 token，直到遇到 EOF
    Lexer lexer(input);
    // 产生 token 直到 EOF
    while (true) {
        lexer.next();
        TokenType tt = lexer.getTokenType();
        if (tt == TokenType::EOF_T) break;
        // 保护性检查：避免在 lexer 出现异常情况下无限循环
        if (lexer.GetTokenList().size() > 1000000) break;
    }

    // 如果在词法分析阶段记录了错误，按行号排序并写入 errorOutPath，然后返回错误状态 2
    if (ErrorRecorder::HasErrors()) {
        auto errors = ErrorRecorder::GetErrors();
        std::sort(errors.begin(), errors.end(), [](const Error &a, const Error &b){ return a.line < b.line; });
        std::ofstream ef(errorOutPath);
        for (const auto &e : errors) {
            std::string code;
            if (e.type == ErrorType::ILLEGAL_SYMBOL) code = "a";
            else if (e.type == ErrorType::MISSING_SEMICOLON) code = "i";
            else if (e.type == ErrorType::MISSING_RPAREN) code = "j";
            else if (e.type == ErrorType::MISSING_RBRACK) code = "k";
            else code = "?";
            ef << e.line << " " << code << "\n";
        }
        return 2;
    }

    // 2) 语法分析阶段：将 token 列表传入 Parser，生成 AST
    TokenStream ts(lexer.GetTokenList());
    Parser parser;
    parser.SetTokenStream(std::move(ts));
    // ParserOutput 可能用于记录或输出解析过程中需要的信息，调用以确保单例/静态初始化
    ParserOutput::Get();
    parser.GenerateAstTree();

    // 如果语法分析阶段记录了错误，同样写入 errorOutPath 并返回 2
    if (ErrorRecorder::HasErrors()) {
        auto errors2 = ErrorRecorder::GetErrors();
        std::sort(errors2.begin(), errors2.end(), [](const Error &a, const Error &b){ return a.line < b.line; });
        std::ofstream ef2(errorOutPath);
        for (const auto &e : errors2) {
            std::string code;
            if (e.type == ErrorType::ILLEGAL_SYMBOL) code = "a";
            else if (e.type == ErrorType::MISSING_SEMICOLON) code = "i";
            else if (e.type == ErrorType::MISSING_RPAREN) code = "j";
            else if (e.type == ErrorType::MISSING_RBRACK) code = "k";
            else code = "?";
            ef2 << e.line << " " << code << "\n";
        }
        return 2;
    }

    // ParserOutput already wrote AST-based parser.txt; do not overwrite it here.
    // Return success (Compiler/driver expects 0 on success unless IO error)
    return 0;
}
