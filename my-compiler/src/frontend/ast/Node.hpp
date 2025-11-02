#pragma once

#include "../lexer/TokenStream.hpp"

class Node {
public:
    virtual ~Node() = default;
    virtual void Parse() = 0;

    static void SetTokenStream(TokenStream* ts) { tokenStream = ts; }
    static TokenStream* GetTokenStream() { return tokenStream; }

protected:
    static TokenStream* tokenStream;
};
