#pragma once
#include "SymbolTable.hpp"
#include "../../frontend/ast/AST.hpp"
#include <vector>
#include <string>
#include <unordered_map>

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
    // (emit symbol control removed; symbol emission is decided by caller)

private:
    std::vector<SymbolTable> tables;
    int currentTable = -1; // index into tables
    // emission control removed

    // mapping from AST Block (or function body) nodes to created symbol table index
    std::unordered_map<const ASTNode*, int> nodeScopeMap;
    // map table id -> index in tables vector for fast parent lookup
    std::unordered_map<int,int> tableIdIndexMap;

    int NewScope();
    void ExitScope();

    void Walk(const ASTNode *node);

    // Two-pass analysis helpers
    void DeclPass(const ASTNode *node);
    void CheckPass(const ASTNode *node);
    void CheckStmt(const ASTNode *node);

    // statement and expression handlers for semantic checks
    void HandleStmt(const ASTNode *node);
    void HandleExp(const ASTNode *node);
    void HandleUnaryExp(const ASTNode *node);
    void HandleLValUse(const ASTNode *node);

    // state tracking
    int currentFunctionRet = -1; // -1 = none, 0 = void, 1 = int
    bool currentFunctionHasReturn = false;
    int loopDepth = 0;

    // node-specific handlers
    void HandleCompUnit(const ASTNode *node);
    void HandleDecl(const ASTNode *node);
    void HandleConstDecl(const ASTNode *node);
    void HandleVarDecl(const ASTNode *node);
    void HandleFuncDef(const ASTNode *node, bool isMain=false);
    void HandleBlock(const ASTNode *node, const std::vector<Symbol>* preInsert = nullptr);
    // helper: search a subtree for nested Block nodes and handle them
    void WalkForBlocks(const ASTNode *node);
    // build parameter symbols from FuncFParams node (does not set tableId)
    std::vector<Symbol> BuildParamSymbols(const ASTNode *paramsNode);
    void HandleFuncFParams(const ASTNode *node);
    void HandleFuncFParam(const ASTNode *node);
};
