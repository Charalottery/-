#pragma once

enum class ErrorType {
    ILLEGAL_SYMBOL,
    MISS_SEMICN, // missing semicolon ';' (i)
    MISS_RPARENT, // missing right parenthesis ')' (j)
    MISS_RBRACK // missing right bracket ']' (k)
};
