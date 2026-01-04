#pragma once
#include "Instr.hpp"
#include "../value/IrBasicBlock.hpp"

#include <vector>

class PhiInstr : public Instr
{
public:
    struct Incoming
    {
        IrBasicBlock *block;
        IrValue *value;
    };

    explicit PhiInstr(IrType *t, std::string n)
        : Instr(t, InstrType::PHI, n)
    {
    }

    void addIncoming(IrValue *v, IrBasicBlock *from)
    {
        incoming.push_back({from, v});
        addOperand(v);
        addOperand(from);
    }

    IrValue *getIncomingValue(IrBasicBlock *from) const
    {
        for (const auto &inc : incoming)
        {
            if (inc.block == from)
                return inc.value;
        }
        return nullptr;
    }

    std::string toString() const override;

private:
    std::vector<Incoming> incoming;
};
