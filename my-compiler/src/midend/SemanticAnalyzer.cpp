#include "SemanticAnalyzer.hpp"
#include "SymbolManager.hpp"
#include "Symbol.hpp"
#include "../error/ErrorRecorder.hpp"
#include "../error/ErrorType.hpp"
#include "../error/Error.hpp"
#include <fstream>
#include <algorithm>

using namespace midend;

bool SemanticAnalyzer::dumpSymbols = true; // default enable for now

// semantic state
static std::string currentFuncReturnType = "";
static bool currentFuncHasReturn = false;
static int loopDepth = 0;

void SemanticAnalyzer::Analyze(const ASTNode* root) {
    if (!root) return;
    // init symbol manager
    SymbolManager::Init();
    ProcessCompUnit(root);
    if (dumpSymbols) DumpSymbolFile();
}

static int CountImmediateToken(const ASTNode* node, TokenType t) {
    if (!node) return 0;
    int cnt = 0;
    for (const auto &c : node->children) {
        if (c->isToken && c->token.type == t) ++cnt;
    }
    return cnt;
}

static int GetIdentLine(const ASTNode* node) {
    if (!node) return -1;
    if (node->isToken && node->token.type == TokenType::IDENFR) return node->token.line;
    for (const auto &c : node->children) {
        int r = GetIdentLine(c.get());
        if (r >= 0) return r;
    }
    return -1;
}

static std::string FindIdent(const ASTNode* node) {
    if (!node) return std::string();
    if (node->isToken && node->token.type == TokenType::IDENFR) return node->token.value;
    for (const auto &c : node->children) {
        std::string r = FindIdent(c.get());
        if (!r.empty()) return r;
    }
    return std::string();
}

// determine whether expression node denotes an array variable being passed
static bool ExprIsArray(const ASTNode* node) {
    if (!node) return false;
    if (node->name == "LVal") {
        if (CountImmediateToken(node, TokenType::LBRACK) > 0) return false; // element access -> int
        std::string id;
        if ((id = FindIdent(node)).size() > 0) {
            Symbol* s = SymbolManager::Lookup(id);
            if (s && s->typeName.find("Array") != std::string::npos) return true;
        }
        return false;
    }
    for (const auto &c : node->children) if (ExprIsArray(c.get())) return true;
    return false;
}

static int CountFuncRParams(const ASTNode* params) {
    if (!params) return 0;
    int cnt = 0;
    for (const auto &c : params->children) if (c->name == "Exp") ++cnt;
    return cnt;
}

static std::vector<const ASTNode*> GetFuncRParamExprs(const ASTNode* params) {
    std::vector<const ASTNode*> out;
    if (!params) return out;
    for (const auto &c : params->children) if (c->name == "Exp") out.push_back(c.get());
    return out;
}

static int GetBlockRBraceLine(const ASTNode* funcNode) {
    if (!funcNode) return -1;
    for (const auto &c : funcNode->children) {
        if (c->name == "Block") {
            // find last RBRACE token in block
            for (int i = (int)c->children.size() - 1; i >= 0; --i) {
                if (c->children[i]->isToken && c->children[i]->token.type == TokenType::RBRACK) return c->children[i]->token.line;
                if (c->children[i]->isToken && c->children[i]->token.type == TokenType::RBRACE) return c->children[i]->token.line; // fallback
            }
        }
    }
    return -1;
}

