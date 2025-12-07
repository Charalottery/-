#pragma once
#include "Instr.hpp"
#include "../value/IrBasicBlock.hpp"

class JumpInstr : public Instr {
public:
    JumpInstr(IrBasicBlock* target) 
        : Instr(IrBaseType::getVoid(), InstrType::JUMP) {
        addOperand(target);
    }

    std::string toString() const override;
};
