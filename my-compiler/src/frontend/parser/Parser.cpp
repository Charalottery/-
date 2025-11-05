#include "Parser.hpp"
#include "../lexer/TokenPrinter.hpp"
#include <iostream>

using std::make_unique;

bool Parser::Match(TokenType t) const {
    Token tk = PeekToken(0);
    return tk.type == t;
}

bool Parser::MatchValue(const std::string &v) const {
    Token tk = PeekToken(0);
    return tk.value == v;
}

Token Parser::Consume() {
    Token tk = PeekToken(0);
    ReadToken();
    return tk;
}

std::unique_ptr<ASTNode> Parser::ParseCompUnit() {
    auto node = make_unique<ASTNode>("CompUnit");

    // {Decl} {FuncDef} MainFuncDef
    while (true) {
        Token cur = PeekToken(0);
        // detect main function start: 'int' 'main' '('
        if (cur.type == TokenType::INTTK && PeekToken(1).type == TokenType::MAINTK && PeekToken(2).type == TokenType::LPARENT) {
            break;
        }
        if (cur.type == TokenType::CONSTTK || cur.type == TokenType::STATICTK) {
            node->AddChild(ParseDecl());
            continue;
        }
        if (cur.type == TokenType::INTTK || cur.type == TokenType::VOIDTK) {
            // decide FuncDef vs Decl: if followed by ID and '(', it's FuncDef
            if (PeekToken(1).type == TokenType::IDENFR && PeekToken(2).type == TokenType::LPARENT) {
                node->AddChild(ParseFuncDef());
                continue;
            } else {
                node->AddChild(ParseDecl());
                continue;
            }
        }
        // if EOF or unexpected, break
        if (cur.type == TokenType::EOF_T) break;
        // fallback: try to consume something to avoid infinite loop
        ReadToken();
    }

    // MainFuncDef
    node->AddChild(ParseMainFuncDef());

    return node;
}

