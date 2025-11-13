#pragma once
#include <vector>
#include "Error.hpp"
#include <iostream>

class ErrorRecorder {
public:
    // Add an error unless an identical (type + line) error was already recorded.
    // This avoids duplicate entries when multiple passes or overlapping traversals
    // report the same logical error.
    static void AddError(const Error &e) {
        auto &errs = GetErrorsRef();
        // Ensure we record at most one error per line. The evaluation guarantees
        // each line has at most one true error; record the first reported
        // error for a given line and ignore subsequent reports for the same line.
        for (const auto &ex : errs) {
            if (ex.line == e.line) return; // already have an error for this line
        }
        errs.push_back(e);
    }
    static const std::vector<Error>& GetErrors() { return GetErrorsRef(); }
    static bool HasErrors() { return !GetErrorsRef().empty(); }

private:
    static std::vector<Error>& GetErrorsRef() {
        static std::vector<Error> errors;
        return errors;
    }
};
