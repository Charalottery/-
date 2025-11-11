#pragma once

enum class ErrorType {
    ILLEGAL_SYMBOL,
    MISS_SEMICN, // missing semicolon ';' (i)
    MISS_RPARENT, // missing right parenthesis ')' (j)
    MISS_RBRACK, // missing right bracket ']' (k)

    // semantic errors:
    REDEFINE, // b
    UNDEFINED, // c
    FUNC_PARAM_COUNT, // d
    FUNC_PARAM_TYPE, // e
    RETURN_VALUE_IN_VOID, // f
    MISSING_RETURN, // g
    ASSIGN_TO_CONST, // h
    PRINTF_MISMATCH, // l
    BAD_BREAK_CONTINUE // m
};
