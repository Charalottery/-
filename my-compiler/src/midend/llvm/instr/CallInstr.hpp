#pragma once
#include "Instr.hpp"
#include "../value/IrFunction.hpp"
#include "../type/IrFunctionType.hpp"
#include "../type/IrPointerType.hpp"
#include <vector>

class CallInstr : public Instr {
public:
    CallInstr(IrFunction* func, const std::vector<IrValue*>& args, std::string n = "") 
        : Instr(func->type->isFunction() ? ((IrFunctionType*)func->type)->returnType : ((IrFunctionType*)((IrPointerType*)func->type)->pointedType)->returnType, InstrType::CALL, n) {
        addOperand(func);
        for (auto arg : args) addOperand(arg);
    }

    std::string toString() const override;
};
