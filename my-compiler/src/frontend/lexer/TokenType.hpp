#pragma once
#include <string>

// TokenType.hpp
// 本文件定义词法分析器产生的 token 类型枚举及若干辅助函数。
// 这些 token 类型在项目中用于统一表示识别到的词素（标识符、关键字、运算符等），
// 供 parser 和输出格式（例如 parser.txt）使用。
enum class TokenType {
    IDENFR,
    INTCON,
    STRCON,

    MAINTK,
    CONSTTK,
    INTTK,
    STATICTK,
    VOIDTK,
    BREAKTK,
    CONTINUETK,
    IFTK,
    ELSETK,
    FORTK,
    PRINTFTK,
    RETURNTK,

    PLUS,
    MINU,
    MULT,
    DIV,
    MOD,

    LSS,
    LEQ,
    GRE,
    GEQ,
    EQL,
    NEQ,
    ASSIGN,

    SEMICN,
    COMMA,

    LPARENT,
    RPARENT,
    LBRACK,
    RBRACK,
    LBRACE,
    RBRACE,

    NOT,
    AND,
    OR,

    EOF_T,
    ERROR_T
};

inline TokenType GetTokenType(const std::string &identifier) {
    // 辅助函数：根据字符串返回对应的 TokenType，处理多字符运算符和关键字
    // 注意：该函数假定传入的是完整的词素（identifier 或符号序列）
    // 多字符运算符优先判断
    if (identifier == "<=") return TokenType::LEQ;
    if (identifier == ">=") return TokenType::GEQ;
    if (identifier == "==") return TokenType::EQL;
    if (identifier == "!=") return TokenType::NEQ;
    if (identifier == "&&") return TokenType::AND;
    if (identifier == "||") return TokenType::OR;

    // 关键字判断：将字符串映射到对应的关键字 token
    if (identifier == "main") return TokenType::MAINTK;
    if (identifier == "const") return TokenType::CONSTTK;
    if (identifier == "int") return TokenType::INTTK;
    if (identifier == "static") return TokenType::STATICTK;
    if (identifier == "void") return TokenType::VOIDTK;
    if (identifier == "break") return TokenType::BREAKTK;
    if (identifier == "continue") return TokenType::CONTINUETK;
    if (identifier == "if") return TokenType::IFTK;
    if (identifier == "else") return TokenType::ELSETK;
    if (identifier == "for") return TokenType::FORTK;
    if (identifier == "printf") return TokenType::PRINTFTK;
    if (identifier == "return") return TokenType::RETURNTK;

    // 默认为标识符类型
    return TokenType::IDENFR;
}

inline TokenType GetTokenType(char c) {
    // 根据单字符直接返回对应的 token 类型（用于简单符号匹配）
    switch (c) {
        case '+': return TokenType::PLUS;
        case '-': return TokenType::MINU;
        case '*': return TokenType::MULT;
        case '/': return TokenType::DIV;
        case '%': return TokenType::MOD;
        case ';': return TokenType::SEMICN;
        case ',': return TokenType::COMMA;
        case '(': return TokenType::LPARENT;
        case ')': return TokenType::RPARENT;
        case '[': return TokenType::LBRACK;
        case ']': return TokenType::RBRACK;
        case '{': return TokenType::LBRACE;
        case '}': return TokenType::RBRACE;
        case '<': return TokenType::LSS;
        case '>': return TokenType::GRE;
        case '=': return TokenType::ASSIGN;
        case '!': return TokenType::NOT;
        default: return TokenType::ERROR_T;
    }
}

inline std::string TokenTypeToString(TokenType t) {
    // 将 TokenType 转换为字符串表示，便于写入 parser.txt 或调试输出
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

