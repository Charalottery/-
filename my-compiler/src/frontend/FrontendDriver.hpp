#pragma once

#include <string>

class FrontendDriver {
public:
    // 处理前端输入的静态接口。
    // 参数：
    //   input - 源代码内容（已读取为单一字符串）
    //   tokensOutPath - 用于输出词法/语法相关列表（例如 parser.txt）的文件路径
    //   errorOutPath - 用于输出错误列表（例如 error.txt）的文件路径
    // 返回值：
    //   0 - 成功执行且未记录任何语法/词法错误
    //   1 - 严重 IO 错误（例如无法写输出文件）
    //   2 - 处理过程中记录了错误（错误信息已写入 errorOutPath）
    static int Process(const std::string &input, const std::string &tokensOutPath, const std::string &errorOutPath);
};
