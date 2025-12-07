#pragma once
#include "Instr.hpp"
#include "../value/IrBasicBlock.hpp"

class BranchInstr : public Instr {
public:
    BranchInstr(IrValue* cond, IrBasicBlock* trueBlock, IrBasicBlock* falseBlock) 
        : Instr(IrBaseType::getVoid(), InstrType::BR) {
        addOperand(cond);
        addOperand(trueBlock);
        addOperand(falseBlock);
    }

    std::string toString() const override;
};
