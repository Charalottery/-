#include "SemanticAnalyzer.hpp"
#include <fstream>
#include <iostream>
#include <functional>
#include "../../error/ErrorRecorder.hpp"
#include "../../error/Error.hpp"
#include "../../error/ErrorType.hpp"

using std::string;

// Debug instrumentation removed: event counter and logging are disabled in
// production runs. Keep scope bookkeeping structures but avoid printing
// verbose runtime traces to stderr.

// Temporary: pretty-print AST helper for diagnostics
static void DumpASTNode(const ASTNode* node, std::ofstream &of, int indent=0) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) of << "  ";
    of << node->name;
    if (node->isToken) {
        of << " : token(" << node->token.value << ") line=" << node->token.line;
    }
    of << "\n";
    for (const auto &c : node->children) DumpASTNode(c.get(), of, indent+1);
}

// Treat some known library identifiers as builtin so they are not reported as UNDEFINED
static bool IsBuiltin(const std::string &name) {
    return name == "getint" || name == "printf";
}

SemanticAnalyzer::SemanticAnalyzer() {
    // create global scope id = 1
    SymbolTable gt;
    gt.id = 1;
    gt.fatherId = 0;
    tables.push_back(gt);
    currentTable = 0;
    // record mapping of table id -> index
    tableIdIndexMap[gt.id] = 0;
}

int SemanticAnalyzer::NewScope() {
    SymbolTable t;
    t.id = (int)tables.size() + 1; // next id
    t.fatherId = (currentTable >= 0) ? tables[currentTable].id : 0;
    // (debug logging removed)
    tables.push_back(t);
    currentTable = (int)tables.size() - 1;
    // record mapping
    tableIdIndexMap[t.id] = currentTable;
    return t.id;
}

void SemanticAnalyzer::ExitScope() {
    if (currentTable > 0) {
        // set to father table index
        int fid = tables[currentTable].fatherId;
        // fast lookup via map
        auto it = tableIdIndexMap.find(fid);
        if (it != tableIdIndexMap.end()) currentTable = it->second;
        else currentTable = 0;
        // no debug logging
    }
}

