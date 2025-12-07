#pragma once
#include <string>

// Token types as specified by the user
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
    WHILETK,
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
    // multi-char operators and keywords
    if (identifier == "<=") return TokenType::LEQ;
    if (identifier == ">=") return TokenType::GEQ;
    if (identifier == "==") return TokenType::EQL;
    if (identifier == "!=") return TokenType::NEQ;
    if (identifier == "&&") return TokenType::AND;
    if (identifier == "||") return TokenType::OR;

    // keywords
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
    if (identifier == "while") return TokenType::WHILETK;
    if (identifier == "printf") return TokenType::PRINTFTK;
    if (identifier == "return") return TokenType::RETURNTK;

    return TokenType::IDENFR;
}

inline TokenType GetTokenType(char c) {
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
