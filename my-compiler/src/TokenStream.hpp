#pragma once
#include "Token.hpp"
#include <vector>
#include <stack>

class TokenStream {
public:
    explicit TokenStream(std::vector<Token> tokens)
        : tokenList(std::move(tokens)), readPoint(0) {}

    void Read() { if (readPoint < tokenList.size()) ++readPoint; }

    Token Peek(size_t peekStep) const {
        if (readPoint + peekStep >= tokenList.size())
            return Token(TokenType::EOF_T, "end of token stream", -1);
        return tokenList[readPoint + peekStep];
    }

    void SetBackPoint() { backPointStack.push(readPoint); }
    void GoToBackPoint() { if (!backPointStack.empty()) { readPoint = backPointStack.top(); backPointStack.pop(); } }

private:
    std::vector<Token> tokenList;
    size_t readPoint;
    std::stack<size_t> backPointStack;
};
