#include "Parser.hpp"
#include "../ast/Node.hpp"
#include "../lexer/TokenType.hpp"
#include "../ast/AST.hpp"
#include "../parser/ParserOutput.hpp"
#include "../../error/ErrorRecorder.hpp"
#include "../../error/Error.hpp"
#include "../../error/ErrorType.hpp"
#include <functional>

Parser* parser_current = nullptr;

Parser::Parser() : tokenStream(std::vector<Token>()), rootNode(nullptr) {}

void Parser::SetCurrent(Parser* p) { parser_current = p; }
Parser* Parser::GetCurrent() { return parser_current; }

void Parser::SetTokenStream(TokenStream stream) {
    this->tokenStream = std::move(stream);
}

Token Parser::peekToken(size_t k) { return this->tokenStream.Peek(k); }
Token Parser::readToken() { Token t = this->tokenStream.Peek(0); this->tokenStream.Read(); return t; }
SPNode Parser::makeTokNode(const Token &t) { return ASTNode::MakeToken(t); }

int Parser::lastTokenLine(const SPNode &n) {
    if (!n) return -1;
    if (n->token) return n->token->line;
    for (auto it = n->children.rbegin(); it != n->children.rend(); ++it) {
        int l = lastTokenLine(*it);
        if (l != -1) return l;
    }
    return -1;
}

// Parsing expression helpers (converted from CompUnit.cpp)
SPNode Parser::parsePrimaryExp() {
    Token t = peekToken();
    if (t.type == TokenType::LPARENT) {
        auto prim = ASTNode::Make("PrimaryExp");
        prim->children.push_back(makeTokNode(readToken())); // LPARENT
        auto expnode = parseExp();
        prim->children.push_back(expnode);
        if (peekToken().type == TokenType::RPARENT) prim->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(expnode);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_RPAREN, errLine, "missing )"));
        }
        return prim;
    }
    if (t.type == TokenType::IDENFR) {
        Token id = readToken();
        auto lv = ASTNode::Make("LVal");
        lv->children.push_back(makeTokNode(id));
        if (peekToken().type == TokenType::LBRACK) {
            lv->children.push_back(makeTokNode(readToken()));
            auto idx = parseExp();
            lv->children.push_back(idx);
            if (peekToken().type == TokenType::RBRACK) lv->children.push_back(makeTokNode(readToken()));
            else {
                int errLine = lastTokenLine(lv);
                if (errLine == -1) errLine = peekToken().line;
                ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ]"));
            }
        }
        auto prim = ASTNode::Make("PrimaryExp");
        prim->children.push_back(lv);
        return prim;
    }
    if (t.type == TokenType::INTCON) {
        Token n = readToken();
        auto num = ASTNode::Make("Number");
        num->children.push_back(makeTokNode(n));
        auto prim = ASTNode::Make("PrimaryExp");
        prim->children.push_back(num);
        return prim;
    }
    auto tok = makeTokNode(readToken());
    auto prim = ASTNode::Make("PrimaryExp");
    prim->children.push_back(tok);
    return prim;
}

SPNode Parser::parseUnaryExp() {
    Token t = peekToken();
    if (t.type == TokenType::PLUS || t.type == TokenType::MINU || t.type == TokenType::NOT) {
        Token op = readToken();
        auto node = ASTNode::Make("UnaryExp");
        node->children.push_back(makeTokNode(op));
        node->children.push_back(parseUnaryExp());
        return node;
    }
    if (t.type == TokenType::IDENFR && peekToken(1).type == TokenType::LPARENT) {
        Token id = readToken();
        auto call = ASTNode::Make("UnaryExp");
        call->children.push_back(makeTokNode(id));
        if (peekToken().type == TokenType::LPARENT) call->children.push_back(makeTokNode(readToken()));
        if (peekToken().type != TokenType::RPARENT) {
            call->children.push_back(parseFuncRParams());
        }
        if (peekToken().type == TokenType::RPARENT) call->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(call);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_RPAREN, errLine, "missing ) in call"));
        }
        return call;
    }
    auto node = ASTNode::Make("UnaryExp");
    node->children.push_back(parsePrimaryExp());
    return node;
}

