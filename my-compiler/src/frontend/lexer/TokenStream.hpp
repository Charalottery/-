#pragma once
#include "frontend/lexer/Token.hpp"
#include <vector>
#include <stack>

class TokenStream {
public:
    // 构造函数：接收一个 token 向量并设置读取指针到起始位置
    explicit TokenStream(std::vector<Token> tokens)
        : tokenList(std::move(tokens)), readPoint(0) {}

    // 读取下一个 token（推进读取指针）
    void Read() { if (readPoint < tokenList.size()) ++readPoint; }

    // 向前查看 peekStep 个 token（0 表示当前 token），不改变读取指针
    // 如果超出范围，返回一个表示 EOF 的 Token
    Token Peek(size_t peekStep) const {
        if (readPoint + peekStep >= tokenList.size())
            return Token(TokenType::EOF_T, "end of token stream", -1);
        return tokenList[readPoint + peekStep];
    }

    // 支持回溯：记录当前读指针到栈中，之后调用 GoToBackPoint 恢复到该点
    void SetBackPoint() { backPointStack.push(readPoint); }
    void GoToBackPoint() { if (!backPointStack.empty()) { readPoint = backPointStack.top(); backPointStack.pop(); } }

private:
    // token 列表和当前读取索引、回溯点栈
    std::vector<Token> tokenList;
    size_t readPoint;
    std::stack<size_t> backPointStack;
};
