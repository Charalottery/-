#pragma once
#include <vector>
#include "Error.hpp"

class ErrorRecorder {
public:
    static void AddError(const Error &e) { GetErrorsRef().push_back(e); }
    static const std::vector<Error>& GetErrors() { return GetErrorsRef(); }
    static bool HasErrors() { return !GetErrorsRef().empty(); }

private:
    static std::vector<Error>& GetErrorsRef() {
        static std::vector<Error> errors;
        return errors;
    }
};