SPNode Parser::parseFuncRParams() {
    // FuncRParams -> Exp { ',' Exp }
    auto frp = ASTNode::Make("FuncRParams");
    while (true) {
        auto expnode = parseExp();
        frp->children.push_back(expnode);
        if (peekToken().type == TokenType::COMMA) { frp->children.push_back(makeTokNode(readToken())); continue; }
        break;
    }
    return frp;
}

SPNode Parser::parseExp() {
    auto n = ASTNode::Make("Exp");
    n->children.push_back(parseAddExp());
    return n;
}

SPNode Parser::parseCond() {
    auto n = ASTNode::Make("Cond");
    n->children.push_back(parseLOrExp());
    return n;
}

SPNode Parser::parseLVal() {
    auto lv = ASTNode::Make("LVal");
    if (peekToken().type == TokenType::IDENFR) lv->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::LBRACK) {
        lv->children.push_back(makeTokNode(readToken()));
        auto idx = parseExp();
        lv->children.push_back(idx);
        if (peekToken().type == TokenType::RBRACK) lv->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(lv);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ]"));
        }
    }
    return lv;
}

SPNode Parser::parseMulExp() {
    auto left = parseUnaryExp();
    auto node = ASTNode::Make("MulExp");
    node->children.push_back(left);
    while (peekToken().type == TokenType::MULT || peekToken().type == TokenType::DIV || peekToken().type == TokenType::MOD) {
        Token op = readToken();
        auto newNode = ASTNode::Make("MulExp");
        newNode->children.push_back(node);
        newNode->children.push_back(makeTokNode(op));
        newNode->children.push_back(parseUnaryExp());
        node = newNode;
    }
    return node;
}

SPNode Parser::parseAddExp() {
    auto left = parseMulExp();
    auto node = ASTNode::Make("AddExp");
    node->children.push_back(left);
    while (peekToken().type == TokenType::PLUS || peekToken().type == TokenType::MINU) {
        Token op = readToken();
        auto newNode = ASTNode::Make("AddExp");
        newNode->children.push_back(node);
        newNode->children.push_back(makeTokNode(op));
        newNode->children.push_back(parseMulExp());
        node = newNode;
    }
    return node;
}

SPNode Parser::parseRelExp() {
    auto left = parseAddExp();
    auto node = ASTNode::Make("RelExp");
    node->children.push_back(left);
    while (peekToken().type == TokenType::LSS || peekToken().type == TokenType::GRE || peekToken().type == TokenType::LEQ || peekToken().type == TokenType::GEQ) {
        Token op = readToken();
        auto newNode = ASTNode::Make("RelExp");
        newNode->children.push_back(node);
        newNode->children.push_back(makeTokNode(op));
        newNode->children.push_back(parseAddExp());
        node = newNode;
    }
    return node;
}

SPNode Parser::parseEqExp() {
    auto left = parseRelExp();
    auto node = ASTNode::Make("EqExp");
    node->children.push_back(left);
    while (peekToken().type == TokenType::EQL || peekToken().type == TokenType::NEQ) {
        Token op = readToken();
        auto newNode = ASTNode::Make("EqExp");
        newNode->children.push_back(node);
        newNode->children.push_back(makeTokNode(op));
        newNode->children.push_back(parseRelExp());
        node = newNode;
    }
    return node;
}

SPNode Parser::parseLAndExp() {
    auto left = parseEqExp();
    auto node = ASTNode::Make("LAndExp");
    node->children.push_back(left);
    while (peekToken().type == TokenType::AND) {
        Token op = readToken();
        auto newNode = ASTNode::Make("LAndExp");
        newNode->children.push_back(node);
        newNode->children.push_back(makeTokNode(op));
        newNode->children.push_back(parseEqExp());
        node = newNode;
    }
    return node;
}

SPNode Parser::parseLOrExp() {
    auto left = parseLAndExp();
    auto node = ASTNode::Make("LOrExp");
    node->children.push_back(left);
    while (peekToken().type == TokenType::OR) {
        Token op = readToken();
        auto newNode = ASTNode::Make("LOrExp");
        newNode->children.push_back(node);
        newNode->children.push_back(makeTokNode(op));
        newNode->children.push_back(parseLAndExp());
        node = newNode;
    }
    return node;
}