void SemanticAnalyzer::ProcessCompUnit(const ASTNode* node) {
    for (const auto &c : node->children) {
        if (!c) continue;
        if (c->name == "Decl") {
            // declarations can appear at global or block level; process children (VarDecl/ConstDecl)
            ProcessNodeRec(c->children.empty() ? nullptr : c->children[0].get());
        } else if (c->name == "FuncDef") {
            // add function symbol to current (global) scope
            // find function name and type
            std::string funcType = GetBType(c.get());
            // find identifier token in this node
            std::string funcName;
            int funcLine = -1;
            for (const auto &ch : c->children) {
                if (ch->isToken && ch->token.type == TokenType::IDENFR) {
                    funcName = ch->token.value;
                    funcLine = ch->token.line;
                    break;
                }
            }
            // collect parameter types
            std::vector<std::string> paramTypes;
            for (const auto &ch : c->children) {
                if (ch->name == "FuncFParams") {
                    for (const auto &p : ch->children) {
                        if (p->name == "FuncFParam") {
                            std::string btype = GetBType(p.get());
                            int dim = CountImmediateToken(p.get(), TokenType::LBRACK);
                            std::string outType = (btype == "int") ? (dim > 0 ? "IntArray" : "Int") : "Int";
                            paramTypes.push_back(outType);
                        }
                    }
                }
            }
            if (!funcName.empty()) {
                // map funcType to output name
                std::string outType = "IntFunc";
                if (funcType == "void") outType = "VoidFunc";
                else if (funcType == "int") outType = "IntFunc";

                Symbol s(funcName, outType, true, paramTypes, funcLine);
                SymbolManager::AddSymbol(s);
            }
            // set current function context
            currentFuncReturnType = funcType;
            currentFuncHasReturn = false;

            // create function inner scope
            SymbolManager::CreateScope();
            // process params and body: traverse children so that params get added in order
            // but when encountering the function's Block node we should NOT create
            // an extra scope because the function scope already represents the
            // function body (matching Java implementation). So pass createScopeForBlock=false
            for (const auto &ch : c->children) ProcessNodeRec(ch.get(), false);
            // leave function scope
            SymbolManager::ExitScope();

            // check missing return for non-void
            if (currentFuncReturnType == "int" && !currentFuncHasReturn) {
                int braceLine = GetBlockRBraceLine(c.get());
                if (braceLine < 0) braceLine = 0;
                ErrorRecorder::AddError(Error(ErrorType::MISSING_RETURN, braceLine));
            }
            currentFuncReturnType.clear();
            currentFuncHasReturn = false;
        } else if (c->name == "MainFuncDef") {
            // main is treated like a function: create main function scope but do not add main to symbol table
            currentFuncReturnType = "int";
            currentFuncHasReturn = false;
            SymbolManager::CreateScope();
            for (const auto &ch : c->children) ProcessNodeRec(ch.get(), false);
            SymbolManager::ExitScope();
            if (!currentFuncHasReturn) {
                int braceLine = GetBlockRBraceLine(c.get());
                if (braceLine < 0) braceLine = 0;
                ErrorRecorder::AddError(Error(ErrorType::MISSING_RETURN, braceLine));
            }
            currentFuncReturnType.clear();
            currentFuncHasReturn = false;
        }
    }
}

