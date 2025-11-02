#include "frontend/FrontendDriver.hpp"
#include <fstream>
#include <iostream>

// 程序入口
// 该 main 负责读取输入文件（默认 testfile.txt），将文件内容作为字符串传递给前端驱动进行处理。
// 处理结果为两个输出文件：parser.txt（词法/语法输出）和 error.txt（错误列表，如果有的话）。
int main() {
    // 默认输入文件名（可以改为通过命令行参数传入）
    const char *infile = "testfile.txt";
    // 以二进制方式打开文件，保证读取原始字节（避免换行转换）
    std::ifstream fin(infile, std::ios::binary);
    if (!fin) {
        // 无法打开输入文件则写错误并返回错误码 1
        std::cerr << "Cannot open input file: " << infile << "\n";
        return 1;
    }

    // 将文件全部读入字符串
    std::string input((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
    // 如果文件以 UTF-8 BOM (0xEF,0xBB,0xBF) 开头，则去掉 BOM
    if (input.size() >= 3 && static_cast<unsigned char>(input[0]) == 0xEF
        && static_cast<unsigned char>(input[1]) == 0xBB
        && static_cast<unsigned char>(input[2]) == 0xBF) {
        input.erase(0, 3);
    }

    // 前端输出路径（parser.txt 用于记录 tokens/结构，error.txt 用于记录错误）
    const std::string tokensOut = "parser.txt"; // 词法/语法输出
    const std::string errorOut = "error.txt";  // 错误输出

    // 调用前端驱动进行处理，返回值含义由 FrontendDriver::Process 定义
    int res = FrontendDriver::Process(input, tokensOut, errorOut);
    // 保持原有行为：如果出现 IO 错误返回 1，否则返回 0（即便有语法/语义错误，会通过 error.txt 报告）
    return (res == 1) ? 1 : 0;
}
