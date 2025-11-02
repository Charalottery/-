#include "ParserOutput.hpp"
#include <iostream>

ParserOutput::ParserOutput() {
    ofs.open("parser.txt", std::ofstream::out);
    if (!ofs.is_open()) {
        std::cerr << "ParserOutput: failed to open parser.txt for writing\n";
    } else {
        // opened successfully; keep silent to avoid noisy output
    }
}

ParserOutput &ParserOutput::Get() {
    static ParserOutput inst;
    return inst;
}

void ParserOutput::Write(const std::string &line) {
    if (ofs.is_open()) {
        ofs << line << "\n";
        ofs.flush();
    } else {
        std::cerr << "ParserOutput: write failed (ofs not open): " << line << "\n";
    }
}
