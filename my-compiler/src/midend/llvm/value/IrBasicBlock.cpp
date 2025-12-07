#include "IrBasicBlock.hpp"
#include "IrFunction.hpp"
#include "../instr/Instr.hpp"
#include "../type/IrBaseType.hpp"

IrBasicBlock::IrBasicBlock(std::string n, IrFunction* p) 
    : IrValue(IrBaseType::getLabel(), n), parent(p) {
}

void IrBasicBlock::addInstr(Instr* instr) {
    instructions.push_back(instr);
}
