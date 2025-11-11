#include "SemanticAnalyzer.hpp"
#include <fstream>
#include <iostream>
#include "../../error/ErrorRecorder.hpp"
#include "../../error/Error.hpp"
#include "../../error/ErrorType.hpp"

using std::string;

SemanticAnalyzer::SemanticAnalyzer() {
    // create global scope id = 1
    SymbolTable gt;
    gt.id = 1;
    gt.fatherId = 0;
    tables.push_back(gt);
    currentTable = 0;
}

int SemanticAnalyzer::NewScope() {
    SymbolTable t;
    t.id = (int)tables.size() + 1; // next id
    t.fatherId = (currentTable >= 0) ? tables[currentTable].id : 0;
    tables.push_back(t);
    currentTable = (int)tables.size() - 1;
    return t.id;
}

void SemanticAnalyzer::ExitScope() {
    if (currentTable > 0) {
        // set to father table index
        int fid = tables[currentTable].fatherId;
        // find index of father table
        for (size_t i = 0; i < tables.size(); ++i) {
            if (tables[i].id == fid) {
                currentTable = (int)i;
                return;
            }
        }
        currentTable = 0;
    }
}

void SemanticAnalyzer::Analyze(const ASTNode *root) {
    // default behavior: emit symbol.txt for correct programs unless explicitly disabled
    // If an 'emit_symbol.txt' file exists and contains '0', disable emission. Any other
    // value (or missing file) will enable symbol emission.
    std::ifstream ef("emit_symbol.txt");
    if (ef) {
        string v; ef >> v;
        if (v == "0") emitSymbol = false; else emitSymbol = true;
    } else {
        emitSymbol = true;
    }

    Walk(root);

    if (emitSymbol) {
        std::ofstream of("symbol.txt");
        // output by scope id ascending (tables are created in ascending id order)
        for (const auto &tbl : tables) {
                for (const auto &sym : tbl.symbols) {
                    // format: 作用域序号 单词的字符/字符串形式 类型名称
                    string tname;
                    // mapping using Symbol::Kind, isConst and isStatic
                    if (sym.kind == Symbol::Kind::FUNC) {
                        tname = (sym.retype == 0) ? "VoidFunc" : "IntFunc";
                    } else if (sym.kind == Symbol::Kind::ARRAY) {
                        if (sym.isConst) tname = "ConstIntArray";
                        else if (sym.isStatic) tname = "StaticIntArray";
                        else tname = "IntArray";
                    } else { // VAR
                        if (sym.isConst) tname = "ConstInt";
                        else if (sym.isStatic) tname = "StaticInt";
                        else tname = "Int";
                    }

                    of << tbl.id << " " << sym.token << " " << tname << "\n";
                }
        }
    }
}

    // helper to report semantic error and continue
    static void ReportSemanticError(ErrorType t, int line) {
        ErrorRecorder::AddError(Error(t, line));
    }

void SemanticAnalyzer::Walk(const ASTNode *node) {
    if (!node) return;
    if (node->name == "CompUnit") {
        HandleCompUnit(node);
        return;
    }

    // fallback: walk children
    for (const auto &c : node->children) Walk(c.get());
}

Symbol* SemanticAnalyzer::Lookup(const std::string &name) {
    // start from current table and go outward following fatherId
    int idx = currentTable;
    while (idx >= 0) {
        Symbol *s = tables[idx].FindLocal(name);
        if (s) return s;
        int fid = tables[idx].fatherId;
        if (fid == 0) break;
        // find father index
        int next = -1;
        for (size_t i = 0; i < tables.size(); ++i) if (tables[i].id == fid) { next = (int)i; break; }
        if (next == -1) break;
        idx = next;
    }
    return nullptr;
}

void SemanticAnalyzer::HandleCompUnit(const ASTNode *node) {
    // CompUnit children are Decl | FuncDef | MainFuncDef
    for (const auto &c : node->children) {
        if (c->name == "Decl") HandleDecl(c.get());
        else if (c->name == "FuncDef") HandleFuncDef(c.get(), false);
        else if (c->name == "MainFuncDef") HandleFuncDef(c.get(), true);
    }
}

