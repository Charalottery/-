#pragma once
#include "Instr.hpp"

class TruncInstr : public Instr {
public:
    TruncInstr(IrValue* val, IrType* destTy, std::string n) 
        : Instr(destTy, InstrType::TRUNC, n) {
        addOperand(val);
    }

    std::string toString() const override;
};