SPNode Parser::parseStmtNode() {
    Token t = peekToken();
    if (t.type == TokenType::FORTK) {
        auto node = ASTNode::Make("Stmt");
        node->children.push_back(makeTokNode(readToken())); // for
        if (peekToken().type == TokenType::LPARENT) node->children.push_back(makeTokNode(readToken()));

        if (peekToken().type != TokenType::SEMICN) {
            auto forInit = ASTNode::Make("ForStmt");
            while (peekToken().type != TokenType::SEMICN && peekToken().type != TokenType::EOF_T) {
                auto lv = parseLVal();
                forInit->children.push_back(lv);
                if (peekToken().type == TokenType::ASSIGN) {
                    forInit->children.push_back(makeTokNode(readToken()));
                    auto expnode = parseExp();
                    forInit->children.push_back(expnode);
                }
                if (peekToken().type == TokenType::COMMA) { forInit->children.push_back(makeTokNode(readToken())); continue; }
                break;
            }
            node->children.push_back(forInit);
        }

        if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(node);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_SEMICOLON, errLine, "missing ; in for"));
        }

        if (peekToken().type != TokenType::SEMICN) {
            auto cond = ASTNode::Make("Cond");
            cond->children.push_back(parseLOrExp());
            node->children.push_back(cond);
        }

        if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(node);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_SEMICOLON, errLine, "missing ; in for"));
        }

        if (peekToken().type != TokenType::RPARENT) {
            auto forStep = ASTNode::Make("ForStmt");
            while (peekToken().type != TokenType::RPARENT && peekToken().type != TokenType::EOF_T) {
                auto lv = parseLVal();
                forStep->children.push_back(lv);
                if (peekToken().type == TokenType::ASSIGN) {
                    forStep->children.push_back(makeTokNode(readToken()));
                    auto expnode = parseExp();
                    forStep->children.push_back(expnode);
                }
                if (peekToken().type == TokenType::COMMA) { forStep->children.push_back(makeTokNode(readToken())); continue; }
                break;
            }
            node->children.push_back(forStep);
        }

        if (peekToken().type == TokenType::RPARENT) node->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(node);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_RPAREN, errLine, "missing ) in for"));
        }

        if (peekToken().type == TokenType::LBRACE) node->children.push_back(parseBlockNode());
        else node->children.push_back(parseStmtNode());
        return node;
    }
    if (t.type == TokenType::IFTK) {
        auto node = ASTNode::Make("Stmt");
        node->children.push_back(makeTokNode(readToken())); // if
        if (peekToken().type == TokenType::LPARENT) node->children.push_back(makeTokNode(readToken()));
        if (peekToken().type != TokenType::RPARENT) {
            auto cond = ASTNode::Make("Cond");
            cond->children.push_back(parseLOrExp());
            node->children.push_back(cond);
        }
        if (peekToken().type == TokenType::RPARENT) node->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(node);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_RPAREN, errLine, "missing ) in if"));
        }
        if (peekToken().type == TokenType::LBRACE) node->children.push_back(parseBlockNode());
        else node->children.push_back(parseStmtNode());
        if (peekToken().type == TokenType::ELSETK) {
            node->children.push_back(makeTokNode(readToken()));
            if (peekToken().type == TokenType::LBRACE) node->children.push_back(parseBlockNode());
            else node->children.push_back(parseStmtNode());
        }
        return node;
    }

    if (t.type == TokenType::CONTINUETK) {
        auto node = ASTNode::Make("Stmt");
        node->children.push_back(makeTokNode(readToken()));
        if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
        return node;
    }

    if (t.type == TokenType::BREAKTK) {
        auto node = ASTNode::Make("Stmt");
        node->children.push_back(makeTokNode(readToken()));
        if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
        return node;
    }

    if (t.type == TokenType::IDENFR && peekToken(1).type == TokenType::ASSIGN) {
        auto node = ASTNode::Make("Stmt");
        auto lv = parseLVal();
        node->children.push_back(lv);
        node->children.push_back(makeTokNode(readToken())); // ASSIGN
        {
            auto expnode = parseExp();
            node->children.push_back(expnode);
        }
        if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(node->children.front());
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_SEMICOLON, errLine, "missing ;"));
        }
        return node;
    }
    if (t.type == TokenType::PRINTFTK) {
        auto node = ASTNode::Make("Stmt");
        node->children.push_back(makeTokNode(readToken())); // printf
        if (peekToken().type == TokenType::LPARENT) node->children.push_back(makeTokNode(readToken()));
        while (peekToken().type != TokenType::RPARENT && peekToken().type != TokenType::EOF_T) {
            if (peekToken().type == TokenType::STRCON) node->children.push_back(makeTokNode(readToken()));
            else {
                auto expnode = parseExp();
                node->children.push_back(expnode);
                if (peekToken().type == TokenType::COMMA) node->children.push_back(makeTokNode(readToken()));
            }
        }
        if (peekToken().type == TokenType::RPARENT) node->children.push_back(makeTokNode(readToken()));
        if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
        return node;
    }
    if (t.type == TokenType::RETURNTK) {
        auto node = ASTNode::Make("Stmt");
        node->children.push_back(makeTokNode(readToken()));
        if (peekToken().type != TokenType::SEMICN) {
            node->children.push_back(parseExp());
        }
        if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
        return node;
    }
    if (t.type == TokenType::SEMICN) {
        auto node = ASTNode::Make("Stmt");
        node->children.push_back(makeTokNode(readToken()));
        return node;
    }
    auto node = ASTNode::Make("Stmt");
    node->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
    return node;
}

