#pragma once
#include <vector>
#include "Error.hpp"

class ErrorRecorder {
public:
    static void AddError(const Error &e) { GetErrorsRef().push_back(e); }
    static const std::vector<Error>& GetErrors() { return GetErrorsRef(); }
    static bool HasErrors() { return !GetErrorsRef().empty(); }
    // Dump recorded errors to a file (defaults to "error.txt").
    // Behavior: preserve detection order, keep only first error per line,
    // sort by line ascending, and write lines in format: "<line> <code>\n".
    static void DumpErrors(const std::string &outfile = "error.txt");

private:
    static std::vector<Error>& GetErrorsRef() {
        static std::vector<Error> errors;
        return errors;
    }
};
