#include "frontend/lexer/Lexer.hpp"
#include "frontend/lexer/TokenType.hpp"
#include "error/ErrorRecorder.hpp"
#include "error/ErrorType.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdio>

static std::string TokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::IDENFR: return "IDENFR";
        case TokenType::INTCON: return "INTCON";
        case TokenType::STRCON: return "STRCON";
        case TokenType::MAINTK: return "MAINTK";
        case TokenType::CONSTTK: return "CONSTTK";
        case TokenType::INTTK: return "INTTK";
        case TokenType::STATICTK: return "STATICTK";
        case TokenType::VOIDTK: return "VOIDTK";
        case TokenType::BREAKTK: return "BREAKTK";
        case TokenType::CONTINUETK: return "CONTINUETK";
        case TokenType::IFTK: return "IFTK";
        case TokenType::ELSETK: return "ELSETK";
        case TokenType::FORTK: return "FORTK";
        case TokenType::NOT: return "NOT";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::PRINTFTK: return "PRINTFTK";
        case TokenType::RETURNTK: return "RETURNTK";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINU: return "MINU";
        case TokenType::MULT: return "MULT";
        case TokenType::DIV: return "DIV";
        case TokenType::MOD: return "MOD";
        case TokenType::LSS: return "LSS";
        case TokenType::LEQ: return "LEQ";
        case TokenType::GRE: return "GRE";
        case TokenType::GEQ: return "GEQ";
        case TokenType::EQL: return "EQL";
        case TokenType::NEQ: return "NEQ";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::SEMICN: return "SEMICN";
        case TokenType::COMMA: return "COMMA";
        case TokenType::LPARENT: return "LPARENT";
        case TokenType::RPARENT: return "RPARENT";
        case TokenType::LBRACK: return "LBRACK";
        case TokenType::RBRACK: return "RBRACK";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::EOF_T: return "EOF";
        case TokenType::ERROR_T: return "ERROR";
    }
    return "UNKNOWN";
}

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
    // produce tokens until EOF
    while (true) {
        lexer.next();
        TokenType tt = lexer.getTokenType();
        if (tt == TokenType::EOF_T) break;
        // avoid infinite loops if something goes wrong
        if (lexer.GetTokenList().size() > 1000000) break;
    }

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
    std::ofstream of("lexer.txt");
    for (const auto &t : lexer.GetTokenList()) {
        if (t.type == TokenType::EOF_T) continue;
        of << TokenTypeToString(t.type) << " " << t.value << "\n";
    }

    return 0;
}