SPNode Parser::parseInitVal() {
    if (peekToken().type == TokenType::LBRACE) {
        auto node = ASTNode::Make("InitVal");
        node->children.push_back(makeTokNode(readToken()));
        if (peekToken().type != TokenType::RBRACE) {
            while (true) {
                node->children.push_back(parseInitVal());
                if (peekToken().type == TokenType::COMMA) node->children.push_back(makeTokNode(readToken()));
                else break;
            }
        }
        if (peekToken().type == TokenType::RBRACE) node->children.push_back(makeTokNode(readToken()));
        return node;
    } else {
        auto node = ASTNode::Make("InitVal");
        node->children.push_back(parseExp());
        return node;
    }
}

SPNode Parser::parseConstInitVal() {
    if (peekToken().type == TokenType::LBRACE) {
        auto node = ASTNode::Make("ConstInitVal");
        node->children.push_back(makeTokNode(readToken()));
        if (peekToken().type != TokenType::RBRACE) {
            while (true) {
                node->children.push_back(parseConstInitVal());
                if (peekToken().type == TokenType::COMMA) node->children.push_back(makeTokNode(readToken()));
                else break;
            }
        }
        if (peekToken().type == TokenType::RBRACE) node->children.push_back(makeTokNode(readToken()));
        return node;
    } else {
        auto node = ASTNode::Make("ConstInitVal");
        auto ce = ASTNode::Make("ConstExp");
        ce->children.push_back(parseAddExp());
        node->children.push_back(ce);
        return node;
    }
}

