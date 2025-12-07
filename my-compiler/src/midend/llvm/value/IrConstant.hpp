#pragma once
#include "IrUser.hpp"

class IrConstant : public IrUser {
public:
    IrConstant(IrType* t, std::string n) : IrUser(t, n) {}
};
