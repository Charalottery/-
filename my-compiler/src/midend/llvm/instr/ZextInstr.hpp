#pragma once
#include "Instr.hpp"

class ZextInstr : public Instr {
public:
    ZextInstr(IrValue* val, IrType* destTy, std::string n) 
        : Instr(destTy, InstrType::ZEXT, n) {
        addOperand(val);
    }

    std::string toString() const override;
};
