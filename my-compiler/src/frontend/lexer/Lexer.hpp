#pragma once
#include "frontend/lexer/Token.hpp"
#include "frontend/lexer/TokenType.hpp"
#include <string>
#include <vector>
#include <unordered_set>

class Lexer {
public:
    // 词法分析器类：从源字符串中逐个识别 token
    // 构造函数：接收完整的源代码字符串
    explicit Lexer(const std::string &src);

    // 读取下一个 token，并更新内部状态（token、tokenType、lineNum 等）
    void next();

    // 访问当前 token 的文本和类型
    std::string getToken() const;
    TokenType getTokenType() const;

    // 可选：生成完整的 token 列表并返回（向后兼容接口）
    void GenerateTokenList();
    const std::vector<Token>& GetTokenList() const { return tokenList; }

private:
private:
    // 源程序及词法分析状态
    std::string source;      // 源程序字符串（整个文件/输入）
    size_t curPos;           // 当前解析位置（索引）
    std::string token;       // 当前 token 的文本值
    TokenType tokenType;     // 当前 token 的类型
    std::unordered_set<std::string> reserveWords; // 关键字/保留字集合
    int lineNum;             // 当前行号（用于错误定位）
    long long number;        // 临时存放解析的整数值

    std::vector<Token> tokenList; // 可选的完整 token 列表

    // 内部辅助函数：字符读取、前瞻、跳过空白以及各种词素扫描函数
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