SPNode Parser::parseBlockNode() {
    auto block = ASTNode::Make("Block");
    if (peekToken().type == TokenType::LBRACE) block->children.push_back(makeTokNode(readToken()));
    while (peekToken().type != TokenType::RBRACE && peekToken().type != TokenType::EOF_T) {
        Token p = peekToken();
        if (p.type == TokenType::CONSTTK) {
            auto g = ASTNode::Make("ConstDecl");
            g->children.push_back(makeTokNode(readToken())); // const
            if (peekToken().type == TokenType::INTTK) g->children.push_back(makeTokNode(readToken()));
            while (true) {
                auto cdef = ASTNode::Make("ConstDef");
                if (peekToken().type == TokenType::IDENFR) cdef->children.push_back(makeTokNode(readToken()));
                if (peekToken().type == TokenType::LBRACK) {
                    cdef->children.push_back(makeTokNode(readToken()));
                    auto ce = ASTNode::Make("ConstExp");
                    ce->children.push_back(parseAddExp());
                    cdef->children.push_back(ce);
                    if (peekToken().type == TokenType::RBRACK) cdef->children.push_back(makeTokNode(readToken()));
                    else {
                        int errLine = lastTokenLine(cdef);
                        if (errLine == -1) errLine = peekToken().line;
                        ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ] in const def"));
                    }
                }
                if (peekToken().type == TokenType::ASSIGN) {
                    cdef->children.push_back(makeTokNode(readToken()));
                    if (peekToken().type == TokenType::LBRACE) {
                        cdef->children.push_back(parseConstInitVal());
                    } else {
                        auto civ = ASTNode::Make("ConstInitVal");
                        auto ce = ASTNode::Make("ConstExp");
                        ce->children.push_back(parseAddExp());
                        civ->children.push_back(ce);
                        cdef->children.push_back(civ);
                    }
                }
                g->children.push_back(cdef);
                if (peekToken().type == TokenType::COMMA) { g->children.push_back(makeTokNode(readToken())); continue; }
                break;
            }
            if (peekToken().type == TokenType::SEMICN) g->children.push_back(makeTokNode(readToken()));
            else {
                int errLine = g->children.empty() ? peekToken().line : lastTokenLine(g->children.front());
                ErrorRecorder::AddError(Error(ErrorType::MISSING_SEMICOLON, errLine, "missing ; after const decl"));
            }
            block->children.push_back(g);
        } else if (p.type == TokenType::INTTK || p.type == TokenType::STATICTK) {
            auto decl = ASTNode::Make("VarDecl");
            if (peekToken().type == TokenType::STATICTK) decl->children.push_back(makeTokNode(readToken()));
            if (peekToken().type == TokenType::INTTK) decl->children.push_back(makeTokNode(readToken()));
            while (true) {
                auto vardef = ASTNode::Make("VarDef");
                if (peekToken().type == TokenType::IDENFR) vardef->children.push_back(makeTokNode(readToken()));
                    if (peekToken().type == TokenType::LBRACK) {
                    vardef->children.push_back(makeTokNode(readToken()));
                        auto ce = ASTNode::Make("ConstExp");
                        ce->children.push_back(parseAddExp());
                        vardef->children.push_back(ce);
                    if (peekToken().type == TokenType::RBRACK) vardef->children.push_back(makeTokNode(readToken()));
                    else {
                        int errLine = lastTokenLine(vardef);
                        if (errLine == -1) errLine = peekToken().line;
                        ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ] in vardef"));
                    }
                }
                if (peekToken().type == TokenType::ASSIGN) {
                    readToken(); // consume '='
                    vardef->children.push_back(makeTokNode(Token(TokenType::ASSIGN, "=")));
                    vardef->children.push_back(parseInitVal());
                }
                decl->children.push_back(vardef);
                if (peekToken().type == TokenType::COMMA) { decl->children.push_back(makeTokNode(readToken())); continue; }
                break;
            }
            if (peekToken().type == TokenType::SEMICN) decl->children.push_back(makeTokNode(readToken()));
            else {
                int errLine = decl->children.empty() ? peekToken().line : lastTokenLine(decl->children.front());
                ErrorRecorder::AddError(Error(ErrorType::MISSING_SEMICOLON, errLine, "missing ; after decl"));
            }
            block->children.push_back(decl);
        } else {
            auto stmt = parseStmtNode();
            if (stmt && stmt->name != "Stmt") {
                auto wrap = ASTNode::Make("Stmt");
                wrap->children.push_back(stmt);
                block->children.push_back(wrap);
                for (auto &c : wrap->children) {
                    if (c && c->name == "ForStmt") {
                        block->children.push_back(ASTNode::Make("Stmt"));
                        break;
                    }
                }
            } else {
                block->children.push_back(stmt);
                if (stmt) {
                    for (auto &c : stmt->children) {
                        if (c && c->name == "ForStmt") {
                            block->children.push_back(ASTNode::Make("Stmt"));
                            break;
                        }
                    }
                }
            }
        }
    }
    if (peekToken().type == TokenType::RBRACE) block->children.push_back(makeTokNode(readToken()));
    return block;
}

