#include "IrConstantInt.hpp"
#include "IrConstantArray.hpp"
#include "IrGlobalValue.hpp"
#include "IrFunction.hpp"
#include "IrBasicBlock.hpp"
#include "../instr/Instr.hpp"
#include "../type/IrFunctionType.hpp"
#include "../type/IrPointerType.hpp"
#include "../type/IrArrayType.hpp"
#include <sstream>

std::string IrConstantInt::toString() const {
    return type->toString() + " " + std::to_string(value);
}

std::string IrConstantArray::toString() const {
    std::stringstream ss;
    ss << type->toString() << " [";
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << elements[i]->toString();
    }
    ss << "]";
    return ss.str();
}

std::string IrGlobalValue::toString() const {
    std::stringstream ss;
    ss << name << " = " << (isConst ? "constant " : "global ");
    if (initVal) {
        ss << initVal->toString(); 
    } else {
        ss << ((IrPointerType*)type)->pointedType->toString() << " zeroinitializer";
    }
    return ss.str();
}

std::string IrFunction::toString() const {
    std::stringstream ss;
    if (isBuiltin) {
        ss << "declare " << ((IrFunctionType*)type)->returnType->toString() << " " << name << "(";
    } else {
        ss << "define " << ((IrFunctionType*)type)->returnType->toString() << " " << name << "(";
    }
    
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << params[i]->type->toString() << " " << params[i]->name;
    }
    
    if (isBuiltin) {
        ss << ")";
    } else {
        ss << ") {\n";
        for (auto* bb : blocks) {
            ss << bb->toString();
        }
        ss << "}";
    }
    return ss.str();
}

std::string IrBasicBlock::toString() const {
    std::stringstream ss;
    std::string label = name;
    if (label.size() > 0 && label[0] == '%') label = label.substr(1);
    
    ss << label << ":\n"; 
    for (auto* instr : instructions) {
        ss << "  " << instr->toString() << "\n";
    }
    return ss.str();
}
