#pragma once
#include "Instr.hpp"
#include "../type/IrPointerType.hpp"

class AllocaInstr : public Instr {
public:
    IrType* allocatedType;

    AllocaInstr(IrType* t, std::string n) 
        : Instr(new IrPointerType(t), InstrType::ALLOCA, n), allocatedType(t) {}

    std::string toString() const override;
};
