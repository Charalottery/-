#pragma once
#include "IrConstant.hpp"
#include <vector>

class IrConstantArray : public IrConstant {
public:
    std::vector<IrConstant*> elements;

    IrConstantArray(IrType* t, const std::vector<IrConstant*>& elms) 
        : IrConstant(t, "array"), elements(elms) {}

    std::string toString() const override;
};