void SemanticAnalyzer::ProcessNodeRec(const ASTNode* node, bool createScopeForBlock) {
    if (!node) return;

    // LVal usage: check undefined identifier
    if (node->name == "LVal") {
        std::string id = GetIdent(node);
        if (!id.empty()) {
            Symbol* s = SymbolManager::Lookup(id);
            if (!s) {
                int line = GetIdentLine(node);
                ErrorRecorder::AddError(Error(ErrorType::NAME_UNDEFINED, line));
            }
        }
        return;
    }

    // ForStmt (loop) handling
    if (node->name == "ForStmt") {
        ++loopDepth;
        for (const auto &c : node->children) ProcessNodeRec(c.get(), true);
        --loopDepth;
        return;
    }

    // Stmt-level specific checks (break/continue/return/printf/assignment)
    if (node->name == "Stmt") {
        if (!node->children.empty()) {
            auto &first = node->children[0];
            if (first->isToken) {
                switch (first->token.type) {
                    case TokenType::BREAKTK: {
                        if (loopDepth == 0) ErrorRecorder::AddError(Error(ErrorType::BAD_BREAK_CONTINUE, first->token.line));
                        return;
                    }
                    case TokenType::CONTINUETK: {
                        if (loopDepth == 0) ErrorRecorder::AddError(Error(ErrorType::BAD_BREAK_CONTINUE, first->token.line));
                        return;
                    }
                    case TokenType::FORTK: {
                        ++loopDepth;
                        for (size_t i = 1; i < node->children.size(); ++i) ProcessNodeRec(node->children[i].get(), true);
                        --loopDepth;
                        return;
                    }
                    case TokenType::RETURNTK: {
                        int line = first->token.line;
                        bool hasExp = false;
                        for (const auto &c : node->children) if (c->name == "Exp") { hasExp = true; break; }
                        currentFuncHasReturn = true;
                        if (currentFuncReturnType == "void" && hasExp) {
                            ErrorRecorder::AddError(Error(ErrorType::RETURN_IN_VOID, line));
                        }
                        return;
                    }
                    case TokenType::PRINTFTK: {
                        int line = first->token.line;
                        int fmtIndex = -1;
                        for (size_t i = 0; i < node->children.size(); ++i) {
                            if (node->children[i]->isToken && node->children[i]->token.type == TokenType::STRCON) { fmtIndex = (int)i; break; }
                        }
                        int fmtCount = 0;
                        if (fmtIndex >= 0) {
                            std::string s = node->children[fmtIndex]->token.value;
                            for (size_t p = 0; p + 1 < s.size(); ++p) if (s[p] == '%' && s[p+1] == 'd') ++fmtCount;
                        }
                        int expCount = 0;
                        for (const auto &c : node->children) if (c->name == "Exp") ++expCount;
                        if (fmtCount != expCount) ErrorRecorder::AddError(Error(ErrorType::PRINTF_ARG_MISMATCH, line));
                        for (const auto &c : node->children) ProcessNodeRec(c.get(), true);
                        return;
                    }
                    default: break;
                }
            }

            // assignment form: LVal '=' Exp ...
            if (node->children.size() >= 3 && node->children[0]->name == "LVal" && node->children[1]->isToken && node->children[1]->token.type == TokenType::ASSIGN) {
                std::string id = GetIdent(node->children[0].get());
                int line = GetIdentLine(node->children[0].get());
                if (!id.empty()) {
                    Symbol* s = SymbolManager::Lookup(id);
                    if (!s) {
                        ErrorRecorder::AddError(Error(ErrorType::NAME_UNDEFINED, line));
                    } else {
                        if (s->isConst) ErrorRecorder::AddError(Error(ErrorType::ASSIGN_TO_CONST, line));
                    }
                }
                // also descend into RHS to check undefined names inside expressions
                for (size_t i = 2; i < node->children.size(); ++i) ProcessNodeRec(node->children[i].get(), true);
                return;
            }
        }
    }

    // UnaryExp: handle function calls
    if (node->name == "UnaryExp") {
        if (!node->children.empty() && node->children[0]->isToken && node->children[0]->token.type == TokenType::IDENFR) {
            // function call pattern: IDENFR LPARENT [FuncRParams] RPARENT
            if (node->children.size() >= 2 && node->children[1]->isToken && node->children[1]->token.type == TokenType::LPARENT) {
                std::string fname = node->children[0]->token.value;
                int line = node->children[0]->token.line;
                Symbol* fsym = SymbolManager::Lookup(fname);
                if (!fsym) {
                    ErrorRecorder::AddError(Error(ErrorType::NAME_UNDEFINED, line));
                } else {
                    // check param count and types
                    std::vector<const ASTNode*> args;
                    if (node->children.size() >= 3 && node->children[2]->name == "FuncRParams") {
                        args = GetFuncRParamExprs(node->children[2].get());
                    }
                    int argCount = (int)args.size();
                    int expect = (int)fsym->paramTypes.size();
                    if (argCount != expect) {
                        ErrorRecorder::AddError(Error(ErrorType::FUNC_PARAM_COUNT_MISMATCH, line));
                    } else {
                        for (int i = 0; i < argCount; ++i) {
                            bool argIsArray = ExprIsArray(args[i]);
                            bool expectArray = (fsym->paramTypes[i].find("Array") != std::string::npos);
                            if (argIsArray != expectArray) {
                                ErrorRecorder::AddError(Error(ErrorType::FUNC_PARAM_TYPE_MISMATCH, line));
                                break;
                            }
                        }
                    }
                }
                return;
            }
        }
    }

    // VarDecl
    if (node->name == "VarDecl") {
        bool isStatic = HasToken(node, TokenType::STATICTK);
        std::string btype = GetBType(node);
        for (const auto &ch : node->children) {
            if (ch->name == "VarDef") {
                std::string ident = GetIdent(ch.get());
                int dim = CountImmediateToken(ch.get(), TokenType::LBRACK);
                std::string outType;
                if (btype == "int") {
                    if (isStatic) outType = (dim > 0) ? "StaticIntArray" : "StaticInt";
                    else outType = (dim > 0) ? "IntArray" : "Int";
                } else outType = (dim > 0) ? "IntArray" : "Int";
                if (!ident.empty()) {
                    int line = GetIdentLine(ch.get());
                    Symbol s(ident, outType, isStatic, dim, false, line);
                    SymbolManager::AddSymbol(s);
                }
            }
        }
        return;
    }

    // ConstDecl
    if (node->name == "ConstDecl") {
        std::string btype = GetBType(node);
        for (const auto &ch : node->children) {
            if (ch->name == "ConstDef") {
                std::string ident = GetIdent(ch.get());
                int dim = CountImmediateToken(ch.get(), TokenType::LBRACK);
                std::string outType = (btype == "int") ? (dim > 0 ? "ConstIntArray" : "ConstInt") : (dim > 0 ? "ConstIntArray" : "ConstInt");
                if (!ident.empty()) {
                    int line = GetIdentLine(ch.get());
                    Symbol s(ident, outType, false, dim, true, line);
                    SymbolManager::AddSymbol(s);
                }
            }
        }
        return;
    }

    // FuncFParam handling: add parameter symbols into current function scope
    if (node->name == "FuncFParam") {
        std::string btype = GetBType(node);
        std::string ident = GetIdent(node);
        int dim = CountImmediateToken(node, TokenType::LBRACK);
        std::string outType = (btype == "int") ? (dim > 0 ? "IntArray" : "Int") : "Int";
        if (!ident.empty()) {
            int line = GetIdentLine(node);
            Symbol s(ident, outType, false, dim, false, line);
            SymbolManager::AddSymbol(s);
        }
        return;
    }

    // Block
    if (node->name == "Block") {
        if (createScopeForBlock) {
            SymbolManager::CreateScope();
            for (const auto &ch : node->children) ProcessNodeRec(ch.get(), true);
            SymbolManager::ExitScope();
        } else {
            for (const auto &ch : node->children) ProcessNodeRec(ch.get(), true);
        }
        return;
    }

    // descend into children
    for (const auto &ch : node->children) ProcessNodeRec(ch.get(), true);
}

