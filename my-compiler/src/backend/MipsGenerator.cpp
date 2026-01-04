#include "MipsGenerator.hpp"
#include <sstream>
#include <algorithm>
#include <functional>

MipsGenerator::MipsGenerator(IrModule *module, std::ostream &out)
    : module(module), out(out) {}

std::string MipsGenerator::makeEdgeLabel(const std::string &base)
{
    return base + "_phi_edge_" + std::to_string(phiEdgeCounter++);
}

void MipsGenerator::emitPhiCopies(IrBasicBlock *from, IrBasicBlock *to)
{
    if (!to)
        return;

    // Phi nodes are expected at the start of the basic block.
    for (auto *instr : to->instructions)
    {
        if (!instr || instr->instrType != InstrType::PHI)
            break;
        auto *phi = dynamic_cast<PhiInstr *>(instr);
        if (!phi)
            break;

        IrValue *incoming = phi->getIncomingValue(from);
        if (!incoming)
        {
            // Fallback: treat as 0
            loadToRegister(IrConstantInt::get(0), T0);
        }
        else
        {
            loadToRegister(incoming, T0);
        }
        storeFromRegister(phi, T0);
    }
}

void MipsGenerator::generate()
{
    // Data Segment
    out << ".data\n";
    for (auto gv : module->globalValues)
    {
        std::string name = "_" + gv->name.substr(1); // Strip @ and add _ prefix
        out << name << ":";

        if (auto constArr = dynamic_cast<IrConstantArray *>(gv->initVal))
        {
            // Flatten array
            out << "\n";
            std::function<void(IrConstantArray *)> emitArray = [&](IrConstantArray *arr)
            {
                for (auto elem : arr->elements)
                {
                    if (auto subArr = dynamic_cast<IrConstantArray *>(elem))
                    {
                        emitArray(subArr);
                    }
                    else if (auto constInt = dynamic_cast<IrConstantInt *>(elem))
                    {
                        if (constInt->type->isInt8())
                        {
                            out << "    .byte " << constInt->value << "\n";
                        }
                        else
                        {
                            out << "    .word " << constInt->value << "\n";
                        }
                    }
                }
            };
            emitArray(constArr);
        }
        else if (auto constInt = dynamic_cast<IrConstantInt *>(gv->initVal))
        {
            out << " .word " << constInt->value << "\n";
        }
        else
        {
            // Zero init or uninit
            // Calculate size
            int size = getSize(dynamic_cast<IrPointerType *>(gv->type)->pointedType);
            out << " .space " << size << "\n";
        }
    }

    // Text Segment
    out << "\n.text\n";
    out << "jal _main\n";             // Entry point
    out << "li $v0, 10\nsyscall\n\n"; // Exit

    for (auto func : module->functions)
    {
        if (func->isBuiltin)
            continue;
        visitFunction(func);
    }
}