void SemanticAnalyzer::HandleDecl(const ASTNode *node) {
    if (node->children.empty()) return;
    const ASTNode *child = node->children[0].get();
    if (child->name == "ConstDecl") HandleConstDecl(child);
    else if (child->name == "VarDecl") HandleVarDecl(child);
}

void SemanticAnalyzer::HandleConstDecl(const ASTNode *node) {
    // node children: 'const' BType ConstDef { , ConstDef } ;
    // iterate children, find ConstDef nodes
    for (const auto &c : node->children) {
        if (c->name == "ConstDef") {
            // in ConstDef, first child is IDENFR (token)
            if (c->children.size() >= 1 && c->children[0]->isToken) {
                auto tk = c->children[0]->token;
                Symbol s;
                s.token = tk.value;
                s.tableId = tables[currentTable].id;
                s.isConst = true;
                s.isStatic = false;
                s.line = tk.line;
                // if array (has LBRACK) then kind->ARRAY
                bool isArray = false;
                for (const auto &cc : c->children) if (cc->isToken && cc->token.type == TokenType::LBRACK) isArray = true;
                s.kind = isArray ? Symbol::Kind::ARRAY : Symbol::Kind::VAR;
                if (tables[currentTable].Insert(s) == -1) {
                    // redefinition in current scope
                    ReportSemanticError(ErrorType::REDEFINE, s.line);
                }
            }
        }
    }
}

void SemanticAnalyzer::HandleVarDecl(const ASTNode *node) {
    // VarDecl may start with static
    bool isStatic = false;
    for (const auto &c : node->children) {
        if (c->isToken && c->token.type == TokenType::STATICTK) { isStatic = true; break; }
    }

    for (const auto &c : node->children) {
        if (c->name == "VarDef") {
            if (c->children.size() >= 1 && c->children[0]->isToken) {
                auto tk = c->children[0]->token;
                Symbol s;
                s.token = tk.value;
                s.tableId = tables[currentTable].id;
                s.isConst = false;
                s.isStatic = isStatic;
                s.line = tk.line;
                // check if array
                bool isArray = false;
                for (const auto &cc : c->children) if (cc->isToken && cc->token.type == TokenType::LBRACK) isArray = true;
                s.kind = isArray ? Symbol::Kind::ARRAY : Symbol::Kind::VAR;
                if (tables[currentTable].Insert(s) == -1) {
                    ReportSemanticError(ErrorType::REDEFINE, s.line);
                }
            }
        }
    }
}

