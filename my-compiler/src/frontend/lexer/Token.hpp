#pragma once
#include "TokenType.hpp"
#include <string>

struct Token {
    TokenType type;
    std::string value;
    int line;

    Token(TokenType t = TokenType::ERROR_T, std::string v = "", int l = -1)
        : type(t), value(std::move(v)), line(l) {}

    std::string toString() const { return value; }
};
