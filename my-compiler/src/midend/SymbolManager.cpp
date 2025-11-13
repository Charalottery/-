#include "SymbolManager.hpp"
#include "../error/ErrorRecorder.hpp"
#include "../error/ErrorType.hpp"
#include "../error/Error.hpp"
#include <algorithm>

SymbolTable* SymbolManager::root = nullptr;
SymbolTable* SymbolManager::current = nullptr;
int SymbolManager::nextId = 1;

void SymbolManager::Init() {
    // root scope id = 1
    nextId = 1;
    root = new SymbolTable(nextId, nullptr);
    current = root;
}

SymbolTable* SymbolManager::GetRoot() { return root; }
SymbolTable* SymbolManager::GetCurrent() { return current; }

void SymbolManager::CreateScope() {
    // assign new id as increment of nextId
    ++nextId;
    SymbolTable* child = current->CreateChild(nextId);
    current = child;
}

void SymbolManager::ExitScope() {
    if (current->parent) current = current->parent;
}

void SymbolManager::AddSymbol(const Symbol &s) {
    if (!current) return;
    // check duplicate in current scope
    if (current->HasSymbol(s.name)) {
        int line = s.line >= 0 ? s.line : 0;
        ErrorRecorder::AddError(Error(ErrorType::NAME_REDEFINE, line));
        return;
    }
    current->AddSymbol(s);
}

// simple lookup: search up the scope chain for name
Symbol* SymbolManager::Lookup(const std::string &name) {
    SymbolTable* t = current;
    while (t) {
        for (auto &s : t->symbols) {
            if (s.name == name) {
                return &const_cast<Symbol&>(s);
            }
        }
        t = t->parent;
    }
    return nullptr;
}

std::vector<SymbolTable*> SymbolManager::GetAllTablesInIdOrder() {
    std::vector<SymbolTable*> out;
    // perform DFS gathering pointers; then sort by id
    std::vector<SymbolTable*> stack;
    if (root) stack.push_back(root);
    while (!stack.empty()) {
        SymbolTable* t = stack.back(); stack.pop_back();
        out.push_back(t);
        for (auto &c : t->children) stack.push_back(c.get());
    }
    std::sort(out.begin(), out.end(), [](SymbolTable* a, SymbolTable* b){ return a->id < b->id; });
    return out;
}
