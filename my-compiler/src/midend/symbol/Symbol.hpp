#pragma once
#include <string>
#include <vector>


struct Symbol {
    enum class Kind { VAR = 0, ARRAY = 1, FUNC = 2 };
    int id = -1;         // optional token id (unused currently)
    int tableId = -1;    // the symbol table id this symbol belongs to
    std::string token;   // identifier name or literal string
    Kind kind = Kind::VAR; // VAR, ARRAY, FUNC
    int btype = 0;       // 0 -> int (future: char)
    bool isConst = false; // true -> const, false -> var
    bool isStatic = false; // true -> static storage

    // function specific
    int retype = 1;      // 0 -> void, 1 -> int
    int paramNum = 0;
    std::vector<Kind> paramTypes; // for functions: kinds of params
    int line = -1; // declaration line
    // keep param types simple as ints
};