void MipsGenerator::visitFunction(IrFunction *func)
{
    currentFunction = func;
    currentBlock = nullptr;
    stackOffsets.clear();
    currentStackSize = 0;
    phiEdgeCounter = 0;

    // Calculate stack layout
    // 1. Saved registers ($ra, $fp)
    currentStackSize += 8;
    // $ra: -4($fp)
    // $fp: -8($fp)
    // Locals start at -12($fp)

    int localStart = 8;

    // 2. Arguments
    for (size_t i = 0; i < func->params.size(); ++i)
    {
        auto arg = func->params[i];
        if (i < 4)
        {
            localStart += 4;
            stackOffsets[arg] = -localStart;
        }
        else
        {
            // Arg 4 is at 0($fp), Arg 5 at 4($fp)
            stackOffsets[arg] = (i - 4) * 4;
        }
    }

    // 3. Instructions (Locals and Temps)
    for (auto bb : func->blocks)
    {
        for (auto instr : bb->instructions)
        {
            if (!instr->type->isVoid())
            {
                int size = 4;
                int align = 4;

                if (auto allocaInstr = dynamic_cast<AllocaInstr *>(instr))
                {
                    // Allocate space for the variable content
                    IrType *pointedType = dynamic_cast<IrPointerType *>(allocaInstr->type)->pointedType;
                    size = getSize(pointedType);

                    if (pointedType->isInt8())
                        align = 1;
                    else if (pointedType->isArray())
                    {
                        auto arr = dynamic_cast<IrArrayType *>(pointedType);
                        if (arr->elementType->isInt8())
                            align = 1;
                        else
                            align = 4;
                    }
                    else
                    {
                        align = 4;
                    }
                }
                else
                {
                    // SSA value - always 4 bytes (promoted)
                    size = 4;
                    align = 4;
                }

                localStart += size;
                if (localStart % align != 0)
                    localStart += (align - (localStart % align));
                stackOffsets[instr] = -localStart;
            }
        }
    }

    // Align stack size to 8 bytes
    if (localStart % 8 != 0)
        localStart += 4;
    currentStackSize = localStart;

    // Emit function label
    out << getFunctionName(func) << ":\n";

    // Prologue
    emit("sw $ra, -4($sp)");
    emit("sw $fp, -8($sp)");
    emit("move $fp, $sp");
    if (currentStackSize > 32767)
    {
        emit("li $t0, " + std::to_string(currentStackSize));
        emit("subu $sp, $sp, $t0");
    }
    else
    {
        emit("addiu $sp, $sp, -" + std::to_string(currentStackSize));
    }

    // Save arguments to stack
    for (size_t i = 0; i < func->params.size() && i < 4; ++i)
    {
        std::string reg = "$a" + std::to_string(i);
        storeFromRegister(func->params[i], reg);
    }

    // Visit Blocks
    for (auto bb : func->blocks)
    {
        visitBasicBlock(bb);
    }
}

void MipsGenerator::visitBasicBlock(IrBasicBlock *bb)
{
    currentBlock = bb;
    out << getLabelName(bb) << ":\n";
    for (auto instr : bb->instructions)
    {
        visitInstr(instr);
    }
}

