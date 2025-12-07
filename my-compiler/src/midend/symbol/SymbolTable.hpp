#pragma once
#include "Symbol.hpp"
#include <vector>
#include <memory>

struct SymbolTable {
    int id = 0;
    SymbolTable *parent = nullptr;
    std::vector<std::unique_ptr<SymbolTable>> children;
    std::vector<Symbol> symbols; // preserve insertion order
    
    mutable int nextChildToVisit = 0; // For traversal

    explicit SymbolTable(int id_, SymbolTable *parent_) : id(id_), parent(parent_) {}

    void AddSymbol(const Symbol &s) { symbols.push_back(s); }

    bool HasSymbol(const std::string &name) const {
        for (const auto &s : symbols) if (s.name == name) return true;
        return false;
    }

    // return pointer to symbol in this local table, or nullptr
    Symbol* GetLocalSymbol(const std::string &name) {
        for (auto &s : symbols) if (s.name == name) return &s;
        return nullptr;
    }

    SymbolTable *CreateChild(int childId) {
        children.emplace_back(std::make_unique<SymbolTable>(childId, this));
        return children.back().get();
    }
};