// Declaration-related parsers generated from grammar provided by user
SPNode Parser::parseBType() {
    // BType -> 'int'
    auto node = ASTNode::Make("BType");
    if (peekToken().type == TokenType::INTTK) node->children.push_back(makeTokNode(readToken()));
    return node;
}

SPNode Parser::parseConstDef() {
    // ConstDef -> Ident [ '[' ConstExp ']' ] '=' ConstInitVal
    auto cdef = ASTNode::Make("ConstDef");
    if (peekToken().type == TokenType::IDENFR) cdef->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::LBRACK) {
        cdef->children.push_back(makeTokNode(readToken()));
        auto ce = ASTNode::Make("ConstExp");
        ce->children.push_back(parseAddExp());
        cdef->children.push_back(ce);
        if (peekToken().type == TokenType::RBRACK) cdef->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(cdef);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ] in const def"));
        }
    }
    if (peekToken().type == TokenType::ASSIGN) {
        cdef->children.push_back(makeTokNode(readToken()));
        cdef->children.push_back(parseConstInitVal());
    }
    return cdef;
}

SPNode Parser::parseConstDecl() {
    // ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';'
    auto node = ASTNode::Make("ConstDecl");
    if (peekToken().type == TokenType::CONSTTK) node->children.push_back(makeTokNode(readToken()));
    node->children.push_back(parseBType());
    while (true) {
        node->children.push_back(parseConstDef());
        if (peekToken().type == TokenType::COMMA) { node->children.push_back(makeTokNode(readToken())); continue; }
        break;
    }
    if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
    else {
        int errLine = node->children.empty() ? peekToken().line : lastTokenLine(node->children.front());
        ErrorRecorder::AddError(Error(ErrorType::MISSING_SEMICOLON, errLine, "missing ; after const decl"));
    }
    return node;
}

SPNode Parser::parseVarDef() {
    // VarDef -> Ident [ '[' ConstExp ']' ] [ '=' InitVal ]
    auto vardef = ASTNode::Make("VarDef");
    if (peekToken().type == TokenType::IDENFR) vardef->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::LBRACK) {
        vardef->children.push_back(makeTokNode(readToken()));
        auto ce = ASTNode::Make("ConstExp");
        ce->children.push_back(parseAddExp());
        vardef->children.push_back(ce);
        if (peekToken().type == TokenType::RBRACK) vardef->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(vardef);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ] in vardef"));
        }
    }
    if (peekToken().type == TokenType::ASSIGN) {
        vardef->children.push_back(makeTokNode(readToken()));
        vardef->children.push_back(parseInitVal());
    }
    return vardef;
}

SPNode Parser::parseVarDecl() {
    // VarDecl -> [ 'static' ] BType VarDef { ',' VarDef } ';'
    auto node = ASTNode::Make("VarDecl");
    if (peekToken().type == TokenType::STATICTK) node->children.push_back(makeTokNode(readToken()));
    node->children.push_back(parseBType());
    while (true) {
        node->children.push_back(parseVarDef());
        if (peekToken().type == TokenType::COMMA) { node->children.push_back(makeTokNode(readToken())); continue; }
        break;
    }
    if (peekToken().type == TokenType::SEMICN) node->children.push_back(makeTokNode(readToken()));
    else {
        int errLine = node->children.empty() ? peekToken().line : lastTokenLine(node->children.front());
        ErrorRecorder::AddError(Error(ErrorType::MISSING_SEMICOLON, errLine, "missing ; after decl"));
    }
    return node;
}

SPNode Parser::parseDecl() {
    // Decl -> ConstDecl | VarDecl
    if (peekToken().type == TokenType::CONSTTK) return parseConstDecl();
    return parseVarDecl();
}

SPNode Parser::parseFuncType() {
    auto ft = ASTNode::Make("FuncType");
    if (peekToken().type == TokenType::VOIDTK || peekToken().type == TokenType::INTTK) ft->children.push_back(makeTokNode(readToken()));
    return ft;
}

