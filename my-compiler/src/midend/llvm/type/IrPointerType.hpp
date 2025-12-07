#pragma once
#include "IrType.hpp"

class IrPointerType : public IrType {
public:
    IrType* pointedType;

    explicit IrPointerType(IrType* pointed) : pointedType(pointed) {}

    std::string toString() const override {
        return pointedType->toString() + "*";
    }
};
