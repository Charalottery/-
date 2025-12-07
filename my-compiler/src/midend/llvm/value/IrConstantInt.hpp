#pragma once
#include "IrConstant.hpp"
#include "../type/IrBaseType.hpp"

class IrConstantInt : public IrConstant {
public:
    int value;

    IrConstantInt(IrType* t, int v) : IrConstant(t, std::to_string(v)), value(v) {}

    static IrConstantInt* get(int v) {
        return new IrConstantInt(IrBaseType::getInt32(), v);
    }
    
    static IrConstantInt* get1(bool v) {
        return new IrConstantInt(IrBaseType::getInt1(), v ? 1 : 0);
    }

    std::string toString() const override;
};
