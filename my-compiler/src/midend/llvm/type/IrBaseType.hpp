#pragma once
#include "IrType.hpp"

class IrBaseType : public IrType {
public:
    enum Kind { INT1, INT8, INT32, VOID, LABEL };
    Kind kind;

    explicit IrBaseType(Kind k) : kind(k) {}

    std::string toString() const override {
        switch (kind) {
            case INT1: return "i1";
            case INT8: return "i8";
            case INT32: return "i32";
            case VOID: return "void";
            case LABEL: return "label";
            default: return "unknown";
        }
    }

    static IrBaseType* getInt1() { static IrBaseType t(INT1); return &t; }
    static IrBaseType* getInt8() { static IrBaseType t(INT8); return &t; }
    static IrBaseType* getInt32() { static IrBaseType t(INT32); return &t; }
    static IrBaseType* getVoid() { static IrBaseType t(VOID); return &t; }
    static IrBaseType* getLabel() { static IrBaseType t(LABEL); return &t; }
};
