#pragma once
#include "IrValue.hpp"
#include "IrUse.hpp"
#include <vector>

class IrUser : public IrValue {
public:
    std::vector<IrUse*> operandList; // Values used by this user

    IrUser(IrType* t, std::string n) : IrValue(t, n) {}

    void addOperand(IrValue* v);
    IrValue* getOperand(int i) const { return operandList[i]->value; }
};
