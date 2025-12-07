#pragma once
#include "IrModule.hpp"
#include "value/IrBasicBlock.hpp"
#include "value/IrFunction.hpp"
#include "instr/Instr.hpp"
#include "instr/AllocaInstr.hpp"
#include "instr/LoadInstr.hpp"
#include "instr/StoreInstr.hpp"
#include "instr/AluInstr.hpp"
#include "instr/IcmpInstr.hpp"
#include "instr/BranchInstr.hpp"
#include "instr/JumpInstr.hpp"
#include "instr/CallInstr.hpp"
#include "instr/ReturnInstr.hpp"
#include "instr/GepInstr.hpp"
#include "instr/ZextInstr.hpp"
#include "instr/TruncInstr.hpp"

class IrBuilder {
public:
    static IrModule* module;
    static IrFunction* currentFunction;
    static IrBasicBlock* currentBlock;

    static void setModule(IrModule* m) { module = m; }
    static void setFunction(IrFunction* f) { currentFunction = f; }
    static void setBasicBlock(IrBasicBlock* bb) { currentBlock = bb; }

    static IrBasicBlock* createBasicBlock(std::string name = "") {
        auto* bb = new IrBasicBlock(name, currentFunction);
        currentFunction->addBasicBlock(bb);
        return bb;
    }

    static void insertInstr(Instr* instr) {
        if (currentBlock) {
            currentBlock->addInstr(instr);
            instr->parentBlock = currentBlock;
        }
    }

    static AllocaInstr* createAlloca(IrType* type, std::string name = "") {
        auto* instr = new AllocaInstr(type, name);
        // Alloca should be in the entry block usually, but for now just insert at current
        // Ideally we should move it to entry block
        insertInstr(instr);
        return instr;
    }
    
    // ... other create methods
};
