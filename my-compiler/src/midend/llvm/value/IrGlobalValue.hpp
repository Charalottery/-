#pragma once
#include "IrConstant.hpp"
#include "../type/IrPointerType.hpp"

class IrGlobalValue : public IrConstant {
public:
    IrConstant* initVal;
    bool isConst;

    IrGlobalValue(IrType* t, std::string n, IrConstant* init, bool isC = false) 
        : IrConstant(new IrPointerType(t), n), initVal(init), isConst(isC) {}

    std::string toString() const override;
};