void SemanticAnalyzer::HandleFuncDef(const ASTNode *node, bool isMain) {
    // FuncDef children: FuncType IDENFR '(' [FuncFParams] ')' Block
    // find function name token
    std::string fname;
    int funcRet = 1; // int
    int fnameLine = -1;
    const ASTNode *paramsNode = nullptr;
    const ASTNode *blkNode = nullptr;

    for (const auto &c : node->children) {
        if (c->name == "FuncType") {
            // first token child is VOID or INT
            if (!c->children.empty() && c->children[0]->isToken) {
                if (c->children[0]->token.type == TokenType::VOIDTK) funcRet = 0;
                else funcRet = 1;
            }
        }
        if (c->isToken && c->token.type == TokenType::IDENFR) {
            fname = c->token.value;
            // preserve function name declaration line
            if (fnameLine == -1) fnameLine = c->token.line;
        }
        if (c->name == "FuncFParams") paramsNode = c.get();
        if (c->name == "Block") blkNode = c.get();
    }

    if (!fname.empty()) {
        // prepare param types list
        std::vector<Symbol> params = BuildParamSymbols(paramsNode);
        if (!isMain) {
            // insert function into current (global) scope
            Symbol s;
            s.token = fname;
            s.tableId = tables[currentTable].id;
            s.kind = Symbol::Kind::FUNC;
            s.retype = funcRet;
            s.paramNum = (int)params.size();
            s.paramTypes.clear();
            for (auto &ps : params) s.paramTypes.push_back(ps.kind);
            s.line = -1; // function name line not preserved here
            if (tables[currentTable].Insert(s) == -1) {
                // redefinition of function name in global scope
                ReportSemanticError(ErrorType::REDEFINE, fnameLine >= 0 ? fnameLine : 0);
            }
        }
        // function body handling below will insert params into function scope
    }

    // function body: create a scope via HandleBlock and ensure parameters
    // are inserted into that scope before processing the block items
    if (blkNode) {
        std::vector<Symbol> params = BuildParamSymbols(paramsNode);
        // set current function context
        int prevFunctionRet = currentFunctionRet;
        bool prevHasReturn = currentFunctionHasReturn;
        currentFunctionRet = funcRet;
        currentFunctionHasReturn = false;
        HandleBlock(blkNode, &params);
        // after processing body, check missing return for non-void
        if (funcRet == 1) {
            // need to check if last statement in blkNode is a return
            bool hasReturnAtEnd = false;
            for (auto it = blkNode->children.rbegin(); it != blkNode->children.rend(); ++it) {
                const ASTNode *ch = it->get();
                if (ch->name == "BlockItem") {
                    if (!ch->children.empty()) {
                        const ASTNode *inner = ch->children[0].get();
                        if (inner->name == "Stmt") {
                            if (!inner->children.empty() && inner->children[0]->isToken && inner->children[0]->token.type == TokenType::RETURNTK) {
                                hasReturnAtEnd = true;
                            }
                        }
                    }
                    break;
                }
            }
            if (!hasReturnAtEnd) {
                // find closing brace token line
                int braceLine = -1;
                for (const auto &ch : blkNode->children) if (ch->isToken && ch->token.type == TokenType::RBRACE) braceLine = ch->token.line;
                if (braceLine < 0) braceLine = -1;
                ReportSemanticError(ErrorType::MISSING_RETURN, braceLine);
            }
        }
        // restore
        currentFunctionRet = prevFunctionRet;
        currentFunctionHasReturn = prevHasReturn;
    }
}

void SemanticAnalyzer::HandleFuncFParams(const ASTNode *node) {
    // sequence of FuncFParam or commas
    for (const auto &c : node->children) {
        if (c->name == "FuncFParam") HandleFuncFParam(c.get());
    }
}

void SemanticAnalyzer::HandleFuncFParam(const ASTNode *node) {
    // FuncFParam -> BType IDENFR [ '[' ']' ]
    std::string name;
    bool isArray = false;
    for (const auto &c : node->children) {
        if (c->isToken && c->token.type == TokenType::IDENFR) name = c->token.value;
        if (c->isToken && c->token.type == TokenType::LBRACK) isArray = true;
    }
    if (!name.empty()) {
        Symbol s;
        s.token = name;
        s.tableId = tables[currentTable].id;
        s.isConst = false;
        s.isStatic = false;
        s.kind = isArray ? Symbol::Kind::ARRAY : Symbol::Kind::VAR;
        s.line = (node->children.size() >= 1 && node->children[0]->isToken) ? node->children[0]->token.line : -1;
        if (tables[currentTable].Insert(s) == -1) {
            ReportSemanticError(ErrorType::REDEFINE, s.line);
        }
    }
}

void SemanticAnalyzer::HandleBlock(const ASTNode *node, const std::vector<Symbol>* preInsert) {
    // node children: '{' { BlockItem } '}'
    // create nested scope for this block
    NewScope();
    // if caller supplied symbols to pre-insert (e.g., function parameters), insert them now
    if (preInsert) {
        for (const auto &ps : *preInsert) {
            Symbol s = ps; // copy
            s.tableId = tables[currentTable].id;
            int res = tables[currentTable].Insert(s);
            if (res == -1) {
                ReportSemanticError(ErrorType::REDEFINE, s.line);
            }
        }
    }
    for (const auto &c : node->children) {
        if (c->name == "BlockItem") {
            // BlockItem -> Decl | Stmt
            if (!c->children.empty()) {
                const ASTNode *inner = c->children[0].get();
                if (inner->name == "Decl") HandleDecl(inner);
                else {
                    // handle statements (this will also recurse into nested blocks)
                    HandleStmt(inner);
                }
            }
        }
    }
    ExitScope();
}

