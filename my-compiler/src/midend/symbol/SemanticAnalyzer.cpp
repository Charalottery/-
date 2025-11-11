#include "SemanticAnalyzer.hpp"
#include <fstream>
#include <iostream>

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
    // default emit flag: check for file 'emit_symbol.txt' in cwd
    std::ifstream ef("emit_symbol.txt");
    if (!ef) emitSymbol = false; else {
        string v; ef >> v; if (v != "1") emitSymbol = false; else emitSymbol = true;
    }

    Walk(root);

    if (emitSymbol) {
        std::ofstream of("symbol.txt");
        // output by scope id ascending (tables are created in ascending id order)
        for (const auto &tbl : tables) {
            for (const auto &sym : tbl.symbols) {
                // format: 作用域序号 单词的字符/字符串形式 类型名称
                string tname;
                // determine type string per user mapping
                // mapping: ConstInt, Int, VoidFunc, ConstIntArray, IntArray, IntFunc, StaticInt, StaticIntArray
                if (sym.type == 2) { // func
                    tname = (sym.retype == 0) ? "VoidFunc" : "IntFunc";
                } else if (sym.type == 1) { // array
                    if (sym.con == 0) tname = "ConstIntArray";
                    else if (sym.type == 3) tname = "StaticIntArray"; // won't happen
                    else tname = "IntArray";
                } else { // var
                    if (sym.con == 0) tname = "ConstInt";
                    else if (sym.type == 3) tname = "StaticInt"; // static
                    else tname = "Int";
                }

                of << tbl.id << " " << sym.token << " " << tname << "\n";
            }
        }
    }
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
                s.type = 0; // var
                s.con = 0; // const
                // if array (has LBRACK) then type->array
                bool isArray = false;
                for (const auto &cc : c->children) if (cc->isToken && cc->token.type == TokenType::LBRACK) isArray = true;
                if (isArray) s.type = 1;
                tables[currentTable].Insert(s);
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
                s.con = 1; // var
                if (isStatic) s.type = 3; // mark static variant using type 3 for printing
                else s.type = 0;
                // check if array
                bool isArray = false;
                for (const auto &cc : c->children) if (cc->isToken && cc->token.type == TokenType::LBRACK) isArray = true;
                if (isArray) s.type = (isStatic ? 3 : 1);
                tables[currentTable].Insert(s);
            }
        }
    }
}

void SemanticAnalyzer::HandleFuncDef(const ASTNode *node, bool isMain) {
    // FuncDef children: FuncType IDENFR '(' [FuncFParams] ')' Block
    // find function name token
    std::string fname;
    int funcRet = 1; // int
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
        }
        if (c->name == "FuncFParams") paramsNode = c.get();
        if (c->name == "Block") blkNode = c.get();
    }

    if (!isMain && !fname.empty()) {
        // insert function into current (global) scope
        Symbol s;
        s.token = fname;
        s.tableId = tables[currentTable].id;
        s.type = 2; // func
        s.retype = funcRet;
        // count parameters if present
        if (paramsNode) {
            int count = 0;
            for (const auto &p : paramsNode->children) if (p->name == "FuncFParam") ++count;
            s.paramNum = count;
        }
        tables[currentTable].Insert(s);
    }

    // function body: create new scope for function internal variables
    NewScope();
    // add parameters as symbols in the function scope
    if (paramsNode) HandleFuncFParams(paramsNode);

    // walk into block (which will create nested scopes for nested blocks)
    if (blkNode) HandleBlock(blkNode);

    ExitScope();
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
        s.con = 1;
        s.type = isArray ? 1 : 0;
        tables[currentTable].Insert(s);
    }
}

void SemanticAnalyzer::HandleBlock(const ASTNode *node) {
    // node children: '{' { BlockItem } '}'
    // create nested scope for this block
    NewScope();
    for (const auto &c : node->children) {
        if (c->name == "BlockItem") {
            // BlockItem -> Decl | Stmt
            if (!c->children.empty()) {
                const ASTNode *inner = c->children[0].get();
                if (inner->name == "Decl") HandleDecl(inner);
                else {
                    // for statements that contain nested blocks, traverse deeper
                    // but we must also inspect children to find inner Blocks
                    for (const auto &gc : inner->children) {
                        if (gc->name == "Block") HandleBlock(gc.get());
                        // for statements like if/for which have nested Stmt that could be Block
                        if (gc->name == "Stmt") HandleBlock(gc.get());
                    }
                }
            }
        }
    }
    ExitScope();
}
