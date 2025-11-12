#pragma once

#include "ErrorType.hpp"
#include <string>

inline std::string ErrorTypeToCode(ErrorType t) {
    switch (t) {
        case ErrorType::ILLEGAL_SYMBOL: return "a";
        case ErrorType::REDEFINE: return "b";
        case ErrorType::UNDEFINED: return "c";
        case ErrorType::FUNC_PARAM_COUNT: return "d";
        case ErrorType::FUNC_PARAM_TYPE: return "e";
        case ErrorType::RETURN_VALUE_IN_VOID: return "f";
        case ErrorType::MISSING_RETURN: return "g";
        case ErrorType::ASSIGN_TO_CONST: return "h";
        case ErrorType::MISS_SEMICN: return "i";
        case ErrorType::MISS_RPARENT: return "j";
        case ErrorType::MISS_RBRACK: return "k";
        case ErrorType::PRINTF_MISMATCH: return "l";
        case ErrorType::BAD_BREAK_CONTINUE: return "m";
        default: return "?";
    }
}
