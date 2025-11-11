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
    // Enforce grammar: {Decl} {FuncDef} MainFuncDef
    // First parse zero-or-more declarations
    while (true) {
        Token cur = PeekToken(0);
        Token pre1 = PeekToken(1);
        Token pre2 = PeekToken(2);

        if (cur.type == TokenType::EOF_T) break;

        // declarations start with 'const' or 'static'
        if (cur.type == TokenType::CONSTTK || cur.type == TokenType::STATICTK) {
            node->AddChild(ParseDecl());
            continue;
        }

        // an 'int' followed by an identifier and then one of [ '[', '=', ',', ';' ] is a VarDecl
        if (cur.type == TokenType::INTTK && pre1.type == TokenType::IDENFR &&
            (pre2.type == TokenType::LBRACK || pre2.type == TokenType::ASSIGN || pre2.type == TokenType::COMMA || pre2.type == TokenType::SEMICN)) {
            node->AddChild(ParseDecl());
            continue;
        }

        // otherwise stop scanning declarations
        break;
    }

    // Then parse zero-or-more function definitions
    while (true) {
        Token cur = PeekToken(0);
        Token pre1 = PeekToken(1);
        Token pre2 = PeekToken(2);

        if (cur.type == TokenType::EOF_T) break;

        // main function start: stop here and parse MainFuncDef later
        if (cur.type == TokenType::INTTK && pre1.type == TokenType::MAINTK && pre2.type == TokenType::LPARENT) {
            break;
        }

        // 'void' always begins a function definition
        if (cur.type == TokenType::VOIDTK) {
            node->AddChild(ParseFuncDef());
            continue;
        }

        // 'int' identifier '(' indicates a function definition
        if (cur.type == TokenType::INTTK && pre1.type == TokenType::IDENFR && pre2.type == TokenType::LPARENT) {
            node->AddChild(ParseFuncDef());
            continue;
        }

        // otherwise stop scanning function defs
        break;
    }

    // Finally parse main function (per grammar this must appear)
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
        // Grammar: '{' [ Exp { ',' Exp } ] '}'
        if (PeekToken(0).type != TokenType::RBRACE) {
            node->AddChild(ParseExp());
            while (Match(TokenType::COMMA)) {
                node->AddChild(MakeTokenNode(Consume()));
                node->AddChild(ParseExp());
            }
        }
        if (Match(TokenType::RBRACE)) node->AddChild(MakeTokenNode(Consume()));
    } else {
        // parse as a full expression so we build proper expression subtree
        node->AddChild(ParseExp());
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
    // use lookahead tokens to disambiguate possible stmt starts without backtracking
    Token tk = PeekToken(0);
    Token pre1 = PeekToken(1);
    Token pre2 = PeekToken(2);
    Token pre3 = PeekToken(3);

    // Block
    if (tk.type == TokenType::LBRACE) {
        node->AddChild(ParseBlock());
        return node;
    }

    // if ( Cond ) Stmt [ else Stmt ]
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

    // for ( [ForStmt] ; [Cond] ; [ForStmt] ) Stmt
    if (tk.type == TokenType::FORTK) {
        node->AddChild(MakeTokenNode(Consume())); // for
        if (Match(TokenType::LPARENT)) node->AddChild(MakeTokenNode(Consume()));
        if (PeekToken(0).type != TokenType::SEMICN) node->AddChild(ParseForStmt());
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        if (PeekToken(0).type != TokenType::SEMICN) node->AddChild(ParseCond());
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        if (PeekToken(0).type != TokenType::RPARENT) node->AddChild(ParseForStmt());
        if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
        node->AddChild(ParseStmt());
        return node;
    }

    // break ;
    if (tk.type == TokenType::BREAKTK) {
        node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }

    // continue ;
    if (tk.type == TokenType::CONTINUETK) {
        node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }

    // return [Exp] ;
    if (tk.type == TokenType::RETURNTK) {
        node->AddChild(MakeTokenNode(Consume()));
        if (PeekToken(0).type != TokenType::SEMICN) node->AddChild(ParseExp());
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }

    // printf ( FormatString , Exp ) ;  (printf starts with PRINTFTK)
    if (tk.type == TokenType::PRINTFTK) {
        node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::LPARENT)) node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::STRCON)) node->AddChild(MakeTokenNode(Consume()));
        // the grammar requires exactly one format string then a comma and an Exp, but keep existing support for additional args
        while (Match(TokenType::COMMA)) {
            node->AddChild(MakeTokenNode(Consume()));
            node->AddChild(ParseExp());
        }
        if (Match(TokenType::RPARENT)) node->AddChild(MakeTokenNode(Consume()));
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }

    // Handle statements starting with identifier (could be assignment, getint assignment, function-call expression, or general expression)
    if (tk.type == TokenType::IDENFR) {
        // If identifier is followed by '[' or '=' => it's definitely a LVal start (so assignment form)
        if (pre1.type == TokenType::LBRACK || pre1.type == TokenType::ASSIGN) {
            // LVal '=' Exp ';' or LVal '=' getint() ';'
            node->AddChild(ParseLVal());
            if (Match(TokenType::ASSIGN)) node->AddChild(MakeTokenNode(Consume()));
            // after '=' could be the getint() special form or any expression
            node->AddChild(ParseExp());
            if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
            return node;
        }

        // Otherwise it must be an expression stmt (function call or lval as rvalue etc.)
        node->AddChild(ParseExp());
        if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
        return node;
    }

    // empty statement ';'
    if (tk.type == TokenType::SEMICN) {
        node->AddChild(MakeTokenNode(Consume()));
        return node;
    }

    // fallback: parse as expression statement
    node->AddChild(ParseExp());
    if (Match(TokenType::SEMICN)) node->AddChild(MakeTokenNode(Consume()));
    return node;
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
std::unique_ptr<ASTNode> Parser::ParseExp() {
    auto node = make_unique<ASTNode>("Exp");
    node->AddChild(ParseAddExp());
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseCond() { 
    auto node = make_unique<ASTNode>("Cond");
    node->AddChild(ParseLOrExp());
    return node;
}

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
        // identifier in a PrimaryExp should be treated as an LVal
        // (function calls are handled earlier in UnaryExp when '(' follows)
        node->AddChild(ParseLVal());
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
    // MulExp -> UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
    // Build left-associative nested MulExp nodes so the AST contains
    // an explicit MulExp node for each reduction of the grammar. This
    // matches the grammar's left-recursive form (produces nested nodes
    // rather than a single flattened node).
    auto node = make_unique<ASTNode>("MulExp");
    // initial operand as UnaryExp wrapped in this MulExp
    node->AddChild(ParseUnaryExp());

    // while there are binary mul/div/mod operators, create a new
    // MulExp node where the left child is the previous MulExp and
    // the right child is the next UnaryExp (preserving left-assoc).
    while (Match(TokenType::MULT) || Match(TokenType::DIV) || Match(TokenType::MOD)) {
        auto opNode = MakeTokenNode(Consume());
        auto right = ParseUnaryExp();

        auto newNode = make_unique<ASTNode>("MulExp");
        // move the previous node as the left child
        newNode->AddChild(std::move(node));
        // operator token
        newNode->AddChild(std::move(opNode));
        // right operand
        newNode->AddChild(std::move(right));

        node = std::move(newNode);
    }
    return node;
}


