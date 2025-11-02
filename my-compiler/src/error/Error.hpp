#pragma once
#include <string>
#include "error/ErrorType.hpp"

struct Error {
    ErrorType type;
    int line;
    std::string message;
    // Error 表示一次在词法/语法分析期间记录的错误信息
    // 字段：
    //   type - 错误类型（例如非法字符、缺少分号等）
    //   line - 错误发生的行号（用于排序/输出）
    //   message - 可选的错误详细文本信息（目前主要用于调试）
    Error(ErrorType t, int l, std::string m = "") : type(t), line(l), message(std::move(m)) {}
};
