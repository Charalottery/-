#include "../instr/AluInstr.hpp"
#include "../instr/AllocaInstr.hpp"
#include "../instr/LoadInstr.hpp"
#include "../instr/StoreInstr.hpp"
#include "../instr/IcmpInstr.hpp"
#include "../instr/BranchInstr.hpp"
#include "../instr/JumpInstr.hpp"
#include "../instr/CallInstr.hpp"
#include "../instr/ReturnInstr.hpp"
#include "../instr/GepInstr.hpp"
#include "../instr/ZextInstr.hpp"
#include "../instr/TruncInstr.hpp"
#include <sstream>

std::string AluInstr::toString() const {
    std::string opStr;
    switch (instrType) {
        case InstrType::ADD: opStr = "add"; break;
        case InstrType::SUB: opStr = "sub"; break;
        case InstrType::MUL: opStr = "mul"; break;
        case InstrType::SDIV: opStr = "sdiv"; break;
        case InstrType::SREM: opStr = "srem"; break;
        default: opStr = "unknown"; break;
    }
    return name + " = " + opStr + " " + getOperand(0)->type->toString() + " " + getOperand(0)->name + ", " + getOperand(1)->name;
}

std::string AllocaInstr::toString() const {
    return name + " = alloca " + allocatedType->toString();
}

LoadInstr::LoadInstr(IrValue* ptr, std::string n) 
    : Instr(((IrPointerType*)ptr->type)->pointedType, InstrType::LOAD, n) {
    addOperand(ptr);
}

std::string LoadInstr::toString() const {
    return name + " = load " + type->toString() + ", " + getOperand(0)->type->toString() + " " + getOperand(0)->name;
}

StoreInstr::StoreInstr(IrValue* val, IrValue* ptr) 
    : Instr(IrBaseType::getVoid(), InstrType::STORE) {
    addOperand(val);
    addOperand(ptr);
}

std::string StoreInstr::toString() const {
    return "store " + getOperand(0)->type->toString() + " " + getOperand(0)->name + ", " + getOperand(1)->type->toString() + " " + getOperand(1)->name;
}

std::string IcmpInstr::toString() const {
    std::string condStr;
    switch (cond) {
        case IcmpCond::EQ: condStr = "eq"; break;
        case IcmpCond::NE: condStr = "ne"; break;
        case IcmpCond::SGT: condStr = "sgt"; break;
        case IcmpCond::SGE: condStr = "sge"; break;
        case IcmpCond::SLT: condStr = "slt"; break;
        case IcmpCond::SLE: condStr = "sle"; break;
    }
    return name + " = icmp " + condStr + " " + getOperand(0)->type->toString() + " " + getOperand(0)->name + ", " + getOperand(1)->name;
}

std::string BranchInstr::toString() const {
    return "br " + getOperand(0)->type->toString() + " " + getOperand(0)->name + ", label %" + getOperand(1)->name + ", label %" + getOperand(2)->name;
}

std::string JumpInstr::toString() const {
    return "br label %" + getOperand(0)->name;
}

std::string CallInstr::toString() const {
    std::stringstream ss;
    if (!type->isVoid()) {
        ss << name << " = ";
    }
    ss << "call " << type->toString() << " " << getOperand(0)->name << "(";
    for (size_t i = 1; i < operandList.size(); ++i) {
        if (i > 1) ss << ", ";
        ss << getOperand(i)->type->toString() << " " << getOperand(i)->name;
    }
    ss << ")";
    return ss.str();
}

std::string ReturnInstr::toString() const {
    if (operandList.empty()) {
        return "ret void";
    } else {
        return "ret " + getOperand(0)->type->toString() + " " + getOperand(0)->name;
    }
}

GepInstr::GepInstr(IrValue* ptr, const std::vector<IrValue*>& indices, std::string n) 
    : Instr(nullptr, InstrType::GEP, n) { // Type needs calculation
    addOperand(ptr);
    for (auto idx : indices) addOperand(idx);
    
    // Calculate type: pointer to element
    // Simplified: assume array or pointer
    IrType* base = ((IrPointerType*)ptr->type)->pointedType;
    // If base is array, and we have 2 indices (0, i), result is pointer to element.
    // If base is pointer, and we have 1 index (i), result is pointer to element.
    
    // For now, let's just assume it returns a pointer to something.
    // Ideally we should implement type calculation logic.
    // Hack: if base is array, result is pointer to element type.
    if (base->isArray()) {
        type = new IrPointerType(((IrArrayType*)base)->elementType);
    } else {
        type = ptr->type; // Pointer arithmetic keeps same pointer type
    }
}

std::string GepInstr::toString() const {
    std::stringstream ss;
    ss << name << " = getelementptr " << ((IrPointerType*)getOperand(0)->type)->pointedType->toString() << ", " 
       << getOperand(0)->type->toString() << " " << getOperand(0)->name;
    
    for (size_t i = 1; i < operandList.size(); ++i) {
        ss << ", " << getOperand(i)->type->toString() << " " << getOperand(i)->name;
    }
    return ss.str();
}

std::string ZextInstr::toString() const {
    return name + " = zext " + getOperand(0)->type->toString() + " " + getOperand(0)->name + " to " + type->toString();
}

std::string TruncInstr::toString() const {
    return name + " = trunc " + getOperand(0)->type->toString() + " " + getOperand(0)->name + " to " + type->toString();
}
