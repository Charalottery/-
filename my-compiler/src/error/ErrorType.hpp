#pragma once

enum class ErrorType {
    ILLEGAL_SYMBOL,
    MISS_SEMICN, // missing semicolon ';' (i)
    MISS_RPARENT, // missing right parenthesis ')' (j)
    MISS_RBRACK // missing right bracket ']' (k)
    , NAME_REDEFINE // b
    , NAME_UNDEFINED // c
    , FUNC_PARAM_COUNT_MISMATCH // d
    , FUNC_PARAM_TYPE_MISMATCH // e
    , RETURN_IN_VOID // f
    , MISSING_RETURN // g
    , ASSIGN_TO_CONST // h
    , PRINTF_ARG_MISMATCH // l
    , BAD_BREAK_CONTINUE // m
};
