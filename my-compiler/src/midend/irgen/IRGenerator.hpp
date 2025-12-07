#pragma once
#include "../../frontend/ast/AST.hpp"
#include "../llvm/IrModule.hpp"
#include "../llvm/IrBuilder.hpp"
#include "../symbol/SymbolTable.hpp"
#include <memory>
#include <stack>

class IRGenerator {
public:
    IrModule* module;
    SymbolTable* currentSymbolTable; // We might need to traverse symbol tables or use the one from semantic analysis
    std::stack<std::pair<IrBasicBlock*, IrBasicBlock*>> loopStack; // <condBlock, nextBlock> for continue/break
    int tmpCounter = 0;

    IRGenerator(ASTNode* root, SymbolTable* rootTable);

    void generate();
    std::string getNewName(std::string prefix = "tmp");

private:
    ASTNode* root;

    void visitCompUnit(ASTNode* node);
    void visitDecl(ASTNode* node);
    void visitConstDecl(ASTNode* node);
    void visitVarDecl(ASTNode* node);
    void visitFuncDef(ASTNode* node);
    void visitMainFuncDef(ASTNode* node);
    void visitBlock(ASTNode* node, bool createScope = true);
    void visitStmt(ASTNode* node);
    
    IrValue* visitExp(ASTNode* node);
    IrValue* visitLVal(ASTNode* node, bool isLeft = false); // isLeft=true means we want the address
    IrValue* visitPrimaryExp(ASTNode* node);
    IrValue* visitUnaryExp(ASTNode* node);
    IrValue* visitMulExp(ASTNode* node);
    IrValue* visitAddExp(ASTNode* node);
    IrValue* visitRelExp(ASTNode* node);
    IrValue* visitEqExp(ASTNode* node);
    void visitLAndExp(ASTNode* node, IrBasicBlock* trueBlock, IrBasicBlock* falseBlock);
    void visitLOrExp(ASTNode* node, IrBasicBlock* trueBlock, IrBasicBlock* falseBlock);
    void visitCond(ASTNode* node, IrBasicBlock* trueBlock, IrBasicBlock* falseBlock);
    void visitForStmt(ASTNode* node);

    void getGlobalInitVals(ASTNode* initVal, std::vector<IrConstant*>& vals);
    void getLocalInitVals(ASTNode* initVal, std::vector<ASTNode*>& exprs);

    void initLibraryFunctions();

    // Helper to find symbol in current or parent scopes
    Symbol* findSymbol(const std::string& name);

    int evaluateConstExp(ASTNode* node);
    
    std::string currentFunctionName;
    void enterScope();
    void exitScope();
};