SPNode Parser::parseFuncFParam() {
    // FuncFParam -> BType Ident ['[' ']']
    auto fp = ASTNode::Make("FuncFParam");
    fp->children.push_back(parseBType());
    if (peekToken().type == TokenType::IDENFR) fp->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::LBRACK) {
        fp->children.push_back(makeTokNode(readToken()));
        if (peekToken().type == TokenType::RBRACK) fp->children.push_back(makeTokNode(readToken()));
        else {
            int errLine = lastTokenLine(fp);
            if (errLine == -1) errLine = peekToken().line;
            ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ] in func param"));
        }
    }
    return fp;
}

SPNode Parser::parseFuncFParams() {
    auto fps = ASTNode::Make("FuncFParams");
    while (peekToken().type != TokenType::RPARENT && peekToken().type != TokenType::EOF_T) {
        fps->children.push_back(parseFuncFParam());
        if (peekToken().type == TokenType::COMMA) fps->children.push_back(makeTokNode(readToken()));
        else break;
    }
    return fps;
}

SPNode Parser::parseFuncDef() {
    auto f = ASTNode::Make("FuncDef");
    f->children.push_back(parseFuncType());
    if (peekToken().type == TokenType::IDENFR) f->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::LPARENT) f->children.push_back(makeTokNode(readToken()));
    if (peekToken().type != TokenType::RPARENT) {
        f->children.push_back(parseFuncFParams());
    }
    if (peekToken().type == TokenType::RPARENT) f->children.push_back(makeTokNode(readToken()));
    else {
        int errLine = lastTokenLine(f);
        if (errLine == -1) errLine = peekToken().line;
        ErrorRecorder::AddError(Error(ErrorType::MISSING_RPAREN, errLine, "missing ) in funcdef"));
    }
    f->children.push_back(parseBlockNode());
    return f;
}

SPNode Parser::parseMainFuncDef() {
    auto mainNode = ASTNode::Make("MainFuncDef");
    if (peekToken().type == TokenType::INTTK) mainNode->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::MAINTK) mainNode->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::LPARENT) mainNode->children.push_back(makeTokNode(readToken()));
    if (peekToken().type == TokenType::RPARENT) mainNode->children.push_back(makeTokNode(readToken()));
    else {
        int errLine = lastTokenLine(mainNode);
        if (errLine == -1) errLine = peekToken().line;
        ErrorRecorder::AddError(Error(ErrorType::MISSING_RPAREN, errLine, "missing ) in main"));
    }
    mainNode->children.push_back(parseBlockNode());
    return mainNode;
}