std::string SemanticAnalyzer::GetBType(const ASTNode* node) {
    if (!node) return "";
    if (node->isToken) {
        if (node->token.type == TokenType::INTTK) return "int";
        if (node->token.type == TokenType::VOIDTK) return "void";
    }
    for (const auto &c : node->children) {
        std::string r = GetBType(c.get());
        if (!r.empty()) return r;
    }
    return "";
}

std::string SemanticAnalyzer::GetIdent(const ASTNode* node) {
    if (!node) return "";
    if (node->isToken && node->token.type == TokenType::IDENFR) return node->token.value;
    for (const auto &c : node->children) {
        std::string r = GetIdent(c.get());
        if (!r.empty()) return r;
    }
    return "";
}

int SemanticAnalyzer::CountToken(const ASTNode* node, TokenType t) {
    if (!node) return 0;
    int cnt = 0;
    if (node->isToken && node->token.type == t) ++cnt;
    for (const auto &c : node->children) cnt += CountToken(c.get(), t);
    return cnt;
}

// Count tokens of type `t` that are immediate children of `node` (do not recurse).
// (implementation defined above)

bool SemanticAnalyzer::HasToken(const ASTNode* node, TokenType t) {
    if (!node) return false;
    if (node->isToken && node->token.type == t) return true;
    for (const auto &c : node->children) if (HasToken(c.get(), t)) return true;
    return false;
}

void SemanticAnalyzer::DumpSymbolFile() {
    auto tables = SymbolManager::GetAllTablesInIdOrder();
    std::ofstream of("symbol.txt");
    if (!of) return;
    for (auto *t : tables) {
        if (!t) continue;
        for (const auto &s : t->symbols) {
            of << t->id << " " << s.name << " " << s.typeName << '\n';
        }
    }
    of.close();
}
