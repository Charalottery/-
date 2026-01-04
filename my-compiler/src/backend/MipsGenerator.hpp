#pragma once
#include "../midend/llvm/IrModule.hpp"
#include "../midend/llvm/value/IrFunction.hpp"
#include "../midend/llvm/value/IrBasicBlock.hpp"
#include "../midend/llvm/instr/Instr.hpp"
#include "../midend/llvm/instr/AluInstr.hpp"
#include "../midend/llvm/instr/LoadInstr.hpp"
#include "../midend/llvm/instr/StoreInstr.hpp"
#include "../midend/llvm/instr/AllocaInstr.hpp"
#include "../midend/llvm/instr/BranchInstr.hpp"
#include "../midend/llvm/instr/JumpInstr.hpp"
#include "../midend/llvm/instr/CallInstr.hpp"
#include "../midend/llvm/instr/ReturnInstr.hpp"
#include "../midend/llvm/instr/GepInstr.hpp"
#include "../midend/llvm/instr/IcmpInstr.hpp"
#include "../midend/llvm/instr/ZextInstr.hpp"
#include "../midend/llvm/instr/TruncInstr.hpp"
#include "../midend/llvm/instr/PhiInstr.hpp"
#include "../midend/llvm/value/IrConstantInt.hpp"
#include "../midend/llvm/value/IrConstantArray.hpp"
#include "../midend/llvm/type/IrBaseType.hpp"
#include "../midend/llvm/type/IrPointerType.hpp"
#include "../midend/llvm/type/IrArrayType.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>

class MipsGenerator
{
public:
    MipsGenerator(IrModule *module, std::ostream &out);
    void generate();

private:
    IrModule *module;
    std::ostream &out;

    // Current function context
    IrFunction *currentFunction;
    IrBasicBlock *currentBlock;
    std::map<IrValue *, int> stackOffsets; // Offset from FP
    int currentStackSize;
    int phiEdgeCounter = 0;

    void visitFunction(IrFunction *func);
    void visitBasicBlock(IrBasicBlock *bb);
    void visitInstr(Instr *instr);

    void emitPhiCopies(IrBasicBlock *from, IrBasicBlock *to);
    std::string makeEdgeLabel(const std::string &base);

    // Helpers
    void emit(std::string instr);
    void emitLabel(std::string label);
    void loadToRegister(IrValue *val, std::string reg);
    void storeFromRegister(IrValue *val, std::string reg);
    int getStackOffset(IrValue *val);
    int getSize(IrType *type);
    std::string getLabelName(IrBasicBlock *bb);
    std::string getFunctionName(IrFunction *func);

    // Register names
    const std::string ZERO = "$zero";
    const std::string SP = "$sp";
    const std::string FP = "$fp";
    const std::string RA = "$ra";
    const std::string V0 = "$v0";
    const std::string A0 = "$a0";
    const std::string A1 = "$a1";
    const std::string A2 = "$a2";
    const std::string A3 = "$a3";
    const std::string T0 = "$t0";
    const std::string T1 = "$t1";
    const std::string T2 = "$t2";
    const std::string T3 = "$t3";
    const std::string T4 = "$t4";
};