void SemanticAnalyzer::HandleStmt(const ASTNode *node) {
    if (!node) return;
    // detect return
    if (!node->children.empty() && node->children[0]->isToken) {
        auto tk = node->children[0]->token;
        if (tk.type == TokenType::RETURNTK) {
            // if current function is void and return has expression -> error f
            if (currentFunctionRet == 0) {
                // return with expression detection: if node has more than 1 child and next isn't SEMICN
                if (node->children.size() > 1) {
                    ReportSemanticError(ErrorType::RETURN_VALUE_IN_VOID, tk.line);
                }
            }
            // mark that function contains a return somewhere
            currentFunctionHasReturn = true;
            return;
        }
        if (tk.type == TokenType::BREAKTK || tk.type == TokenType::CONTINUETK) {
            if (loopDepth <= 0) {
                ReportSemanticError(ErrorType::BAD_BREAK_CONTINUE, tk.line);
            }
            return;
        }
        if (tk.type == TokenType::PRINTFTK) {
            // printf ( STRCON {, Exp } ) ;
            // count number of Exp expressions after first STRCON
            // find STRCON child in node->children
            int exprCount = 0;
            bool seenStr = false;
            for (const auto &c : node->children) {
                if (c->isToken && c->token.type == TokenType::STRCON) { seenStr = true; continue; }
                if (seenStr) {
                    // count expressions by looking for subtrees named Exp
                    if (c->name == "Exp" || c->name == "LVal" || c->name == "AddExp" || c->name == "PrimaryExp") exprCount++;
                }
            }
            // count placeholders '%d' in the string
            int placeholder = 0;
            for (const auto &c : node->children) {
                if (c->isToken && c->token.type == TokenType::STRCON) {
                    const std::string &s = c->token.value;
                    for (size_t i = 0; i + 1 < s.size(); ++i) if (s[i] == '%' && s[i+1] == 'd') placeholder++;
                }
            }
            if (placeholder != exprCount) {
                ReportSemanticError(ErrorType::PRINTF_MISMATCH, tk.line);
            }
            // still check inner expressions for undefined/function calls
            for (const auto &c : node->children) HandleExp(c.get());
            return;
        }
    }

    // for-loop handling: increment loopDepth when descending into body
    if (node->name == "ForStmt" || (!node->children.empty() && node->children[0]->isToken && node->children[0]->token.type == TokenType::FORTK)) {
        // search for nested LVal=Exp assignments inside this ForStmt
        loopDepth++;
        for (const auto &c : node->children) HandleExp(c.get());
        // body may be later in Stmt nodes; search subtree
        WalkForBlocks(node);
        loopDepth--;
        return;
    }

    // assignment stmt: LVal '=' Exp ';' pattern
    if (!node->children.empty() && node->children[0]->name == "LVal") {
        // check LVal definition
        const ASTNode *lval = node->children[0].get();
        if (!lval->children.empty() && lval->children[0]->isToken) {
            auto tk = lval->children[0]->token;
            Symbol *s = Lookup(tk.value);
            if (!s) {
                ReportSemanticError(ErrorType::UNDEFINED, tk.line);
            } else {
                if (s->isConst) ReportSemanticError(ErrorType::ASSIGN_TO_CONST, tk.line);
            }
        }
        // also traverse RHS expression for function-call checks and undefined names
        for (const auto &c : node->children) HandleExp(c.get());
        return;
    }

    // generic statement: traverse children for expressions/statements
    for (const auto &c : node->children) {
        if (c->name == "Block") HandleBlock(c.get());
        else HandleExp(c.get());
        // look for nested statements
        if (c->name == "Stmt") HandleStmt(c.get());
    }
}

void SemanticAnalyzer::HandleExp(const ASTNode *node) {
    if (!node) return;
    if (node->name == "LVal") {
        HandleLValUse(node);
        return;
    }
    if (node->name == "UnaryExp") {
        HandleUnaryExp(node);
        return;
    }
    // traverse children
    for (const auto &c : node->children) HandleExp(c.get());
}

