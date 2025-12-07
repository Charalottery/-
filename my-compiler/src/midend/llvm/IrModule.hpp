#pragma once
#include "value/IrGlobalValue.hpp"
#include "value/IrFunction.hpp"
#include <vector>
#include <string>
#include <iostream>

class IrModule {
public:
    std::vector<IrGlobalValue*> globalValues;
    std::vector<IrFunction*> functions;

    void addGlobalValue(IrGlobalValue* gv) { globalValues.push_back(gv); }
    void addFunction(IrFunction* f) { functions.push_back(f); }

    void print(std::ostream& os) const;
};
