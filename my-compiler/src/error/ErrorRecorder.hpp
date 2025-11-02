#pragma once
#include <vector>
#include "error/Error.hpp"

class ErrorRecorder {
public:
    // 向记录器中添加一条错误
    static void AddError(const Error &e) { GetErrorsRef().push_back(e); }
    // 获取当前记录的错误列表（只读引用）
    static const std::vector<Error>& GetErrors() { return GetErrorsRef(); }
    // 判断是否记录了错误
    static bool HasErrors() { return !GetErrorsRef().empty(); }

private:
    static std::vector<Error>& GetErrorsRef() {
        // 使用函数内静态变量实现简单的单例错误向量，保证全局访问一致性
        static std::vector<Error> errors;
        return errors;
    }
};
