#pragma once
#include "IrValue.hpp"

class IrUse {
public:
    IrUser* user;
    IrValue* value;

    IrUse(IrUser* u, IrValue* v) : user(u), value(v) {}
};
