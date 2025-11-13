#pragma once
#include "SymbolTable.hpp"
#include "Symbol.hpp"
#include <string>
#include <vector>

class SymbolManager {
public:
    static void Init();
    static SymbolTable* GetRoot();
    static SymbolTable* GetCurrent();
    static void CreateScope();
    static void ExitScope();
    static void AddSymbol(const Symbol &s);
    static Symbol* Lookup(const std::string &name);
    static std::vector<SymbolTable*> GetAllTablesInIdOrder();
private:
    static SymbolTable* root;
    static SymbolTable* current;
    static int nextId; // next scope id to assign
};
