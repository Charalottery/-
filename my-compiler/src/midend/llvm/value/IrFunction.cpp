#include "IrFunction.hpp"
#include "../type/IrFunctionType.hpp"
#include "../type/IrPointerType.hpp"

IrFunction::IrFunction(IrType* returnType, const std::vector<IrType*>& paramTypes, std::string n, bool isBuiltin)
    : IrGlobalValue(new IrFunctionType(returnType, paramTypes), n, nullptr), isBuiltin(isBuiltin) {
    // Function type is actually a pointer to function type in some contexts, but here we keep it simple
    // In LLVM, function name is a global value (pointer)
    // We construct params
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        params.push_back(new IrValue(paramTypes[i], "%arg" + std::to_string(i)));
    }
}

void IrFunction::addBasicBlock(IrBasicBlock* bb) {
    blocks.push_back(bb);
}