// Top-level: build CompUnit AST and output
void Parser::ParseCompUnit() {
    auto root = ASTNode::Make("CompUnit");
    // parse top-level declarations and functions
    while (peekToken().type != TokenType::MAINTK && peekToken().type != TokenType::EOF_T) {
        if (peekToken().type == TokenType::INTTK || peekToken().type == TokenType::VOIDTK) {
            if (peekToken(1).type == TokenType::IDENFR && peekToken(2).type == TokenType::LPARENT) {
                root->children.push_back(parseFuncDef());
            } else {
                auto g = ASTNode::Make("VarDecl");
                if (peekToken().type == TokenType::INTTK) g->children.push_back(makeTokNode(readToken()));
                while (peekToken().type != TokenType::SEMICN && peekToken().type != TokenType::EOF_T) {
                    if (peekToken().type == TokenType::IDENFR) {
                        auto vardef = ASTNode::Make("VarDef");
                        vardef->children.push_back(makeTokNode(readToken()));
                        if (peekToken().type == TokenType::LBRACK) {
                            vardef->children.push_back(makeTokNode(readToken()));
                            auto ce = ASTNode::Make("ConstExp");
                            ce->children.push_back(parseAddExp());
                            vardef->children.push_back(ce);
                            if (peekToken().type == TokenType::RBRACK) vardef->children.push_back(makeTokNode(readToken()));
                            else {
                                int errLine = lastTokenLine(vardef);
                                if (errLine == -1) errLine = peekToken().line;
                                ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ] in global vardef"));
                            }
                        }
                        if (peekToken().type == TokenType::ASSIGN) {
                            readToken();
                            vardef->children.push_back(makeTokNode(Token(TokenType::ASSIGN, "=")));
                            vardef->children.push_back(parseInitVal());
                        }
                        g->children.push_back(vardef);
                    } else {
                        g->children.push_back(makeTokNode(readToken()));
                    }
                }
                if (peekToken().type == TokenType::SEMICN) g->children.push_back(makeTokNode(readToken()));
                root->children.push_back(g);
            }
        } else if (peekToken().type == TokenType::CONSTTK) {
            auto g = ASTNode::Make("ConstDecl");
            g->children.push_back(makeTokNode(readToken()));
            if (peekToken().type == TokenType::INTTK) g->children.push_back(makeTokNode(readToken()));
            while (true) {
                auto cdef = ASTNode::Make("ConstDef");
                if (peekToken().type == TokenType::IDENFR) cdef->children.push_back(makeTokNode(readToken()));
                if (peekToken().type == TokenType::LBRACK) {
                    cdef->children.push_back(makeTokNode(readToken()));
                    auto ce2 = ASTNode::Make("ConstExp");
                    ce2->children.push_back(parseAddExp());
                    cdef->children.push_back(ce2);
                    if (peekToken().type == TokenType::RBRACK) cdef->children.push_back(makeTokNode(readToken()));
                    else {
                        int errLine = lastTokenLine(cdef);
                        if (errLine == -1) errLine = peekToken().line;
                        ErrorRecorder::AddError(Error(ErrorType::MISSING_RBRACK, errLine, "missing ] in const def"));
                    }
                }
                if (peekToken().type == TokenType::ASSIGN) {
                    cdef->children.push_back(makeTokNode(readToken()));
                    if (peekToken().type == TokenType::LBRACE) {
                        cdef->children.push_back(parseConstInitVal());
                    } else {
                        auto civ = ASTNode::Make("ConstInitVal");
                        auto ce = ASTNode::Make("ConstExp");
                        ce->children.push_back(parseAddExp());
                        civ->children.push_back(ce);
                        cdef->children.push_back(civ);
                    }
                }
                g->children.push_back(cdef);
                if (peekToken().type == TokenType::COMMA) { g->children.push_back(makeTokNode(readToken())); continue; }
                break;
            }
            if (peekToken().type == TokenType::SEMICN) g->children.push_back(makeTokNode(readToken()));
            root->children.push_back(g);
        } else if (peekToken().type == TokenType::STATICTK) {
            auto g = ASTNode::Make("Decl");
            while (peekToken().type != TokenType::SEMICN && peekToken().type != TokenType::EOF_T) g->children.push_back(makeTokNode(readToken()));
            if (peekToken().type == TokenType::SEMICN) g->children.push_back(makeTokNode(readToken()));
            root->children.push_back(g);
        } else {
            root->children.push_back(makeTokNode(readToken()));
        }
    }
    if (peekToken().type == TokenType::INTTK && peekToken(1).type == TokenType::MAINTK) {
        root->children.push_back(parseMainFuncDef());
    }

    // Post-process and write output
    for (size_t i = 0; i < root->children.size(); ++i) {
        auto &child = root->children[i];
        if (child && child->name == "FuncDef") {
            bool isMain = false;
            for (auto &cc : child->children) {
                if (cc && cc->token && cc->token->type == TokenType::MAINTK) { isMain = true; break; }
            }
            if (isMain) {
                auto mainNode = ASTNode::Make("MainFuncDef");
                for (auto &cc : child->children) mainNode->children.push_back(cc);
                root->children.erase(root->children.begin() + i);
                root->children.push_back(mainNode);
                break;
            }
        }
    }
    for (auto &c : root->children) TraversePreOrder(c);
    ParserOutput::Get().Write("<CompUnit>");
}

void Parser::GenerateAstTree() {
    Node::SetTokenStream(&this->tokenStream);
    this->rootNode = new CompUnit();
    // set current parser pointer so CompUnit::Parse can call back
    Parser::SetCurrent(this);
    this->rootNode->Parse();
    Parser::SetCurrent(nullptr);
}

CompUnit* Parser::GetAstTree() { return this->rootNode; }
