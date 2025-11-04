#pragma once
#include <string>
#include "ErrorType.hpp"

struct Error {
    ErrorType type;
    int line;
    std::string message;

    Error(ErrorType t, int l, std::string m = "") : type(t), line(l), message(std::move(m)) {}
};
