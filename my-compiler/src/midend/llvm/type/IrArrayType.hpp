#pragma once
#include "IrType.hpp"

class IrArrayType : public IrType {
public:
    IrType* elementType;
    int numElements;

    IrArrayType(IrType* element, int num) : elementType(element), numElements(num) {}

    std::string toString() const override {
        return "[" + std::to_string(numElements) + " x " + elementType->toString() + "]";
    }
};
