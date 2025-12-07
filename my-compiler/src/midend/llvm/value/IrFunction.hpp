#pragma once
#include "IrGlobalValue.hpp"
#include "IrBasicBlock.hpp"
#include <list>
#include <vector>

class IrFunction : public IrGlobalValue {
public:
    std::list<IrBasicBlock*> blocks;
    std::vector<IrValue*> params; // Arguments
    bool isBuiltin;

    IrFunction(IrType* returnType, const std::vector<IrType*>& paramTypes, std::string n, bool isBuiltin = false);

    void addBasicBlock(IrBasicBlock* bb);

    std::string toString() const override;
};
