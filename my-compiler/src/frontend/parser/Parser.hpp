#pragma once

#include "../lexer/TokenStream.hpp"
#include "../ast/CompUnit.hpp"
#include "../ast/AST.hpp"
#include "../parser/ParserOutput.hpp"
#include "../../error/ErrorRecorder.hpp"
#include "../../error/Error.hpp"
#include "../../error/ErrorType.hpp"
#include <memory>
#include <vector>

using SPNode = std::shared_ptr<ASTNode>;

// Forward declare Token for signatures
struct Token;

class Parser {
public:
    Parser();

    void SetTokenStream(TokenStream stream);
    void GenerateAstTree();
    CompUnit* GetAstTree();

    // expose a singleton-like pointer so CompUnit::Parse() can call back
    static void SetCurrent(Parser* p);
    static Parser* GetCurrent();

    // top-level parse entry used by CompUnit::Parse wrapper
    void ParseCompUnit();

private:
    TokenStream tokenStream;
    CompUnit* rootNode;

    // helper token functions
    Token peekToken(size_t k = 0);
    Token readToken();
    SPNode makeTokNode(const Token &t);
    int lastTokenLine(const SPNode &n);

    // parsing subroutines (recursive descent)
    SPNode parsePrimaryExp();
    SPNode parseUnaryExp();
    SPNode parseMulExp();
    SPNode parseAddExp();
    SPNode parseRelExp();
    SPNode parseEqExp();
    SPNode parseLAndExp();
    SPNode parseLOrExp();
    SPNode parseExp();
    SPNode parseCond();
    SPNode parseLVal();
    SPNode parseFuncRParams();
    SPNode parseInitVal();
    SPNode parseConstInitVal();
    SPNode parseBlockNode();
    SPNode parseStmtNode();
    // declaration related parsers
    SPNode parseDecl();
    SPNode parseConstDecl();
    SPNode parseBType();
    SPNode parseConstDef();
    SPNode parseVarDecl();
    SPNode parseVarDef();
    // function related
    SPNode parseFuncType();
    SPNode parseFuncFParam();
    SPNode parseFuncFParams();
    SPNode parseFuncDef();
    SPNode parseMainFuncDef();
};
