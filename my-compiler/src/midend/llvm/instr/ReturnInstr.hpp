#pragma once
#include "Instr.hpp"

class ReturnInstr : public Instr {
public:
    ReturnInstr(IrValue* val = nullptr) 
        : Instr(IrBaseType::getVoid(), InstrType::RET) {
        if (val) addOperand(val);
    }

    std::string toString() const override;
};
