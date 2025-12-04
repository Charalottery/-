#include "ErrorRecorder.hpp"
#include <fstream>
#include <unordered_set>
#include <algorithm>

static char ErrorTypeToCode(ErrorType t) {
    switch (t) {
        case ErrorType::ILLEGAL_SYMBOL: return 'a';
        case ErrorType::MISS_SEMICN: return 'i';
        case ErrorType::MISS_RPARENT: return 'j';
        case ErrorType::MISS_RBRACK: return 'k';
        case ErrorType::NAME_REDEFINE: return 'b';
        case ErrorType::NAME_UNDEFINED: return 'c';
        case ErrorType::FUNC_PARAM_COUNT_MISMATCH: return 'd';
        case ErrorType::FUNC_PARAM_TYPE_MISMATCH: return 'e';
        case ErrorType::RETURN_IN_VOID: return 'f';
        case ErrorType::MISSING_RETURN: return 'g';
        case ErrorType::ASSIGN_TO_CONST: return 'h';
        case ErrorType::PRINTF_ARG_MISMATCH: return 'l';
        case ErrorType::BAD_BREAK_CONTINUE: return 'm';
        default: return '?';
    }
}

void ErrorRecorder::DumpErrors(const std::string &outfile) {
    const auto &all = GetErrors();
    if (all.empty()) return;

    // preserve detection order, keep only first error per line
    std::vector<Error> picked;
    std::unordered_set<int> seenLines;
    for (const auto &e : all) {
        if (seenLines.find(e.line) == seenLines.end()) {
            picked.push_back(e);
            seenLines.insert(e.line);
        }
    }

    // sort by line ascending
    std::sort(picked.begin(), picked.end(), [](const Error &a, const Error &b){ return a.line < b.line; });

    std::ofstream ef(outfile);
    if (!ef) return;
    for (const auto &e : picked) {
        char code = ErrorTypeToCode(e.type);
        ef << e.line << " " << code << "\n";
    }
}
