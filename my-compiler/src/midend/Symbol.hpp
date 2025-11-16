#pragma once
#include <string>
#include <vector>

struct Symbol {
    std::string name;
    std::string typeName; // output type name like "Int", "ConstIntArray", etc.
    bool isStatic = false;
    bool isConst = false;
    bool isFunction = false;
    std::vector<std::string> paramTypes; // for functions: parameter types in order
    // additional fields for later use (dimension, init values etc.)
    int dimension = 0;
    int line = -1;
    bool isBuiltin = false;
    bool isArray = false; // true if symbol is an array (dimension > 0)
    std::vector<bool> paramIsArray; // for function symbols: whether each param is array

    Symbol() = default;
    Symbol(std::string n, std::string t, bool s = false, int d = 0, bool isConst_ = false, int line_ = -1)
        : name(std::move(n)), typeName(std::move(t)), isStatic(s), isConst(isConst_), dimension(d), line(line_) {
            isArray = (dimension > 0);
    }
    Symbol(std::string n, std::string t, bool funcFlag, const std::vector<std::string> &params, int line_)
        : name(std::move(n)), typeName(std::move(t)), isStatic(false), isConst(false), isFunction(funcFlag), paramTypes(params), dimension(0), line(line_) {}
};
