#pragma once

enum class ErrorType {
    // 错误类型枚举：注释中给出在 error.txt 中输出时对应的代码（a/i/j/k）
    ILLEGAL_SYMBOL,    // 非法字符/符号，对应输出代码: a
    MISSING_SEMICOLON, // 缺少分号，对应输出代码: i
    MISSING_RPAREN,    // 缺少右括号 ')', 对应输出代码: j
    MISSING_RBRACK     // 缺少右大括号 '}', 对应输出代码: k
};
