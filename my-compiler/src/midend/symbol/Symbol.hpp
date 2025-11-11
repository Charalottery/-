#pragma once
#include <string>

struct Symbol {
    int id = -1;         // optional token id (unused currently)
    int tableId = -1;    // the symbol table id this symbol belongs to
    std::string token;   // identifier name or literal string
    int type = 0;        // 0 -> var, 1 -> array, 2 -> func, 3 -> static var
    int btype = 0;       // 0 -> int (future: char)
    int con = 1;         // 0 -> const, 1 -> var

    // function specific
    int retype = 1;      // 0 -> void, 1 -> int
    int paramNum = 0;
    // keep param types simple as ints
};
