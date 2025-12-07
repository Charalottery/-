#include "IrModule.hpp"

void IrModule::print(std::ostream& os) const {
    // Print global variables
    for (auto* gv : globalValues) {
        os << gv->toString() << "\n";
    }
    if (!globalValues.empty()) os << "\n";

    // Print functions
    for (auto* func : functions) {
        os << func->toString() << "\n";
    }
}
