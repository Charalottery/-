#pragma once
#include "Instr.hpp"

class LoadInstr : public Instr {
public:
    LoadInstr(IrValue* ptr, std::string n);

    std::string toString() const override;
};
