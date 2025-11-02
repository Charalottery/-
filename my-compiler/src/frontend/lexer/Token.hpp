#pragma once
#include "frontend/lexer/TokenType.hpp"
#include <string>

struct Token {
    TokenType type;
    std::string value;
    int line;
    // Token 表示词法单元：类型、在源代码中的文本值以及所在行号
    Token(TokenType t = TokenType::ERROR_T, std::string v = "", int l = -1)
        : type(t), value(std::move(v)), line(l) {}

    // 返回 token 的字符串表示（当前实现返回其原始文本值）
    std::string toString() const { return value; }
};
