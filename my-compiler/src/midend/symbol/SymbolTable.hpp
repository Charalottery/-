#pragma once
#include "Symbol.hpp"
#include <string>
#include <vector>
#include <unordered_map>

struct SymbolTable {
    int id = -1;
    int fatherId = 0;
    std::vector<Symbol> symbols; // preserve insertion order
    std::unordered_map<std::string, int> directory; // name -> index in symbols

    // insert symbol; returns index in symbols
    int Insert(const Symbol &s) {
        auto it = directory.find(s.token);
        if (it != directory.end()) return -1; // already exists in this table
        symbols.push_back(s);
        int idx = (int)symbols.size() - 1;
        directory[s.token] = idx;
        return idx;
    }

    // find symbol in this table only; returns pointer or nullptr
    Symbol *FindLocal(const std::string &name) {
        auto it = directory.find(name);
        if (it == directory.end()) return nullptr;
        return &symbols[it->second];
    }
};
