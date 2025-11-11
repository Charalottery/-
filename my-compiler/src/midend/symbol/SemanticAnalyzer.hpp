#pragma once
#include "SymbolTable.hpp"
#include "../../frontend/ast/AST.hpp"
#include <vector>
#include <string>

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    // analyze AST and populate symbol tables
    void Analyze(const ASTNode *root);

    // access results
    const std::vector<SymbolTable>& GetTables() const { return tables; }

    // lookup symbol by name starting from current scope outward; returns nullptr if not found
    Symbol* Lookup(const std::string &name);

    // toggle symbol emission
    void SetEmitSymbol(bool v) { emitSymbol = v; }
    bool EmitSymbol() const { return emitSymbol; }

private:
    std::vector<SymbolTable> tables;
    int currentTable = -1; // index into tables
    bool emitSymbol = true;

    int NewScope();
    void ExitScope();

    void Walk(const ASTNode *node);

    // node-specific handlers
    void HandleCompUnit(const ASTNode *node);
    void HandleDecl(const ASTNode *node);
    void HandleConstDecl(const ASTNode *node);
    void HandleVarDecl(const ASTNode *node);
    void HandleFuncDef(const ASTNode *node, bool isMain=false);
    void HandleBlock(const ASTNode *node);
    void HandleFuncFParams(const ASTNode *node);
    void HandleFuncFParam(const ASTNode *node);
};
