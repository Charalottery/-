#pragma once
#include "../type/IrType.hpp"
#include <string>
#include <vector>
#include <list>

class IrUse;
class IrUser;

class IrValue {
public:
    IrType* type;
    std::string name;
    std::list<IrUse*> useList; // Uses of this value

    IrValue(IrType* t, std::string n) : type(t), name(std::move(n)) {}
    virtual ~IrValue() = default;

    virtual std::string toString() const { return name; }

    void addUse(IrUse* use);
    void removeUse(IrUser* user); // Remove all uses by a specific user
    void replaceAllUsesWith(IrValue* newValue);

    std::string getName() const { return name; }
    IrType* getType() const { return type; }
};
