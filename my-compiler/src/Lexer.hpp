#pragma once
#include "Token.hpp"
#include "TokenType.hpp"
#include <string>
#include <vector>
#include <unordered_set>

class Lexer {
public:
    // Construct from source string
    explicit Lexer(const std::string &src);

    // Advance to next token and fill internal state
    void next();

    // Accessors for current token
    std::string getToken() const;
    TokenType getTokenType() const;

    // Optional: generate full token list (keeps compatibility)
    void GenerateTokenList();
    const std::vector<Token>& GetTokenList() const { return tokenList; }

private:
    std::string source;      // 源程序字符串
    size_t curPos;           // 当前字符串位置指针
    std::string token;       // 解析单词值
    TokenType tokenType;     // 解析单词类型
    std::unordered_set<std::string> reserveWords; // 保留字表
    int lineNum;             // 当前行号
    long long number;        // 解析的数值（如整数）

    std::vector<Token> tokenList; // 用于 GenerateTokenList

    // helpers
    char currentChar() const;
    char peekChar(size_t offset = 1) const;
    void advance(size_t step = 1);
    void skipWhitespace();

    void scanNumber();
    void scanString();
    void scanCharacter();
    void scanIdentifierOrKeyword();
    void scanOperatorOrComment();
};