void MipsGenerator::visitInstr(Instr *instr)
{
    switch (instr->instrType)
    {
    case InstrType::ADD:
    case InstrType::SUB:
    case InstrType::MUL:
    case InstrType::SDIV:
    case InstrType::SREM:
    {
        loadToRegister(instr->getOperand(0), T0);
        loadToRegister(instr->getOperand(1), T1);
        std::string op;
        switch (instr->instrType)
        {
        case InstrType::ADD:
            op = "addu";
            break;
        case InstrType::SUB:
            op = "subu";
            break;
        case InstrType::MUL:
            op = "mul";
            break;
        case InstrType::SDIV:
            op = "div";
            break;
        case InstrType::SREM:
            op = "div";
            break;
        default:
            break;
        }

        if (instr->instrType == InstrType::SDIV)
        {
            emit("div " + T0 + ", " + T1);
            emit("mflo " + T2);
        }
        else if (instr->instrType == InstrType::SREM)
        {
            emit("div " + T0 + ", " + T1);
            emit("mfhi " + T2);
        }
        else
        {
            emit(op + " " + T2 + ", " + T0 + ", " + T1);
        }
        storeFromRegister(instr, T2);
        break;
    }
    case InstrType::ALLOCA:
    {
        break;
    }
    case InstrType::PHI:
    {
        // Phi is lowered on CFG edges (in BR/JUMP). Nothing to emit here.
        break;
    }
    case InstrType::LOAD:
    {
        loadToRegister(instr->getOperand(0), T0); // Load address
        if (instr->type->isInt8())
        {
            emit("lb " + T1 + ", 0(" + T0 + ")");
        }
        else
        {
            emit("lw " + T1 + ", 0(" + T0 + ")");
        }
        storeFromRegister(instr, T1);
        break;
    }
    case InstrType::STORE:
    {
        loadToRegister(instr->getOperand(0), T0); // Value
        loadToRegister(instr->getOperand(1), T1); // Pointer
        if (instr->getOperand(0)->type->isInt8())
        {
            emit("sb " + T0 + ", 0(" + T1 + ")");
        }
        else
        {
            emit("sw " + T0 + ", 0(" + T1 + ")");
        }
        break;
    }
    case InstrType::ICMP:
    {
        auto icmp = dynamic_cast<IcmpInstr *>(instr);
        loadToRegister(icmp->getOperand(0), T0);
        loadToRegister(icmp->getOperand(1), T1);

        switch (icmp->cond)
        {
        case IcmpCond::EQ:
            emit("xor " + T2 + ", " + T0 + ", " + T1);
            emit("sltiu " + T2 + ", " + T2 + ", 1");
            break;
        case IcmpCond::NE:
            emit("xor " + T2 + ", " + T0 + ", " + T1);
            emit("sltu " + T2 + ", $zero, " + T2);
            break;
        case IcmpCond::SGT:
            emit("slt " + T2 + ", " + T1 + ", " + T0);
            break;
        case IcmpCond::SGE:
            emit("slt " + T2 + ", " + T0 + ", " + T1);
            emit("xori " + T2 + ", " + T2 + ", 1");
            break;
        case IcmpCond::SLT:
            emit("slt " + T2 + ", " + T0 + ", " + T1);
            break;
        case IcmpCond::SLE:
            emit("slt " + T2 + ", " + T1 + ", " + T0);
            emit("xori " + T2 + ", " + T2 + ", 1");
            break;
        }
        storeFromRegister(instr, T2);
        break;
    }
    case InstrType::BR:
    {
        loadToRegister(instr->getOperand(0), T0);
        auto *trueBlock = dynamic_cast<IrBasicBlock *>(instr->getOperand(1));
        auto *falseBlock = dynamic_cast<IrBasicBlock *>(instr->getOperand(2));

        std::string edgeTrue = makeEdgeLabel(getLabelName(currentBlock) + "_to_" + getLabelName(trueBlock));
        std::string edgeFalse = makeEdgeLabel(getLabelName(currentBlock) + "_to_" + getLabelName(falseBlock));

        emit("bne " + T0 + ", $zero, " + edgeTrue);
        emit("j " + edgeFalse);

        out << edgeTrue << ":\n";
        emitPhiCopies(currentBlock, trueBlock);
        emit("j " + getLabelName(trueBlock));

        out << edgeFalse << ":\n";
        emitPhiCopies(currentBlock, falseBlock);
        emit("j " + getLabelName(falseBlock));
        break;
    }
    case InstrType::JUMP:
    {
        auto *target = dynamic_cast<IrBasicBlock *>(instr->getOperand(0));
        emitPhiCopies(currentBlock, target);
        emit("j " + getLabelName(target));
        break;
    }
    case InstrType::CALL:
    {
        // Operand 0 is function
        // Operand 1... are args
        int argCount = instr->operandList.size() - 1;

        int stackArgs = 0;
        if (argCount > 4)
            stackArgs = argCount - 4;
        if (stackArgs > 0)
        {
            emit("addiu $sp, $sp, -" + std::to_string(stackArgs * 4));
        }

        for (int i = 0; i < argCount; ++i)
        {
            loadToRegister(instr->getOperand(i + 1), T0);
            if (i < 4)
            {
                emit("move $a" + std::to_string(i) + ", " + T0);
            }
            else
            {
                emit("sw " + T0 + ", " + std::to_string((i - 4) * 4) + "($sp)");
            }
        }

        auto func = dynamic_cast<IrFunction *>(instr->getOperand(0));
        std::string funcName = getFunctionName(func);

        if (func->name == "@getint")
        {
            emit("li $v0, 5");
            emit("syscall");
        }
        else if (func->name == "@putint")
        {
            emit("li $v0, 1");
            emit("syscall");
        }
        else if (func->name == "@putch")
        {
            emit("li $v0, 11");
            emit("syscall");
        }
        else
        {
            emit("jal " + funcName);
        }

        if (stackArgs > 0)
        {
            emit("addiu $sp, $sp, " + std::to_string(stackArgs * 4));
        }

        if (!instr->type->isVoid())
        {
            storeFromRegister(instr, V0);
        }
        break;
    }
    case InstrType::RET:
    {
        if (instr->operandList.size() > 0)
        {
            loadToRegister(instr->getOperand(0), V0);
        }
        // Epilogue
        emit("move $sp, $fp");
        emit("lw $ra, -4($sp)");
        emit("lw $fp, -8($sp)");
        emit("jr $ra");
        break;
    }
    case InstrType::GEP:
    {
        loadToRegister(instr->getOperand(0), T0); // Base pointer

        IrType *curType = instr->getOperand(0)->type;
        if (curType->isPointer())
        {
            curType = dynamic_cast<IrPointerType *>(curType)->pointedType;
        }

        for (size_t i = 1; i < instr->operandList.size(); ++i)
        {
            IrValue *index = instr->getOperand(i);
            int elementSize = getSize(curType);

            loadToRegister(index, T1);
            emit("li " + T2 + ", " + std::to_string(elementSize));
            emit("mul " + T1 + ", " + T1 + ", " + T2);
            emit("addu " + T0 + ", " + T0 + ", " + T1);

            if (curType->isArray())
            {
                curType = dynamic_cast<IrArrayType *>(curType)->elementType;
            }
        }
        storeFromRegister(instr, T0);
        break;
    }
    case InstrType::ZEXT:
    {
        loadToRegister(instr->getOperand(0), T0);
        storeFromRegister(instr, T0);
        break;
    }
    case InstrType::TRUNC:
    {
        loadToRegister(instr->getOperand(0), T0);
        if (instr->type->isInt1())
        {
            emit("andi " + T0 + ", " + T0 + ", 1");
        }
        storeFromRegister(instr, T0);
        break;
    }
    default:
        out << "# Unknown instr\n";
        break;
    }
}

