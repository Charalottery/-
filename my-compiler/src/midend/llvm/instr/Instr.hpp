#pragma once
#include "../value/IrUser.hpp"
#include "InstrType.hpp"

class IrBasicBlock;

class Instr : public IrUser {
public:
    InstrType instrType;
    IrBasicBlock* parentBlock;

    Instr(IrType* t, InstrType it, std::string n = "") 
        : IrUser(t, (n.empty() || n[0] == '%') ? n : "%" + n), instrType(it), parentBlock(nullptr) {}
    
    virtual std::string toString() const = 0;
};