void SemanticAnalyzer::HandleUnaryExp(const ASTNode *node) {
    if (!node) return;
    // function call form: IDENFR '(' [FuncRParams] ')'
    if (!node->children.empty() && node->children[0]->isToken && node->children[0]->token.type == TokenType::IDENFR && node->children.size() >= 2 && node->children[1]->isToken && node->children[1]->token.type == TokenType::LPARENT) {
        std::string fname = node->children[0]->token.value;
        int fnameLine = node->children[0]->token.line;
        Symbol *fsym = Lookup(fname);
        if (!fsym || fsym->kind != Symbol::Kind::FUNC) {
            ReportSemanticError(ErrorType::UNDEFINED, fnameLine);
            // still traverse args
            for (const auto &c : node->children) HandleExp(c.get());
            return;
        }
        // find FuncRParams child if any
        const ASTNode *rparams = nullptr;
        for (const auto &c : node->children) if (c->name == "FuncRParams") rparams = c.get();
        int actualCount = 0;
        std::vector<Symbol::Kind> actualKinds;
        if (rparams) {
            for (const auto &arg : rparams->children) {
                if (arg->name == "Exp") {
                    actualCount++;
                    // determine if argument is array name (Identifier LVal without bracket)
                    if (!arg->children.empty() && arg->children[0]->name == "LVal") {
                        const ASTNode *l = arg->children[0].get();
                        if (!l->children.empty() && l->children[0]->isToken) {
                            std::string aname = l->children[0]->token.value;
                            Symbol *asym = Lookup(aname);
                            if (asym && asym->kind == Symbol::Kind::ARRAY && (l->children.size() == 1)) actualKinds.push_back(Symbol::Kind::ARRAY);
                            else actualKinds.push_back(Symbol::Kind::VAR);
                        } else actualKinds.push_back(Symbol::Kind::VAR);
                    } else {
                        actualKinds.push_back(Symbol::Kind::VAR);
                    }
                }
            }
        }
        if (actualCount != fsym->paramNum) {
            ReportSemanticError(ErrorType::FUNC_PARAM_COUNT, fnameLine);
        } else {
            // check types
            for (int i = 0; i < actualCount; ++i) {
                Symbol::Kind expected = fsym->paramTypes[i];
                Symbol::Kind got = actualKinds[i];
                if (expected != got) {
                    ReportSemanticError(ErrorType::FUNC_PARAM_TYPE, fnameLine);
                    break;
                }
            }
        }
        // traverse args recursively
        if (rparams) for (const auto &arg : rparams->children) HandleExp(arg.get());
        return;
    }
    // fallback traverse
    for (const auto &c : node->children) HandleExp(c.get());
}

void SemanticAnalyzer::HandleLValUse(const ASTNode *node) {
    if (!node) return;
    if (!node->children.empty() && node->children[0]->isToken) {
        auto tk = node->children[0]->token;
        Symbol *s = Lookup(tk.value);
        if (!s) ReportSemanticError(ErrorType::UNDEFINED, tk.line);
    }
}

void SemanticAnalyzer::WalkForBlocks(const ASTNode *node) {
    if (!node) return;
    if (node->name == "Block") {
        HandleBlock(node);
        return; // handled its own subtree inside HandleBlock
    }
    for (const auto &c : node->children) {
        WalkForBlocks(c.get());
    }
}

std::vector<Symbol> SemanticAnalyzer::BuildParamSymbols(const ASTNode *paramsNode) {
    std::vector<Symbol> res;
    if (!paramsNode) return res;
    for (const auto &c : paramsNode->children) {
        if (c->name == "FuncFParam") {
            // Get identifier and whether array
            Symbol s;
            s.isConst = false;
            s.btype = 0;
            s.kind = Symbol::Kind::VAR;
            for (const auto &gc : c->children) {
                if (gc->isToken && gc->token.type == TokenType::IDENFR) { s.token = gc->token.value; s.line = gc->token.line; }
                if (gc->isToken && gc->token.type == TokenType::LBRACK) s.kind = Symbol::Kind::ARRAY; // array
            }
            res.push_back(s);
        }
    }
    return res;
}