void MipsGenerator::emit(std::string instr)
{
    out << "    " << instr << "\n";
}

void MipsGenerator::loadToRegister(IrValue *val, std::string reg)
{
    if (auto constInt = dynamic_cast<IrConstantInt *>(val))
    {
        emit("li " + reg + ", " + std::to_string(constInt->value));
    }
    else if (auto gv = dynamic_cast<IrGlobalValue *>(val))
    {
        std::string name = "_" + gv->name.substr(1);
        emit("la " + reg + ", " + name);
    }
    else if (auto allocaInstr = dynamic_cast<AllocaInstr *>(val))
    {
        int offset = stackOffsets[allocaInstr];
        emit("addiu " + reg + ", $fp, " + std::to_string(offset));
    }
    else
    {
        if (stackOffsets.find(val) != stackOffsets.end())
        {
            int offset = stackOffsets[val];
            emit("lw " + reg + ", " + std::to_string(offset) + "($fp)");
        }
        else
        {
            emit("# Error: Value not found in stack map: " + val->name);
        }
    }
}

void MipsGenerator::storeFromRegister(IrValue *val, std::string reg)
{
    if (stackOffsets.find(val) != stackOffsets.end())
    {
        int offset = stackOffsets[val];
        emit("sw " + reg + ", " + std::to_string(offset) + "($fp)");
    }
}

int MipsGenerator::getSize(IrType *type)
{
    if (type->isInt32())
        return 4;
    if (type->isInt1())
        return 4;
    if (type->isInt8())
        return 1;
    if (type->isPointer())
        return 4;
    if (type->isArray())
    {
        auto arr = dynamic_cast<IrArrayType *>(type);
        return arr->numElements * getSize(arr->elementType);
    }
    return 4;
}

std::string MipsGenerator::getLabelName(IrBasicBlock *bb)
{
    std::string funcName = currentFunction->name.substr(1);
    return "L_" + funcName + "_" + bb->name.substr(1); // Strip %
}

std::string MipsGenerator::getFunctionName(IrFunction *func)
{
    if (func->isBuiltin)
        return func->name.substr(1);
    return "_" + func->name.substr(1);
}