std::unique_ptr<ASTNode> Parser::ParseDecl() {
    auto node = make_unique<ASTNode>("Decl");
    if (PeekToken(0).type == TokenType::CONSTTK) {
        node->AddChild(ParseConstDecl());
    } else {
        node->AddChild(ParseVarDecl());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseConstDecl() {
    auto node = make_unique<ASTNode>("ConstDecl");
    // 'const'
    if (Match(TokenType::CONSTTK)) node->AddChild(MakeTokenNode(Consume()));
    node->AddChild(ParseBType());
    node->AddChild(ParseConstDef());
    while (Match(TokenType::COMMA)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseConstDef());
    }
    if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseBType() {
    auto node = make_unique<ASTNode>("BType");
    if (Match(TokenType::INTTK)) node->AddChild(MakeTokenNode(Consume()));
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseConstDef() {
    auto node = make_unique<ASTNode>("ConstDef");
    if (Match(TokenType::IDENFR)) node->AddChild(MakeTokenNode(Consume()));
    if (Match(TokenType::LBRACK)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseConstExp());
        if (Match(TokenType::RBRACK)) node->AddChild(MakeTokenNode(Consume()));
    }
    if (Match(TokenType::ASSIGN)) node->AddChild(MakeTokenNode(Consume()));
    node->AddChild(ParseConstInitVal());
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseConstInitVal() {
    auto node = make_unique<ASTNode>("ConstInitVal");
    if (Match(TokenType::LBRACE)) {
        node->AddChild(MakeTokenNode(Consume()));
        // optional list
        if (PeekToken(0).type != TokenType::RBRACE) {
            node->AddChild(ParseConstExp());
            while (Match(TokenType::COMMA)) {
                node->AddChild(MakeTokenNode(Consume()));
                node->AddChild(ParseConstExp());
            }
        }
        if (Match(TokenType::RBRACE)) node->AddChild(MakeTokenNode(Consume()));
    } else {
        node->AddChild(ParseConstExp());
    }
    return node;
}


std::unique_ptr<ASTNode> Parser::ParseVarDecl() {
    auto node = make_unique<ASTNode>("VarDecl");
    if (Match(TokenType::STATICTK)) node->AddChild(MakeTokenNode(Consume()));
    node->AddChild(ParseBType());
    node->AddChild(ParseVarDef());
    while (Match(TokenType::COMMA)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseVarDef());
    }
    if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseVarDef() {
    auto node = make_unique<ASTNode>("VarDef");
    if (Match(TokenType::IDENFR)) node->AddChild(MakeTokenNode(Consume()));
    if (Match(TokenType::LBRACK)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseConstExp());
        if (Match(TokenType::RBRACK)) node->AddChild(MakeTokenNode(Consume()));
    }
    if (Match(TokenType::ASSIGN)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseInitVal());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseInitVal() {
    auto node = make_unique<ASTNode>("InitVal");
    if (Match(TokenType::LBRACE)) {
        node->AddChild(MakeTokenNode(Consume()));
        if (PeekToken(0).type != TokenType::RBRACE) {
            node->AddChild(ParseInitVal());
            while (Match(TokenType::COMMA)) {
                node->AddChild(MakeTokenNode(Consume()));
                node->AddChild(ParseInitVal());
            }
        }
        if (Match(TokenType::RBRACE)) node->AddChild(MakeTokenNode(Consume()));
    } else {
        // for now treat as single token expression
        Token tk = PeekToken(0);
        if (tk.type == TokenType::IDENFR || tk.type == TokenType::INTCON) {
            node->AddChild(MakeTokenNode(Consume()));
        }
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseFuncDef() {
    auto node = make_unique<ASTNode>("FuncDef");
    node->AddChild(ParseFuncType());
    if (Match(TokenType::IDENFR)) node->AddChild(MakeTokenNode(Consume()));
    if (Match(TokenType::LPARENT)) node->AddChild(MakeTokenNode(Consume()));
    if (PeekToken(0).type != TokenType::RPARENT) {
        node->AddChild(ParseFuncFParams());
    }
    if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
    node->AddChild(ParseBlock());
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseMainFuncDef() {
    auto node = make_unique<ASTNode>("MainFuncDef");
    if (Match(TokenType::INTTK)) node->AddChild(MakeTokenNode(Consume()));
    if (Match(TokenType::MAINTK)) node->AddChild(MakeTokenNode(Consume()));
    if (Match(TokenType::LPARENT)) node->AddChild(MakeTokenNode(Consume()));
    if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
    node->AddChild(ParseBlock());
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseFuncType() {
    auto node = make_unique<ASTNode>("FuncType");
    Token tk = PeekToken(0);
    if (tk.type == TokenType::VOIDTK || tk.type == TokenType::INTTK) {
        node->AddChild(MakeTokenNode(Consume()));
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseFuncFParams() {
    auto node = make_unique<ASTNode>("FuncFParams");
    node->AddChild(ParseFuncFParam());
    while (Match(TokenType::COMMA)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseFuncFParam());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseFuncFParam() {
    auto node = make_unique<ASTNode>("FuncFParam");
    node->AddChild(ParseBType());
    if (Match(TokenType::IDENFR)) node->AddChild(MakeTokenNode(Consume()));
    if (Match(TokenType::LBRACK)) {
        node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::RBRACK)) node->AddChild(MakeTokenNode(Consume()));
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseBlock() {
    auto node = make_unique<ASTNode>("Block");
    // Parse '{' { BlockItem } '}'
    if (Match(TokenType::LBRACE)) node->AddChild(MakeTokenNode(Consume()));
    while (PeekToken(0).type != TokenType::RBRACE && PeekToken(0).type != TokenType::EOF_T) {
        node->AddChild(ParseBlockItem());
    }
    if (Match(TokenType::RBRACE)) node->AddChild(MakeTokenNode(Consume()));
    return node;
}

// BlockItem -> Decl | Stmt
std::unique_ptr<ASTNode> Parser::ParseBlockItem() {
    auto node = make_unique<ASTNode>("BlockItem");
    Token tk = PeekToken(0);
    if (tk.type == TokenType::CONSTTK || tk.type == TokenType::INTTK || tk.type == TokenType::STATICTK) {
        node->AddChild(ParseDecl());
    } else {
        node->AddChild(ParseStmt());
    }
    return node;
}

// Stmt -> many forms
std::unique_ptr<ASTNode> Parser::ParseStmt() {
    auto node = make_unique<ASTNode>("Stmt");
    Token tk = PeekToken(0);
    if (tk.type == TokenType::LBRACE) {
        node->AddChild(ParseBlock());
        return node;
    }
    if (tk.type == TokenType::IFTK) {
        node->AddChild(MakeTokenNode(Consume())); // if
        if (Match(TokenType::LPARENT)) node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseCond());
        if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseStmt());
        if (Match(TokenType::ELSETK)) {
            node->AddChild(MakeTokenNode(Consume()));
            node->AddChild(ParseStmt());
        }
        return node;
    }
    if (tk.type == TokenType::FORTK) {
        node->AddChild(MakeTokenNode(Consume())); // for
        if (Match(TokenType::LPARENT)) node->AddChild(MakeTokenNode(Consume()));
        // [ForStmt]
        if (PeekToken(0).type != TokenType::SEMICN) node->AddChild(ParseForStmt());
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        // [Cond]
        if (PeekToken(0).type != TokenType::SEMICN) node->AddChild(ParseCond());
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        // [ForStmt]
        if (PeekToken(0).type != TokenType::RPARENT) node->AddChild(ParseForStmt());
        if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseStmt());
        return node;
    }
    if (tk.type == TokenType::BREAKTK) {
        node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }
    if (tk.type == TokenType::CONTINUETK) {
        node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }
    if (tk.type == TokenType::RETURNTK) {
        node->AddChild(MakeTokenNode(Consume()));
        if (PeekToken(0).type != TokenType::SEMICN) node->AddChild(ParseExp());
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }
    if (tk.type == TokenType::PRINTFTK) {
        node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::LPARENT)) node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::STRCON)) node->AddChild(MakeTokenNode(Consume()));
        while (Match(TokenType::COMMA)) {
            node->AddChild(MakeTokenNode(Consume()));
            node->AddChild(ParseExp());
        }
        if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }

    // LVal '=' Exp ';' | [Exp] ';' or function call as expression
    if (tk.type == TokenType::IDENFR) {
        // Lookahead to decide assignment vs expression
        if (PeekToken(1).type == TokenType::ASSIGN || PeekToken(1).type == TokenType::LBRACK) {
            // parse LVal
            node->AddChild(ParseLVal());
            if (Match(TokenType::ASSIGN)) node->AddChild(MakeTokenNode(Consume()));
            node->AddChild(ParseExp());
            if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
            return node;
        }
    }

    // [Exp] ';' (expression stmt)
    if (PeekToken(0).type == TokenType::SEMICN) {
        // empty stmt
        node->AddChild(MakeTokenNode(Consume()));
        return node;
    } else {
        node->AddChild(ParseExp());
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }
}

// ForStmt -> LVal '=' Exp { ',' LVal '=' Exp }
std::unique_ptr<ASTNode> Parser::ParseForStmt() {
    auto node = make_unique<ASTNode>("ForStmt");
    node->AddChild(ParseLVal());
    if (Match(TokenType::ASSIGN)) node->AddChild(MakeTokenNode(Consume()));
    node->AddChild(ParseExp());
    while (Match(TokenType::COMMA)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseLVal());
        if (Match(TokenType::ASSIGN)) node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseExp());
    }
    return node;
}

// Expressions
std::unique_ptr<ASTNode> Parser::ParseExp() { return ParseAddExp(); }
std::unique_ptr<ASTNode> Parser::ParseCond() { return ParseLOrExp(); }

std::unique_ptr<ASTNode> Parser::ParseLVal() {
    auto node = make_unique<ASTNode>("LVal");
    if (Match(TokenType::IDENFR)) node->AddChild(MakeTokenNode(Consume()));
    if (Match(TokenType::LBRACK)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseExp());
        if (Match(TokenType::RBRACK)) node->AddChild(MakeTokenNode(Consume()));
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParsePrimaryExp() {
    auto node = make_unique<ASTNode>("PrimaryExp");
    if (Match(TokenType::LPARENT)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseExp());
        if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
    } else if (Match(TokenType::IDENFR)) {
        // could be LVal
        // peek ahead for '[' indicates LVal
        if (PeekToken(1).type == TokenType::LBRACK) {
            node->AddChild(ParseLVal());
        } else {
            node->AddChild(MakeTokenNode(Consume()));
        }
    } else {
        node->AddChild(ParseNumber());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseNumber() {
    auto node = make_unique<ASTNode>("Number");
    if (Match(TokenType::INTCON)) node->AddChild(MakeTokenNode(Consume()));
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseUnaryExp() {
    auto node = make_unique<ASTNode>("UnaryExp");
    if (Match(TokenType::IDENFR) && PeekToken(1).type == TokenType::LPARENT) {
        node->AddChild(MakeTokenNode(Consume())); // func name
        if (Match(TokenType::LPARENT)) node->AddChild(MakeTokenNode(Consume()));
        if (PeekToken(0).type != TokenType::RPARENT) node->AddChild(ParseFuncRParams());
        if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
    } else if (Match(TokenType::PLUS) || Match(TokenType::MINU) || Match(TokenType::NOT)) {
        node->AddChild(ParseUnaryOp());
        node->AddChild(ParseUnaryExp());
    } else {
        node->AddChild(ParsePrimaryExp());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseUnaryOp() {
    auto node = make_unique<ASTNode>("UnaryOp");
    if (Match(TokenType::PLUS) || Match(TokenType::MINU) || Match(TokenType::NOT)) node->AddChild(MakeTokenNode(Consume()));
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseFuncRParams() {
    auto node = make_unique<ASTNode>("FuncRParams");
    node->AddChild(ParseExp());
    while (Match(TokenType::COMMA)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseExp());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseMulExp() {
    auto node = make_unique<ASTNode>("MulExp");
    node->AddChild(ParseUnaryExp());
    while (Match(TokenType::MULT) || Match(TokenType::DIV) || Match(TokenType::MOD)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseUnaryExp());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseAddExp() {
    auto node = make_unique<ASTNode>("AddExp");
    node->AddChild(ParseMulExp());
    while (Match(TokenType::PLUS) || Match(TokenType::MINU)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseMulExp());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseRelExp() {
    auto node = make_unique<ASTNode>("RelExp");
    node->AddChild(ParseAddExp());
    while (Match(TokenType::LSS) || Match(TokenType::GRE) || Match(TokenType::LEQ) || Match(TokenType::GEQ)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseAddExp());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseEqExp() {
    auto node = make_unique<ASTNode>("EqExp");
    node->AddChild(ParseRelExp());
    while (Match(TokenType::EQL) || Match(TokenType::NEQ)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseRelExp());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseLAndExp() {
    auto node = make_unique<ASTNode>("LAndExp");
    node->AddChild(ParseEqExp());
    while (Match(TokenType::AND)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseEqExp());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseLOrExp() {
    auto node = make_unique<ASTNode>("LOrExp");
    node->AddChild(ParseLAndExp());
    while (Match(TokenType::OR)) {
        node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseLAndExp());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseConstExp() {
    auto node = make_unique<ASTNode>("ConstExp");
    node->AddChild(ParseAddExp());
    return node;
}