std::unique_ptr<ASTNode> Parser::ParseAddExp() {
    // AddExp -> MulExp | AddExp ('+' | '-') MulExp
    // Build left-associative nested AddExp nodes to match the grammar's
    // left-recursive form and produce explicit <AddExp> nodes for each
    // reduction (so the printer can output them at the correct times).
    auto node = make_unique<ASTNode>("AddExp");
    node->AddChild(ParseMulExp());

    while (Match(TokenType::PLUS) || Match(TokenType::MINU)) {
        auto opNode = MakeTokenNode(Consume());
        auto right = ParseMulExp();

        auto newNode = make_unique<ASTNode>("AddExp");
        newNode->AddChild(std::move(node));
        newNode->AddChild(std::move(opNode));
        newNode->AddChild(std::move(right));

        node = std::move(newNode);
    }
    return node;
}




std::unique_ptr<ASTNode> Parser::ParseRelExp() {
    // RelExp -> AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
    // Build left-associative nested RelExp nodes so each reduction
    // produces an explicit <RelExp> node (matching the grammar).
    auto node = make_unique<ASTNode>("RelExp");
    node->AddChild(ParseAddExp());
    while (Match(TokenType::LSS) || Match(TokenType::GRE) || Match(TokenType::LEQ) || Match(TokenType::GEQ)) {
        auto opNode = MakeTokenNode(Consume());
        auto right = ParseAddExp();

        auto newNode = make_unique<ASTNode>("RelExp");
        newNode->AddChild(std::move(node));
        newNode->AddChild(std::move(opNode));
        newNode->AddChild(std::move(right));

        node = std::move(newNode);
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::ParseEqExp() {
    // EqExp -> RelExp | EqExp ('==' | '!=') RelExp
    auto node = make_unique<ASTNode>("EqExp");
    node->AddChild(ParseRelExp());
    while (Match(TokenType::EQL) || Match(TokenType::NEQ)) {
        auto opNode = MakeTokenNode(Consume());
        auto right = ParseRelExp();

        auto newNode = make_unique<ASTNode>("EqExp");
        newNode->AddChild(std::move(node));
        newNode->AddChild(std::move(opNode));
        newNode->AddChild(std::move(right));

        node = std::move(newNode);
    }
    return node;
}


std::unique_ptr<ASTNode> Parser::ParseLAndExp() {
    // LAndExp -> EqExp | LAndExp '&&' EqExp
    auto node = make_unique<ASTNode>("LAndExp");
    node->AddChild(ParseEqExp());
    while (Match(TokenType::AND)) {
        auto opNode = MakeTokenNode(Consume());
        auto right = ParseEqExp();

        auto newNode = make_unique<ASTNode>("LAndExp");
        newNode->AddChild(std::move(node));
        newNode->AddChild(std::move(opNode));
        newNode->AddChild(std::move(right));

        node = std::move(newNode);
    }
    return node;
}


std::unique_ptr<ASTNode> Parser::ParseLOrExp() {
    // LOrExp -> LAndExp | LOrExp '||' LAndExp
    auto node = make_unique<ASTNode>("LOrExp");
    node->AddChild(ParseLAndExp());
    while (Match(TokenType::OR)) {
        auto opNode = MakeTokenNode(Consume());
        auto right = ParseLAndExp();

        auto newNode = make_unique<ASTNode>("LOrExp");
        newNode->AddChild(std::move(node));
        newNode->AddChild(std::move(opNode));
        newNode->AddChild(std::move(right));

        node = std::move(newNode);
    }
    return node;
}


std::unique_ptr<ASTNode> Parser::ParseConstExp() {
    auto node = make_unique<ASTNode>("ConstExp");
    node->AddChild(ParseAddExp());
    return node;
}
