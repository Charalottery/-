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
        for (const auto &ex : errs) {
            if (ex.type == e.type && ex.line == e.line) return; // duplicate, skip
        }
        // Do not print diagnostics to stderr during normal runs; errors are
        // recorded and written to error.txt by the caller. This avoids
        // producing additional log output during automated testing.
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
