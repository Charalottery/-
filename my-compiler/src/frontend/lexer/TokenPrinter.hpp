#pragma once
#include "Token.hpp"
#include "TokenType.hpp"
#include <ostream>
#include <string>

inline std::string TokenTypeToString(TokenType t) {
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
        case TokenType::WHILETK: return "WHILETK";
        case TokenType::EOF_T: return "EOF";
        case TokenType::ERROR_T: return "ERROR";
    }
    return "UNKNOWN";
}

inline std::ostream &operator<<(std::ostream &os, TokenType t) {
    os << TokenTypeToString(t);
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const Token &tok) {
    os << TokenTypeToString(tok.type) << " " << tok.value;
    return os;
}
