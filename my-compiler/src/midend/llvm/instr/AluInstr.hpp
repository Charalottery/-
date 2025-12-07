#pragma once
#include "Instr.hpp"
#include "../type/IrBaseType.hpp"

class AluInstr : public Instr {
public:
    AluInstr(InstrType op, IrValue* lhs, IrValue* rhs, std::string n) 
        : Instr(IrBaseType::getInt32(), op, n) {
        addOperand(lhs);
        addOperand(rhs);
    }

    std::string toString() const override;
};
