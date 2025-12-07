#pragma once

enum class InstrType {
    ADD, SUB, MUL, SDIV, SREM, // ALU
    ALLOCA, LOAD, STORE,
    ICMP,
    BR, // Conditional branch
    JUMP, // Unconditional branch
    CALL,
    RET,
    GEP, // GetElementPtr
    ZEXT,
    TRUNC,
    PHI
};
