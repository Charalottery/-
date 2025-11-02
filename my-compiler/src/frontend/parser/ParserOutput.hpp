#pragma once

#include <fstream>
#include <string>

class ParserOutput {
public:
    static ParserOutput &Get();
    void Write(const std::string &line);
private:
    ParserOutput();
    std::ofstream ofs;
};
