#pragma once
#include "Instr.hpp"
#include "../type/IrBaseType.hpp"

enum class IcmpCond {
    EQ, NE, SGT, SGE, SLT, SLE
};

class IcmpInstr : public Instr {
public:
    IcmpCond cond;

    IcmpInstr(IcmpCond c, IrValue* lhs, IrValue* rhs, std::string n) 
        : Instr(IrBaseType::getInt1(), InstrType::ICMP, n), cond(c) {
        addOperand(lhs);
        addOperand(rhs);
    }

    std::string toString() const override;
};