void SemanticAnalyzer::Analyze(const ASTNode *root) {
    // (symbol emission control removed: caller decides whether to write symbol.txt)

    // Reset scope tables to a fresh global scope for this analysis run
    tables.clear();
    SymbolTable gt;
    gt.id = 1;
    gt.fatherId = 0;
    tables.push_back(gt);
    currentTable = 0;
    nodeScopeMap.clear();
    // reset id->index mapping for fresh analysis run
    tableIdIndexMap.clear();
    tableIdIndexMap[gt.id] = 0;

    // Single-pass analysis: walk the AST, creating scopes and inserting
    // declarations as we encounter them, and perform checks on-the-fly.
    Walk(root);

    // Symbol emission removed from analyzer. Caller (main.cpp) decides whether
    // to write out `symbol.txt` (e.g., only when there are no errors).
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
        if (fid == 0) break; // reached outermost
        auto it = tableIdIndexMap.find(fid);
        if (it == tableIdIndexMap.end()) break;
        idx = it->second;
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
                    int _res = tables[currentTable].Insert(s);
                    if (_res == -1) {
                        // diagnostic: print existing vs new symbol properties to help
                        // debug why a duplicate insert is reported in the same table.
                        Symbol *prev = tables[currentTable].FindLocal(s.token);
                        if (prev) {
                            // previous symbol exists; report redefine as semantic error below
                        } else {
                            // previous not found; will report redefine below
                        }
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
                    int _res = tables[currentTable].Insert(s);
                    if (_res == -1) {
                        // previous symbol exists or not; ReportSemanticError will handle diagnostics
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
            s.line = fnameLine; // preserve function name declaration line for error reporting
                    if (tables[currentTable].Insert(s) == -1) {
                        // Redefinition detected; report semantic error (do not print debug info)
                        ReportSemanticError(ErrorType::REDEFINE, s.line >= 0 ? s.line : 0);
                    }
        }
        // function body handling below will insert params into function scope
    }

    // function body: create a scope via HandleBlock and ensure parameters
    // are inserted into that scope before processing the block items
    if (blkNode) {
        // (debug AST dump removed)
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
    // create nested scope for this block, but avoid creating it multiple
    // times for the same AST node by consulting nodeScopeMap. If a scope
    // already exists for this node, reuse it.
    bool created_here = false;
    int prevTable = currentTable;
    auto it = nodeScopeMap.find(node);
    if (it == nodeScopeMap.end()) {
        NewScope();
        nodeScopeMap[node] = currentTable;
        created_here = true;
    } else {
        currentTable = it->second;
    }
    // (debug tables dump removed)
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
    // Single-pass block processing: traverse BlockItem in order. When a
    // declaration is encountered we insert it immediately; otherwise we
    // handle the statement/expression. This preserves single-pass semantics
    // (no global two-pass) while ensuring declarations that appear before
    // a use are visible to subsequent items.
    for (const auto &c : node->children) {
        if (c->name != "BlockItem") continue;
        if (c->children.empty()) continue;
        const ASTNode *inner = c->children[0].get();
        if (!inner) continue;
        if (inner->name == "Decl") {
            // declaration: insert immediately so later items in this block
            // can see the symbol during the same single traversal.
            HandleDecl(inner);
        } else {
            // handle statements and other constructs in order
            if (inner->name == "Stmt") HandleStmt(inner);
            else HandleExp(inner);
        }
    }
    if (created_here) ExitScope();
    else currentTable = prevTable;
}

void SemanticAnalyzer::HandleStmt(const ASTNode *node) {
    if (!node) return;
    // detect return
    if (!node->children.empty() && node->children[0]->isToken) {
        auto tk = node->children[0]->token;
        if (tk.type == TokenType::RETURNTK) {
            // if current function is void and return has expression -> error f
            if (currentFunctionRet == 0) {
                // detect whether this return carries an expression
                bool hasExpr = false;
                for (const auto &ch : node->children) {
                    if (ch && ch->name == "Exp") { hasExpr = true; break; }
                }
                if (hasExpr) {
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
            // Count placeholders '%d' in the first STRCON and count only
            // AST children named 'Exp' after the STRCON as actual args.
            int exprCount = 0;
            int placeholder = 0;
            bool seenStr = false;
            for (const auto &c : node->children) {
                if (c->isToken && c->token.type == TokenType::STRCON) {
                    if (!seenStr) {
                        const std::string &s = c->token.value;
                        for (size_t i = 0; i + 1 < s.size(); ++i) if (s[i] == '%' && s[i+1] == 'd') placeholder++;
                        seenStr = true;
                    }
                    continue;
                }
                if (seenStr) {
                    if (c->name == "Exp") exprCount++;
                }
            }
            if (placeholder != exprCount) ReportSemanticError(ErrorType::PRINTF_MISMATCH, tk.line);
            // still check inner expressions for undefined/function calls
            for (const auto &c : node->children) HandleExp(c.get());
            return;
        }
    }

    // ForStmt and other statements: evaluate children strictly in
    // textual (left-to-right) order. This ensures we don't traverse
    // a subexpression earlier than its lexical position. When a child
    // is a Block, HandleBlock will create its scope and process its
    // items in-order, so no separate pre-walk is necessary.
    if (node->name == "ForStmt" || (!node->children.empty() && node->children[0]->isToken && node->children[0]->token.type == TokenType::FORTK)) {
        loopDepth++;
        // ForStmt children may contain comma-separated LVal '=' Exp sequences.
        // Detect LVal (possibly wrapped inside an Exp node) followed by ASSIGN
        // and treat it as a write (assignment) so that ASSIGN_TO_CONST / UNDEFINED
        // checks run for for-header assigns.
        for (size_t i = 0; i < node->children.size(); ++i) {
            const auto &c = node->children[i];
            if (!c) continue;
            // helper to check an AST node for a direct LVal '=' pattern
            auto checkNodeForAssign = [&](const ASTNode* cand)->void {
                if (!cand) return;
                // case 1: direct LVal node
                if (cand->name == "LVal") {
                    if (!cand->children.empty() && cand->children[0]->isToken) {
                        auto tk = cand->children[0]->token;
                        Symbol *s = Lookup(tk.value);
                        if (!s) {
                            if (!IsBuiltin(tk.value)) ReportSemanticError(ErrorType::UNDEFINED, tk.line);
                        } else {
                            if (s->isConst) ReportSemanticError(ErrorType::ASSIGN_TO_CONST, tk.line);
                        }
                    }
                    return;
                }
                // case 2: Exp wrapping an assignment like Exp -> LVal ASSIGN Exp
                if (cand->name == "Exp" && cand->children.size() >= 2) {
                    const ASTNode *first = cand->children[0].get();
                    const ASTNode *second = cand->children[1].get();
                    if (first && first->name == "LVal" && second && second->isToken && second->token.type == TokenType::ASSIGN) {
                        if (!first->children.empty() && first->children[0]->isToken) {
                            auto tk = first->children[0]->token;
                            Symbol *s = Lookup(tk.value);
                            if (!s) {
                                if (!IsBuiltin(tk.value)) ReportSemanticError(ErrorType::UNDEFINED, tk.line);
                            } else {
                                if (s->isConst) ReportSemanticError(ErrorType::ASSIGN_TO_CONST, tk.line);
                            }
                        }
                    }
                }
            };

            // Check current child itself
            checkNodeForAssign(c.get());
            // If the child is a nested ForStmt (this Stmt may contain a ForStmt node),
            // its own children can contain the LVal ASSIGN pattern (parse produced a nested ForStmt node).
            if (c->name == "ForStmt") {
                for (const auto &fc : c->children) {
                    if (!fc) continue;
                    checkNodeForAssign(fc.get());
                    if (fc->name == "Block") HandleBlock(fc.get());
                    else HandleExp(fc.get());
                }
                continue;
            }
            // Additionally, if current child is an LVal and next child is an ASSIGN token
            if (c->name == "LVal" && i + 1 < node->children.size() && node->children[i+1]->isToken && node->children[i+1]->token.type == TokenType::ASSIGN) {
                checkNodeForAssign(c.get());
            }
            // If current child is Exp, its first child might be LVal and second an ASSIGN
            if (c->name == "Exp") checkNodeForAssign(c.get());

            if (c->name == "Block") HandleBlock(c.get());
            else HandleExp(c.get());
        }
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
                if (!IsBuiltin(tk.value)) {
                    ReportSemanticError(ErrorType::UNDEFINED, tk.line);
                }
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
    // traverse children; treat Block and Stmt specially so their scopes
    // are created/handled by the appropriate handlers instead of
    // descending into them as plain expressions (which can cause
    // lookups before the Block's scope is created).
    for (const auto &c : node->children) {
        if (!c) continue;
        if (c->name == "Block") {
            HandleBlock(c.get());
        } else if (c->name == "Stmt") {
            HandleStmt(c.get());
        } else {
            HandleExp(c.get());
        }
    }
}

void SemanticAnalyzer::HandleUnaryExp(const ASTNode *node) {
    if (!node) return;
    // function call form: IDENFR '(' [FuncRParams] ')'
    if (!node->children.empty() && node->children[0]->isToken && node->children[0]->token.type == TokenType::IDENFR && node->children.size() >= 2 && node->children[1]->isToken && node->children[1]->token.type == TokenType::LPARENT) {
        std::string fname = node->children[0]->token.value;
        int fnameLine = node->children[0]->token.line;
        Symbol *fsym = Lookup(fname);
        if (!fsym || fsym->kind != Symbol::Kind::FUNC) {
            if (!IsBuiltin(fname)) {
                ReportSemanticError(ErrorType::UNDEFINED, fnameLine);
            }
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
                    // Find a plain identifier LVal inside the argument expression (no brackets)
                    std::string foundId;
                    // Find a "pure" identifier LVal for this argument, but avoid
                    // descending into nested function-call subtrees. Previously the
                    // recursive search could find identifiers inside inner calls
                    // (e.g., in map(copyEven(arr1,...)) we'd pick up 'arr1' and
                    // incorrectly classify the whole expression as ARRAY). To
                    // prevent that, skip recursing into nodes that represent a
                    // function call and only accept LVal nodes encountered
                    // without passing through such call nodes.
                    std::function<void(const ASTNode*)> findPureId = [&](const ASTNode* n) {
                        if (!n || !foundId.empty()) return;
                        if (n->name == "LVal") {
                            if (!n->children.empty() && n->children[0]->isToken && n->children[0]->token.type == TokenType::IDENFR) {
                                // check for any direct bracket tokens inside this LVal
                                bool hasBracket = false;
                                for (const auto &ch : n->children) {
                                    if (ch->isToken && ch->token.type == TokenType::LBRACK) { hasBracket = true; break; }
                                }
                                if (!hasBracket) {
                                    foundId = n->children[0]->token.value;
                                    return;
                                }
                            }
                            return; // do not recurse deeper from an LVal
                        }
                        for (const auto &ch : n->children) {
                            if (!ch) continue;
                            // skip recursing into nested function calls (UnaryExp with IDENFR '(' ...)
                            if (ch->name == "UnaryExp" && ch->children.size() >= 2 && ch->children[0]->isToken && ch->children[0]->token.type == TokenType::IDENFR && ch->children[1]->isToken && ch->children[1]->token.type == TokenType::LPARENT) {
                                continue;
                            }
                            findPureId(ch.get());
                            if (!foundId.empty()) return;
                        }
                    };
                    findPureId(arg.get());
                    if (!foundId.empty()) {
                        Symbol *asym = Lookup(foundId);
                        (void)0; // silence unused-warning for asym in release path
                        if (asym && asym->kind == Symbol::Kind::ARRAY) actualKinds.push_back(Symbol::Kind::ARRAY);
                        else actualKinds.push_back(Symbol::Kind::VAR);
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
                    // In the evaluation environment, FUNC_PARAM_TYPE (e) is
                    // only reported for two cases: passing a variable where
                    // an array is expected, or passing an array where a
                    // variable is expected. Ignore other mismatches.
                    if ((expected == Symbol::Kind::ARRAY && got == Symbol::Kind::VAR) ||
                        (expected == Symbol::Kind::VAR && got == Symbol::Kind::ARRAY)) {
                        ReportSemanticError(ErrorType::FUNC_PARAM_TYPE, fnameLine);
                    }
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
        if (!s) {
        if (!IsBuiltin(tk.value)) {
            ReportSemanticError(ErrorType::UNDEFINED, tk.line);
        }
    }
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

// === DeclPass ==============================================================
// Create scopes and insert declarations across the whole AST. Scopes created
// here are recorded in nodeScopeMap so the check pass can reuse them.
void SemanticAnalyzer::DeclPass(const ASTNode *node) {
    if (!node) return;
    if (node->name == "CompUnit") {
        for (const auto &c : node->children) {
            if (c->name == "Decl") {
                // global decls: const/var
                HandleDecl(c.get());
            } else if (c->name == "FuncDef" || c->name == "MainFuncDef") {
                // insert function name into global scope and prepare its body
                // (do not descend into body with checks here; only create scope & insert params/decls)
                // Extract function parts
                std::string fname;
                int funcRet = 1;
                const ASTNode *paramsNode = nullptr;
                const ASTNode *blkNode = nullptr;
                for (const auto &gc : c->children) {
                    if (gc->name == "FuncType") {
                        if (!gc->children.empty() && gc->children[0]->isToken) {
                            if (gc->children[0]->token.type == TokenType::VOIDTK) funcRet = 0;
                            else funcRet = 1;
                        }
                    }
                    if (gc->isToken && gc->token.type == TokenType::IDENFR) fname = gc->token.value;
                    if (gc->name == "FuncFParams") paramsNode = gc.get();
                    if (gc->name == "Block") blkNode = gc.get();
                }
                if (!fname.empty() && c->name == "FuncDef") {
                    Symbol s;
                    s.token = fname;
                    s.tableId = tables[currentTable].id;
                    s.kind = Symbol::Kind::FUNC;
                    s.retype = funcRet;
                    std::vector<Symbol> params = BuildParamSymbols(paramsNode);
                    s.paramNum = (int)params.size();
                    for (auto &ps : params) s.paramTypes.push_back(ps.kind);
                    if (tables[currentTable].Insert(s) == -1) {
                        ReportSemanticError(ErrorType::REDEFINE, s.line);
                    }
                }
                // create function body scope and insert parameters and any decls inside
                if (blkNode) {
                    // avoid double-creating scope if already mapped
                    if (nodeScopeMap.find(blkNode) == nodeScopeMap.end()) {
                        NewScope();
                        nodeScopeMap[blkNode] = currentTable;
                        // insert params
                        std::vector<Symbol> params = BuildParamSymbols(paramsNode);
                        for (const auto &ps : params) {
                            Symbol s = ps; s.tableId = tables[currentTable].id;
                            int _r = tables[currentTable].Insert(s);
                            if (_r == -1) {
                                // redefinition detected when inserting parameter into function scope
                                ReportSemanticError(ErrorType::REDEFINE, s.line);
                            }
                        }
                        // insert declarations directly inside blkNode
                        for (const auto &bi : blkNode->children) {
                            if (bi->name == "BlockItem" && !bi->children.empty()) {
                                const ASTNode *inner = bi->children[0].get();
                                if (inner->name == "Decl") HandleDecl(inner);
                            }
                        }
                        // recurse to find nested Blocks inside this function body
                        for (const auto &bi : blkNode->children) DeclPass(bi.get());
                        ExitScope();
                    }
                }
            } else {
                // generic descent
                DeclPass(c.get());
            }
        }
        return;
    }

    if (node->name == "Block") {
        if (nodeScopeMap.find(node) != nodeScopeMap.end()) return; // already created
        NewScope();
        nodeScopeMap[node] = currentTable;
        // insert Decl children in this block
        for (const auto &bi : node->children) {
            if (bi->name == "BlockItem" && !bi->children.empty()) {
                const ASTNode *inner = bi->children[0].get();
                if (inner->name == "Decl") HandleDecl(inner);
            }
        }
        // recurse into children to find nested blocks
        for (const auto &c : node->children) DeclPass(c.get());
        ExitScope();
        return;
    }

    // default: recurse
    for (const auto &c : node->children) DeclPass(c.get());
}

// === CheckPass ============================================================
// Uses prebuilt nodeScopeMap to set currentTable when entering blocks; performs
// semantic checks (no symbol insertions that would alter tables).
void SemanticAnalyzer::CheckPass(const ASTNode *node) {
    if (!node) return;
    if (node->name == "CompUnit") {
        for (const auto &c : node->children) {
            if (c->name == "Decl") {
                // Decl may contain initializer expressions that need checking
                for (const auto &gc : c->children) CheckPass(gc.get());
            } else if (c->name == "FuncDef" || c->name == "MainFuncDef") {
                // find function body and run check on it
                const ASTNode *blkNode = nullptr;
                int funcRet = 1;
                for (const auto &gc : c->children) {
                    if (gc->name == "FuncType") {
                        if (!gc->children.empty() && gc->children[0]->isToken && gc->children[0]->token.type == TokenType::VOIDTK) funcRet = 0;
                    }
                    if (gc->name == "Block") blkNode = gc.get();
                }
                if (blkNode) {
                    // set function context
                    int prevFunctionRet = currentFunctionRet;
                    bool prevHasReturn = currentFunctionHasReturn;
                    currentFunctionRet = funcRet;
                    currentFunctionHasReturn = false;
                    CheckPass(blkNode);
                    // check missing return as in HandleFuncDef
                    if (funcRet == 1) {
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
                            int braceLine = -1;
                            for (const auto &ch : blkNode->children) if (ch->isToken && ch->token.type == TokenType::RBRACE) braceLine = ch->token.line;
                            ReportSemanticError(ErrorType::MISSING_RETURN, braceLine);
                        }
                    }
                    currentFunctionRet = prevFunctionRet;
                    currentFunctionHasReturn = prevHasReturn;
                }
            } else {
                CheckPass(c.get());
            }
        }
        return;
    }

    if (node->name == "Block") {
        auto it = nodeScopeMap.find(node);
        if (it == nodeScopeMap.end()) {
            // no scope created? just descend
            for (const auto &c : node->children) CheckPass(c.get());
            return;
        }
        int prev = currentTable;
        currentTable = it->second;
        // process block items: declarations already inserted; check decl initializers and statements
        for (const auto &bi : node->children) {
            if (bi->name != "BlockItem") continue;
            if (bi->children.empty()) continue;
            const ASTNode *inner = bi->children[0].get();
            if (inner->name == "Decl") {
                // check initializer expressions inside Decl (VarDef initializers or ConstDef)
                for (const auto &gc : inner->children) {
                    CheckPass(gc.get());
                }
            } else if (inner->name == "Stmt") {
                // use CheckStmt (does not create scopes)
                CheckStmt(inner);
            } else {
                CheckPass(inner);
            }
        }
        currentTable = prev;
        return;
    }

    // default descent
    for (const auto &c : node->children) CheckPass(c.get());
}

// CheckStmt mirrors HandleStmt logic but without creating or exiting scopes.
void SemanticAnalyzer::CheckStmt(const ASTNode *node) {
    if (!node) return;
    if (!node->children.empty() && node->children[0]->isToken) {
        auto tk = node->children[0]->token;
        if (tk.type == TokenType::RETURNTK) {
            if (currentFunctionRet == 0) {
                bool hasExpr = false;
                for (const auto &ch : node->children) if (ch && ch->name == "Exp") { hasExpr = true; break; }
                if (hasExpr) ReportSemanticError(ErrorType::RETURN_VALUE_IN_VOID, tk.line);
            }
            currentFunctionHasReturn = true;
            return;
        }
        if (tk.type == TokenType::BREAKTK || tk.type == TokenType::CONTINUETK) {
            if (loopDepth <= 0) ReportSemanticError(ErrorType::BAD_BREAK_CONTINUE, tk.line);
            return;
        }
        if (tk.type == TokenType::PRINTFTK) {
            int exprCount = 0;
            int placeholder = 0;
            bool seenStr = false;
            for (const auto &c : node->children) {
                if (c->isToken && c->token.type == TokenType::STRCON) {
                    if (!seenStr) {
                        const std::string &s = c->token.value;
                        for (size_t i = 0; i + 1 < s.size(); ++i) if (s[i] == '%' && s[i+1] == 'd') placeholder++;
                        seenStr = true;
                    }
                    continue;
                }
                if (seenStr) {
                    if (c->name == "Exp") exprCount++;
                }
            }
            if (placeholder != exprCount) ReportSemanticError(ErrorType::PRINTF_MISMATCH, tk.line);
            for (const auto &c : node->children) HandleExp(c.get());
            return;
        }
    }

    // ForStmt and similar: process children in textual order. Blocks are
    // handled inline (CheckPass will set currentTable based on nodeScopeMap).
    if (node->name == "ForStmt" || (!node->children.empty() && node->children[0]->isToken && node->children[0]->token.type == TokenType::FORTK)) {
        loopDepth++;
        // Similar to HandleStmt: detect LVal '=' patterns in for header and
        // perform assignment checks here in the CheckPass phase as well.
        for (size_t i = 0; i < node->children.size(); ++i) {
            const auto &c = node->children[i];
            if (!c) continue;
            auto checkNodeForAssign = [&](const ASTNode* cand)->void {
                if (!cand) return;
                if (cand->name == "LVal") {
                    if (!cand->children.empty() && cand->children[0]->isToken) {
                        auto tk = cand->children[0]->token;
                        Symbol *s = Lookup(tk.value);
                        if (!s) {
                            if (!IsBuiltin(tk.value)) ReportSemanticError(ErrorType::UNDEFINED, tk.line);
                        } else {
                            if (s->isConst) ReportSemanticError(ErrorType::ASSIGN_TO_CONST, tk.line);
                        }
                    }
                    return;
                }
                if (cand->name == "Exp" && cand->children.size() >= 2) {
                    const ASTNode *first = cand->children[0].get();
                    const ASTNode *second = cand->children[1].get();
                    if (first && first->name == "LVal" && second && second->isToken && second->token.type == TokenType::ASSIGN) {
                        if (!first->children.empty() && first->children[0]->isToken) {
                            auto tk = first->children[0]->token;
                            Symbol *s = Lookup(tk.value);
                            if (!s) {
                                if (!IsBuiltin(tk.value)) ReportSemanticError(ErrorType::UNDEFINED, tk.line);
                            } else {
                                if (s->isConst) ReportSemanticError(ErrorType::ASSIGN_TO_CONST, tk.line);
                            }
                        }
                    }
                }
            };

            checkNodeForAssign(c.get());
            if (c->name == "ForStmt") {
                for (const auto &fc : c->children) {
                    if (!fc) continue;
                    checkNodeForAssign(fc.get());
                    if (fc->name == "Block") CheckPass(fc.get());
                    else HandleExp(fc.get());
                }
                continue;
            }
            if (c->name == "LVal" && i + 1 < node->children.size() && node->children[i+1]->isToken && node->children[i+1]->token.type == TokenType::ASSIGN) {
                checkNodeForAssign(c.get());
            }
            if (c->name == "Exp") checkNodeForAssign(c.get());

            if (c->name == "Block") CheckPass(c.get());
            else HandleExp(c.get());
        }
        loopDepth--;
        return;
    }

    // assignment-like: LVal '=' Exp
    if (!node->children.empty() && node->children[0]->name == "LVal") {
        const ASTNode *lval = node->children[0].get();
        if (!lval->children.empty() && lval->children[0]->isToken) {
            auto tk = lval->children[0]->token;
            Symbol *s = Lookup(tk.value);
            if (!s) {
                if (!IsBuiltin(tk.value)) ReportSemanticError(ErrorType::UNDEFINED, tk.line);
            } else {
                if (s->isConst) ReportSemanticError(ErrorType::ASSIGN_TO_CONST, tk.line);
            }
        }
        // check RHS expressions
        for (const auto &c : node->children) HandleExp(c.get());
        return;
    }

    // generic: descend into blocks specially, other children use HandleExp
    for (const auto &c : node->children) {
        if (c->name == "Block") CheckPass(c.get());
        else HandleExp(c.get());
        if (c->name == "Stmt") CheckStmt(c.get());
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
