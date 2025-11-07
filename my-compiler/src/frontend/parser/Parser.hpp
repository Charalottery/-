#pragma once
#include "../lexer/TokenStream.hpp"
#include "../ast/AST.hpp"
#include "../lexer/Token.hpp"
#include <memory>

class Parser {
public:
    explicit Parser(TokenStream &ts) : ts(ts) {}

    // Entry for the grammar fragment provided
    std::unique_ptr<ASTNode> ParseCompUnit();

private:
    TokenStream &ts;

    // helpers
    bool Match(TokenType t) const;
    bool MatchValue(const std::string &v) const;
    Token PeekToken(int k = 0) const { return ts.Peek(k); }
    void ReadToken() { ts.Read(); }
    Token Consume();

    std::unique_ptr<ASTNode> MakeTokenNode(const Token &t) { return std::make_unique<ASTNode>(t); }

    // grammar subroutines (per provided EBNF)
    std::unique_ptr<ASTNode> ParseDecl();
    std::unique_ptr<ASTNode> ParseConstDecl();
    std::unique_ptr<ASTNode> ParseBType();
    std::unique_ptr<ASTNode> ParseConstDef();
    std::unique_ptr<ASTNode> ParseConstInitVal();

    std::unique_ptr<ASTNode> ParseVarDecl();
    std::unique_ptr<ASTNode> ParseVarDef();
    std::unique_ptr<ASTNode> ParseInitVal();

    std::unique_ptr<ASTNode> ParseFuncDef();
    std::unique_ptr<ASTNode> ParseMainFuncDef();
    std::unique_ptr<ASTNode> ParseFuncType();
    std::unique_ptr<ASTNode> ParseFuncFParams();
    std::unique_ptr<ASTNode> ParseFuncFParam();

    std::unique_ptr<ASTNode> ParseBlock();
    std::unique_ptr<ASTNode> ParseBlockItem();
    std::unique_ptr<ASTNode> ParseStmt();
    std::unique_ptr<ASTNode> ParseForStmt();

    // expressions
    std::unique_ptr<ASTNode> ParseExp();
    std::unique_ptr<ASTNode> ParseCond();
    std::unique_ptr<ASTNode> ParseLVal();
    std::unique_ptr<ASTNode> ParsePrimaryExp();
    std::unique_ptr<ASTNode> ParseNumber();
    std::unique_ptr<ASTNode> ParseUnaryExp();
    std::unique_ptr<ASTNode> ParseUnaryOp();
    std::unique_ptr<ASTNode> ParseFuncRParams();

    std::unique_ptr<ASTNode> ParseMulExp();
    std::unique_ptr<ASTNode> ParseAddExp();
    std::unique_ptr<ASTNode> ParseAddExpPrime();
    std::unique_ptr<ASTNode> ParseRelExp();
    std::unique_ptr<ASTNode> ParseEqExp();
    std::unique_ptr<ASTNode> ParseLAndExp();
    std::unique_ptr<ASTNode> ParseLOrExp();
    std::unique_ptr<ASTNode> ParseConstExp();
};
