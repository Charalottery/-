#pragma once
#include "IrUser.hpp"
#include <list>
#include <string>

class IrFunction;
class Instr;

class IrBasicBlock : public IrValue {
public:
    IrFunction* parent;
    std::list<Instr*> instructions;

    IrBasicBlock(std::string n, IrFunction* p);
    
    void addInstr(Instr* instr);

    std::string toString() const override;
};
