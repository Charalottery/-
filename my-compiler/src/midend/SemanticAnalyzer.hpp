#pragma once
#include "../frontend/ast/AST.hpp"
#include <string>

namespace midend {
class SemanticAnalyzer {
public:
    // enable/disable dumping symbol.txt
    static void SetDumpSymbols(bool v) { dumpSymbols = v; }
    static void Analyze(const ASTNode* root);

private:
    static bool dumpSymbols;
    static void ProcessCompUnit(const ASTNode* node);
    static void ProcessNodeRec(const ASTNode* node, bool createScopeForBlock = true);
    static std::string GetBType(const ASTNode* node);
    static std::string GetIdent(const ASTNode* node);
    static int CountToken(const ASTNode* node, TokenType t);
    static bool HasToken(const ASTNode* node, TokenType t);
    static void DumpSymbolFile();
};
}
