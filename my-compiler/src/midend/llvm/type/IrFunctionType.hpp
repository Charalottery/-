#pragma once
#include "IrType.hpp"
#include <vector>

class IrFunctionType : public IrType {
public:
    IrType* returnType;
    std::vector<IrType*> paramTypes;

    IrFunctionType(IrType* ret, const std::vector<IrType*>& params) 
        : returnType(ret), paramTypes(params) {}

    std::string toString() const override {
        // Function type string representation is usually not printed directly in LLVM IR type position
        // but we can provide one.
        return returnType->toString(); 
    }
};
