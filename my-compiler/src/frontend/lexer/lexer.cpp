#include "Lexer.hpp"
#include <cctype>
#include <stdexcept>
#include <iostream>
#include "../../error/ErrorRecorder.hpp"

static bool isIdentifierStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
static bool isIdentifierPart(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

Lexer::Lexer(const std::string &src)
    : source(src), curPos(0), token(), tokenType(TokenType::ERROR_T), reserveWords(), lineNum(1), number(0)
{
    // initialize reserve words
    reserveWords = {"main", "const", "int", "char", "void", "break", "continue", "if", "else", "for", "getint", "getchar", "printf", "return", "static"};
}

char Lexer::currentChar() const
{
    if (curPos >= source.size())
        return '\0';
    return source[curPos];
}

char Lexer::peekChar(size_t offset) const
{
    size_t p = curPos + offset;
    if (p >= source.size())
        return '\0';
    return source[p];
}

void Lexer::advance(size_t step)
{
    for (size_t i = 0; i < step && curPos < source.size(); ++i)
    {
        if (source[curPos] == '\n')
            ++lineNum;
        ++curPos;
    }
}

void Lexer::skipWhitespace()
{
    while (true)
    {
        char c = currentChar();
        if (c == '\0')
            return;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            advance();
            continue;
        }
        break;
    }
}

void Lexer::next()
{
    token.clear();
    tokenType = TokenType::ERROR_T;
    number = 0;

    skipWhitespace();
    char c = currentChar();
    if (c == '\0')
    {
        tokenType = TokenType::EOF_T;
        token = "EOF";
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    if (std::isdigit(static_cast<unsigned char>(c)))
    {
        scanNumber();
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    if (c == '"')
    {
        scanString();
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    if (c == '\'')
    {
        scanCharacter();
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    if (isIdentifierStart(c))
    {
        scanIdentifierOrKeyword();
        tokenList.push_back(Token(tokenType, token, lineNum));
        return;
    }

    // operator or comment
    scanOperatorOrComment();
    if (!token.empty())
        tokenList.push_back(Token(tokenType, token, lineNum));
}

std::string Lexer::getToken() const { return token; }
TokenType Lexer::getTokenType() const { return tokenType; }

void Lexer::GenerateTokenList()
{
    // reset position
    curPos = 0;
    tokenList.clear();
    lineNum = 1;
    while (true)
    {
        next();
        if (!tokenList.empty() && tokenList.back().type == TokenType::EOF_T)
            break;
    }
}

void Lexer::scanNumber()
{
    size_t start = curPos;
    while (std::isdigit(static_cast<unsigned char>(currentChar())))
        advance();
    token = source.substr(start, curPos - start);
    try
    {
        number = std::stoll(token);
    }
    catch (...)
    {
        number = 0;
    }
    tokenType = TokenType::INTCON;
}

void Lexer::scanString()
{
    size_t start = curPos;
    advance(); // skip '"'
    while (currentChar() != '\0' && currentChar() != '"')
    {
        if (currentChar() == '\\')
        {
            advance(1); // include escape char
            if (currentChar() != '\0')
                advance(1);
            continue;
        }
        advance();
    }
    if (currentChar() == '"')
        advance();
    token = source.substr(start, curPos - start);
    tokenType = TokenType::STRCON;
}

void Lexer::scanCharacter()
{
    size_t start = curPos;
    advance(); // skip '\''
    if (currentChar() == '\\')
    {
        advance();
        if (currentChar() != '\0')
            advance();
    }
    else
    {
        if (currentChar() != '\0')
            advance();
    }
    if (currentChar() == '\'')
        advance();
    token = source.substr(start, curPos - start);
    // Character constants are not in the user's token table; mark as ERROR
    tokenType = TokenType::ERROR_T;
}

void Lexer::scanIdentifierOrKeyword()
{
    size_t start = curPos;
    advance();
    while (isIdentifierPart(currentChar()))
        advance();
    token = source.substr(start, curPos - start);
    if (reserveWords.find(token) != reserveWords.end())
    {
        tokenType = GetTokenType(token);
    }
    else
    {
        tokenType = TokenType::IDENFR;
    }
}

void Lexer::scanOperatorOrComment()
{
    char c = currentChar();
    char n = peekChar(1);

    // comments
    if (c == '/' && n == '/')
    {
        // single-line comment
        advance(2);
        while (currentChar() != '\0' && currentChar() != '\n')
            advance();
        // skip newline
        if (currentChar() == '\n')
            advance();
        // after skipping comment, return to outer next() so it will continue scanning
        token.clear();
        tokenType = TokenType::ERROR_T;
        return;
    }
    if (c == '/' && n == '*')
    {
        // multi-line comment
        advance(2);
        while (true)
        {
            if (currentChar() == '\0')
            {
                tokenType = TokenType::EOF_T;
                token = "EOF";
                return;
            }
            if (currentChar() == '*' && peekChar(1) == '/')
            {
                advance(2);
                break;
            }
            advance();
        }
        // after skipping, return to outer next() so it will continue scanning
        token.clear();
        tokenType = TokenType::ERROR_T;
        return;
        return;
    }

    // two-char operators: <= >= == != && ||
    std::string two;
    two.reserve(2);
    two.push_back(c);
    two.push_back(n);
    if (two == "<=" || two == ">=" || two == "==" || two == "!=" || two == "&&" || two == "||")
    {
        token = two;
        tokenType = GetTokenType(token);
        advance(2);
        return;
    }

    // single '&' or '|' are illegal symbols per spec -> record error
    // BUT treat them as logical operators for parsing (as '&&' / '||')
    // while keeping the recorded token text as '&' or '|' (per spec)
    if (c == '&' && n != '&')
    {
        // record illegal-symbol error at this line with the single-character name
        ErrorRecorder::AddError(Error(ErrorType::ILLEGAL_SYMBOL, lineNum, "&"));
        // produce a logical-AND token so parser can continue as if '&&' was present
        token = "&";                // keep the token text as single '&' for recording/printing
        tokenType = TokenType::AND; // parser expects TokenType::AND for logical and
        advance();
        return;
    }
    if (c == '|' && n != '|')
    {
        ErrorRecorder::AddError(Error(ErrorType::ILLEGAL_SYMBOL, lineNum, "|"));
        token = "|";
        tokenType = TokenType::OR; // treat as logical-or for parser
        advance();
        return;
    }

    // single-char operators and punctuation
    token = std::string(1, c);
    tokenType = GetTokenType(c);
    advance();
}
