#pragma once
#include "Instr.hpp"
#include "../type/IrPointerType.hpp"
#include "../type/IrArrayType.hpp"

class GepInstr : public Instr {
public:
    GepInstr(IrValue* ptr, const std::vector<IrValue*>& indices, std::string n);

    std::string toString() const override;
};
