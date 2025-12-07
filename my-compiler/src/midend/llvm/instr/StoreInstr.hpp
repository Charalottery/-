#pragma once
#include "Instr.hpp"

class StoreInstr : public Instr {
public:
    StoreInstr(IrValue* val, IrValue* ptr);

    std::string toString() const override;
};
