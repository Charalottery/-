#include "IrType.hpp"
#include "IrBaseType.hpp"
#include "IrPointerType.hpp"
#include "IrArrayType.hpp"
#include "IrFunctionType.hpp"

bool IrType::isInt1() const {
    const auto* base = dynamic_cast<const IrBaseType*>(this);
    return base && base->kind == IrBaseType::INT1;
}
bool IrType::isInt8() const {
    const auto* base = dynamic_cast<const IrBaseType*>(this);
    return base && base->kind == IrBaseType::INT8;
}
bool IrType::isInt32() const {
    const auto* base = dynamic_cast<const IrBaseType*>(this);
    return base && base->kind == IrBaseType::INT32;
}
bool IrType::isVoid() const {
    const auto* base = dynamic_cast<const IrBaseType*>(this);
    return base && base->kind == IrBaseType::VOID;
}
bool IrType::isLabel() const {
    const auto* base = dynamic_cast<const IrBaseType*>(this);
    return base && base->kind == IrBaseType::LABEL;
}
bool IrType::isPointer() const {
    return dynamic_cast<const IrPointerType*>(this) != nullptr;
}
bool IrType::isArray() const {
    return dynamic_cast<const IrArrayType*>(this) != nullptr;
}
bool IrType::isFunction() const {
    return dynamic_cast<const IrFunctionType*>(this) != nullptr;
}
