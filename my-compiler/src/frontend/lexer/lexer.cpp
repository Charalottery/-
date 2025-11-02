#include "frontend/lexer/Lexer.hpp"
#include <cctype>
#include <stdexcept>
#include <iostream>
#include "error/ErrorRecorder.hpp"

// 判断标识符开始字符（字母或下划线）
static bool isIdentifierStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
// 判断标识符的后续字符（字母、数字或下划线）
static bool isIdentifierPart(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

Lexer::Lexer(const std::string &src)
    : source(src), curPos(0), token(), tokenType(TokenType::ERROR_T), reserveWords(), lineNum(1), number(0) {
    // 构造函数：初始化源字符串和词法分析状态
    // 初始化保留字集合（关键字）
    reserveWords = {"main","const","int","char","void","break","continue","if","else","for","getint","getchar","printf","return","static"};
}

char Lexer::currentChar() const {
    if (curPos >= source.size()) return '\0';
    return source[curPos];
}

char Lexer::peekChar(size_t offset) const {
    size_t p = curPos + offset;
    if (p >= source.size()) return '\0';
    return source[p];
}

void Lexer::advance(size_t step) {
    // 前移指针并在遇到换行时更新行号
    for (size_t i = 0; i < step && curPos < source.size(); ++i) {
        if (source[curPos] == '\n') ++lineNum;
        ++curPos;
    }
}

void Lexer::skipWhitespace() {
    // 跳过空白字符（空格、制表、回车、换行）。注意：换行会由 advance 更新行号。
    while (true) {
        char c = currentChar();
        if (c == '\0') return;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
            continue;
        }
        break;
    }
}

void Lexer::next() {
    // 识别下一个 token 并将其加入 tokenList（同时更新 token, tokenType）
    token.clear();
    tokenType = TokenType::ERROR_T;
    number = 0;

    skipWhitespace();
    char c = currentChar();
    // 处理文件结尾
    if (c == '\0') {
        tokenType = TokenType::EOF_T;
        token = "EOF";
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    // 数字常量
    if (std::isdigit(static_cast<unsigned char>(c))) {
        scanNumber();
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    // 字符串常量
    if (c == '"') {
        scanString();
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    // 字符常量
    if (c == '\'') {
        scanCharacter();
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    // 标识符或关键字
    if (isIdentifierStart(c)) {
        scanIdentifierOrKeyword();
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    // 运算符或注释（包含单/多行注释）
    scanOperatorOrComment();
    if (!token.empty()) tokenList.push_back(Token(tokenType, token, lineNum));
}

std::string Lexer::getToken() const { return token; }
TokenType Lexer::getTokenType() const { return tokenType; }

void Lexer::GenerateTokenList() {
    // reset position
    curPos = 0; tokenList.clear(); lineNum = 1;
    while (true) {
        next();
        if (!tokenList.empty() && tokenList.back().type == TokenType::EOF_T) break;
        if (curPos >= source.size()) break;
    }
}

void Lexer::scanNumber() {
    // 解析整数字面量（连续的数字），并尝试转换为 long long
    size_t start = curPos;
    while (std::isdigit(static_cast<unsigned char>(currentChar()))) advance();
    token = source.substr(start, curPos - start);
    try { number = std::stoll(token); } catch(...) { number = 0; }
    tokenType = TokenType::INTCON;
}

void Lexer::scanString() {
    // 解析字符串字面量（双引号括起来），保留转义序列
    size_t start = curPos;
    advance(); // skip '"'
    while (currentChar() != '\0' && currentChar() != '"') {
        if (currentChar() == '\\') {
            // 碰到转义字符，跳过转义符和被转义字符
            advance(1); // include escape char
            if (currentChar() != '\0') advance(1);
            continue;
        }
        advance();
    }
    if (currentChar() == '"') advance();
    token = source.substr(start, curPos - start);
    tokenType = TokenType::STRCON;
}

void Lexer::scanCharacter() {
    // 解析字符字面量（单引号括起来），当前实现不将其视为合法 token
    size_t start = curPos;
    advance(); // skip '\''
    if (currentChar() == '\\') {
        advance();
        if (currentChar() != '\0') advance();
    } else {
        if (currentChar() != '\0') advance();
    }
    if (currentChar() == '\'') advance();
    token = source.substr(start, curPos - start);
    // 字符常量在本项目的 token 列表中未定义，标记为 ERROR_T
    tokenType = TokenType::ERROR_T;
}

void Lexer::scanIdentifierOrKeyword() {
    // 解析标识符或关键字：以字母/下划线开始，后续为字母/数字/下划线
    size_t start = curPos;
    advance();
    while (isIdentifierPart(currentChar())) advance();
    token = source.substr(start, curPos - start);
    if (reserveWords.find(token) != reserveWords.end()) {
        // 若在保留字集合中，返回相应的关键字 token
        tokenType = GetTokenType(token);
    } else {
        tokenType = TokenType::IDENFR;
    }
}

void Lexer::scanOperatorOrComment() {
    // 处理运算符和注释的识别逻辑
    char c = currentChar();
    char n = peekChar(1);

    // 单行注释："// ... \n"
    if (c == '/' && n == '/') {
        // consume '//' 并跳过到行尾
        advance(2);
        while (currentChar() != '\0' && currentChar() != '\n') advance();
        // 跳过换行符
        if (currentChar() == '\n') advance();
        // 注释不产生 token，设置为 ERROR_T 并返回，由外层 next() 继续扫描
        token.clear();
        tokenType = TokenType::ERROR_T;
        return;
    }
    // 多行注释："/* ... */"，支持跨行
    if (c == '/' && n == '*') {
        advance(2);
        while (true) {
            if (currentChar() == '\0') {
                // 遇到文件结尾但注释未闭合，视为文件结束的特殊处理
                tokenType = TokenType::EOF_T;
                token = "EOF";
                return;
            }
            if (currentChar() == '*' && peekChar(1) == '/') {
                advance(2);
                break;
            }
            advance();
        }
        // 跳过注释后不产生 token，返回让外层继续扫描
        token.clear();
        tokenType = TokenType::ERROR_T;
        return;
    }

    // 两字符运算符（<=, >=, ==, !=, &&, ||）
    std::string two; two.reserve(2);
    two.push_back(c);
    two.push_back(n);
    if (two == "<=" || two == ">=" || two == "==" || two == "!=" || two == "&&" || two == "||") {
        token = two;
        tokenType = GetTokenType(token);
        advance(2);
        return;
    }

    // 单字符 '&' 或 '|' 若非成对出现则视为非法符号，按规范记录错误
    if (c == '&' && n != '&') {
        ErrorRecorder::AddError(Error(ErrorType::ILLEGAL_SYMBOL, lineNum, "Illegal single '&'"));
        token = "&";
        tokenType = TokenType::ERROR_T;
        advance();
        return;
    }
    if (c == '|' && n != '|') {
        ErrorRecorder::AddError(Error(ErrorType::ILLEGAL_SYMBOL, lineNum, "Illegal single '|'"));
        token = "|";
        tokenType = TokenType::ERROR_T;
        advance();
        return;
    }

    // 其余单字符运算符或标点（例如 + - * / ; , ( ) { } 等）
    token = std::string(1, c);
    tokenType = GetTokenType(c);
    advance();
}
