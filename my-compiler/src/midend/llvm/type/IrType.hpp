#pragma once
#include <string>
#include <memory>

class IrType {
public:
    virtual ~IrType() = default;
    virtual std::string toString() const = 0;

    bool isInt1() const;
    bool isInt8() const;
    bool isInt32() const;
    bool isVoid() const;
    bool isPointer() const;
    bool isArray() const;
    bool isFunction() const;
    bool isLabel() const; // For BasicBlock
};
