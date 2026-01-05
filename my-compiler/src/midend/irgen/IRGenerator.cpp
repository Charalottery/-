#include "IRGenerator.hpp"
#include "../llvm/type/IrBaseType.hpp"
#include "../llvm/type/IrFunctionType.hpp"
#include "../llvm/type/IrPointerType.hpp"
#include "../llvm/type/IrArrayType.hpp"
#include "../llvm/value/IrConstantInt.hpp"
#include "../llvm/value/IrConstantArray.hpp"
#include "../llvm/value/IrGlobalValue.hpp"
#include "../llvm/instr/AllocaInstr.hpp"
#include "../llvm/instr/StoreInstr.hpp"
#include "../llvm/instr/LoadInstr.hpp"
#include "../llvm/instr/AluInstr.hpp"
#include "../llvm/instr/IcmpInstr.hpp"
#include "../llvm/instr/BranchInstr.hpp"
#include "../llvm/instr/JumpInstr.hpp"
#include "../llvm/instr/CallInstr.hpp"
#include "../llvm/instr/ReturnInstr.hpp"
#include "../llvm/instr/GepInstr.hpp"
#include "../llvm/instr/ZextInstr.hpp"
#include "../llvm/instr/TruncInstr.hpp"
#include <iostream>
#include <functional>

IRGenerator::IRGenerator(ASTNode *root, SymbolTable *rootTable)
    : currentSymbolTable(rootTable), root(root)
{
    module = new IrModule();
    IrBuilder::setModule(module);
    initLibraryFunctions();
}

void IRGenerator::initLibraryFunctions()
{
    // getint() -> int
    {
        std::vector<IrType *> params;
        IrFunction *func = new IrFunction(IrBaseType::getInt32(), params, "@getint", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("getint");
        if (!sym)
        {
            Symbol s("getint", "IntFunc", true, std::vector<std::string>(), 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("getint");
        }
        if (sym)
            sym->llvmValue = func;
    }
    // getch() -> int
    {
        std::vector<IrType *> params;
        IrFunction *func = new IrFunction(IrBaseType::getInt32(), params, "@getch", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("getch");
        if (!sym)
        {
            Symbol s("getch", "IntFunc", true, std::vector<std::string>(), 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("getch");
        }
        if (sym)
            sym->llvmValue = func;
    }
    // getarray(int[]) -> int
    {
        std::vector<IrType *> params = {new IrPointerType(IrBaseType::getInt32())};
        IrFunction *func = new IrFunction(IrBaseType::getInt32(), params, "@getarray", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("getarray");
        if (!sym)
        {
            Symbol s("getarray", "IntFunc", true, std::vector<std::string>{"IntArray"}, 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("getarray");
        }
        if (sym)
            sym->llvmValue = func;
    }
    // putint(int) -> void
    {
        std::vector<IrType *> params = {IrBaseType::getInt32()};
        IrFunction *func = new IrFunction(IrBaseType::getVoid(), params, "@putint", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("putint");
        if (!sym)
        {
            Symbol s("putint", "VoidFunc", true, std::vector<std::string>{"Int"}, 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("putint");
        }
        if (sym)
            sym->llvmValue = func;
    }
    // putch(int) -> void
    {
        std::vector<IrType *> params = {IrBaseType::getInt32()};
        IrFunction *func = new IrFunction(IrBaseType::getVoid(), params, "@putch", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("putch");
        if (!sym)
        {
            Symbol s("putch", "VoidFunc", true, std::vector<std::string>{"Int"}, 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("putch");
        }
        if (sym)
            sym->llvmValue = func;
    }
    // putarray(int, int[]) -> void
    {
        std::vector<IrType *> params = {IrBaseType::getInt32(), new IrPointerType(IrBaseType::getInt32())};
        IrFunction *func = new IrFunction(IrBaseType::getVoid(), params, "@putarray", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("putarray");
        if (!sym)
        {
            Symbol s("putarray", "VoidFunc", true, std::vector<std::string>{"Int", "IntArray"}, 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("putarray");
        }
        if (sym)
            sym->llvmValue = func;
    }
    // putstr(char*) -> void
    {
        std::vector<IrType *> params = {new IrPointerType(IrBaseType::getInt8())};
        IrFunction *func = new IrFunction(IrBaseType::getVoid(), params, "@putstr", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("putstr");
        if (!sym)
        {
            Symbol s("putstr", "VoidFunc", true, std::vector<std::string>{"IntArray"}, 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("putstr");
        }
        if (sym)
            sym->llvmValue = func;
    }
    // starttime() -> void
    {
        std::vector<IrType *> params;
        IrFunction *func = new IrFunction(IrBaseType::getVoid(), params, "@starttime", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("starttime");
        if (!sym)
        {
            Symbol s("starttime", "VoidFunc", true, std::vector<std::string>(), 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("starttime");
        }
        if (sym)
            sym->llvmValue = func;
    }
    // stoptime() -> void
    {
        std::vector<IrType *> params;
        IrFunction *func = new IrFunction(IrBaseType::getVoid(), params, "@stoptime", true);
        module->addFunction(func);
        Symbol *sym = currentSymbolTable->GetLocalSymbol("stoptime");
        if (!sym)
        {
            Symbol s("stoptime", "VoidFunc", true, std::vector<std::string>(), 0);
            s.isBuiltin = true;
            currentSymbolTable->AddSymbol(s);
            sym = currentSymbolTable->GetLocalSymbol("stoptime");
        }
        if (sym)
            sym->llvmValue = func;
    }
}

void IRGenerator::generate()
{
    visitCompUnit(root);
}

std::string IRGenerator::getNewName(std::string prefix)
{
    return prefix + "_" + std::to_string(tmpCounter++);
}

Symbol *IRGenerator::findSymbol(const std::string &name)
{
    SymbolTable *table = currentSymbolTable;
    while (table)
    {
        if (table->HasSymbol(name))
        {
            Symbol *sym = table->GetLocalSymbol(name);
            // If the symbol is found but has no LLVM value and is not a constant/function/builtin,
            // it might be a variable declared later in the same scope (forward reference in AST but not valid in C).
            // In C, "int a = b, b = 10;" means 'a' uses outer 'b'.
            // So if we find a local symbol without llvmValue, we should continue searching in outer scopes
            // UNLESS it's a global variable (which might be initialized later but exists) or function.
            // However, globals usually have llvmValue created early or are handled differently.
            // Let's assume if llvmValue is null and it's a local variable, we skip it.

            if (sym->llvmValue != nullptr || sym->isConst || sym->isFunction || sym->isBuiltin)
            {
                return sym;
            }
            // If we are in the scope where this symbol is defined, but it hasn't been visited yet (llvmValue is null),
            // we should look in the parent scope.
        }
        table = table->parent;
    }
    return nullptr;
}

void IRGenerator::enterScope()
{
    if (currentSymbolTable->nextChildToVisit < (int)currentSymbolTable->children.size())
    {
        currentSymbolTable = currentSymbolTable->children[currentSymbolTable->nextChildToVisit++].get();
    }
}

void IRGenerator::exitScope()
{
    if (currentSymbolTable->parent)
    {
        currentSymbolTable = currentSymbolTable->parent;
    }
}

void IRGenerator::visitCompUnit(ASTNode *node)
{
    for (const auto &child : node->children)
    {
        if (child->name == "Decl")
        {
            visitDecl(child.get());
        }
        else if (child->name == "FuncDef")
        {
            visitFuncDef(child.get());
        }
        else if (child->name == "MainFuncDef")
        {
            visitMainFuncDef(child.get());
        }
    }
}

void IRGenerator::visitDecl(ASTNode *node)
{
    if (node->children[0]->name == "ConstDecl")
    {
        visitConstDecl(node->children[0].get());
    }
    else
    {
        visitVarDecl(node->children[0].get());
    }
}

void IRGenerator::visitConstDecl(ASTNode *node)
{
    for (const auto &child : node->children)
    {
        if (child->name != "ConstDef")
            continue;
        ASTNode *constDef = child.get();
        std::string name = constDef->children[0]->token.value;
        Symbol *sym = currentSymbolTable->GetLocalSymbol(name);

        if (!sym)
            continue;

        std::vector<int> dims;
        for (size_t i = 0; i < constDef->children.size(); ++i)
        {
            if (constDef->children[i]->isToken && constDef->children[i]->token.type == TokenType::LBRACK)
            {
                if (i + 1 < constDef->children.size() && constDef->children[i + 1]->name == "ConstExp")
                {
                    dims.push_back(evaluateConstExp(constDef->children[i + 1].get()));
                }
            }
        }

        IrType *type = IrBaseType::getInt32();
        if (!dims.empty())
        {
            for (int i = dims.size() - 1; i >= 0; --i)
            {
                type = new IrArrayType(type, dims[i]);
            }
        }

        bool isGlobal = (currentSymbolTable->parent == nullptr);
        ASTNode *initVal = constDef->children.back().get();

        if (dims.empty())
        {
            int val = 0;
            if (initVal->children[0]->name == "ConstExp")
            {
                val = evaluateConstExp(initVal->children[0].get());
            }
            sym->constVal = val;

            if (isGlobal)
            {
                IrConstant *init = IrConstantInt::get(val);
                IrGlobalValue *gv = new IrGlobalValue(type, "@" + name, init, true);
                module->addGlobalValue(gv);
                sym->llvmValue = gv;
            }
            else
            {
                AllocaInstr *alloca = IrBuilder::createAlloca(type, getNewName(name + "_addr"));
                sym->llvmValue = alloca;
                IrBuilder::insertInstr(new StoreInstr(IrConstantInt::get(val), alloca));
            }
        }
        else
        {
            // Array constant
            int totalSize = 1;
            std::vector<int> strides;
            for (size_t k = 0; k < dims.size(); ++k)
            {
                int s = 1;
                for (size_t j = k + 1; j < dims.size(); ++j)
                    s *= dims[j];
                strides.push_back(s);
                if (k == 0)
                    totalSize = dims[0] * s;
            }

            if (isGlobal)
            {
                std::vector<IrConstant *> flatVals;
                getGlobalInitVals(initVal, flatVals);
                while (flatVals.size() < (size_t)totalSize)
                    flatVals.push_back(IrConstantInt::get(0));

                // Store values in symbol for constant folding
                for (auto *val : flatVals)
                {
                    if (val->type->isInt32())
                    {
                        sym->arrayValues.push_back(((IrConstantInt *)val)->value);
                    }
                    else
                    {
                        sym->arrayValues.push_back(0);
                    }
                }

                // Reconstruct
                int offset = 0;
                std::function<IrConstant *(IrType *)> reconstruct = [&](IrType *t) -> IrConstant *
                {
                    if (t->isInt32())
                        return flatVals[offset++];
                    if (t->isArray())
                    {
                        IrArrayType *at = (IrArrayType *)t;
                        std::vector<IrConstant *> elms;
                        for (int i = 0; i < at->numElements; ++i)
                            elms.push_back(reconstruct(at->elementType));
                        return new IrConstantArray(t, elms);
                    }
                    return nullptr;
                };

                IrConstant *init = reconstruct(type);
                IrGlobalValue *gv = new IrGlobalValue(type, "@" + name, init, true);
                module->addGlobalValue(gv);
                sym->llvmValue = gv;
            }
            else
            {
                // For local const arrays, we also need to store values for constant folding
                std::vector<IrConstant *> flatVals;
                getGlobalInitVals(initVal, flatVals);
                while (flatVals.size() < (size_t)totalSize)
                    flatVals.push_back(IrConstantInt::get(0));

                for (auto *val : flatVals)
                {
                    if (val->type->isInt32())
                    {
                        sym->arrayValues.push_back(((IrConstantInt *)val)->value);
                    }
                    else
                    {
                        sym->arrayValues.push_back(0);
                    }
                }

                AllocaInstr *alloca = IrBuilder::createAlloca(type, getNewName(name + "_addr"));
                sym->llvmValue = alloca;

                std::vector<ASTNode *> flatExprs;
                getLocalInitVals(initVal, flatExprs);

                for (int i = 0; i < totalSize; ++i)
                {
                    IrValue *val = nullptr;
                    if (i < (int)flatExprs.size())
                    {
                        int v = evaluateConstExp(flatExprs[i]);
                        val = IrConstantInt::get(v);
                    }
                    else
                    {
                        val = IrConstantInt::get(0);
                    }

                    std::vector<IrValue *> indices;
                    indices.push_back(IrConstantInt::get(0)); // Dereference pointer
                    int temp = i;
                    for (int s : strides)
                    {
                        indices.push_back(IrConstantInt::get(temp / s));
                        temp %= s;
                    }

                    auto *gep = new GepInstr(alloca, indices, getNewName("gep"));
                    IrBuilder::insertInstr(gep);
                    IrBuilder::insertInstr(new StoreInstr(val, gep));
                }
            }
        }
    }
}

void IRGenerator::visitVarDecl(ASTNode *node)
{
    bool isStatic = false;
    for (const auto &child : node->children)
    {
        if (child->isToken && child->token.value == "static")
        {
            isStatic = true;
            break;
        }
    }

    for (const auto &child : node->children)
    {
        if (child->name != "VarDef")
            continue;
        ASTNode *varDef = child.get();
        std::string name = varDef->children[0]->token.value;
        Symbol *sym = currentSymbolTable->GetLocalSymbol(name);

        if (!sym)
        {
            continue;
        }

        std::vector<int> dims;
        for (size_t i = 0; i < varDef->children.size(); ++i)
        {
            if (varDef->children[i]->isToken && varDef->children[i]->token.type == TokenType::LBRACK)
            {
                if (i + 1 < varDef->children.size() && varDef->children[i + 1]->name == "ConstExp")
                {
                    dims.push_back(evaluateConstExp(varDef->children[i + 1].get()));
                }
            }
        }

        IrType *type = IrBaseType::getInt32();
        if (!dims.empty())
        {
            for (int i = dims.size() - 1; i >= 0; --i)
            {
                type = new IrArrayType(type, dims[i]);
            }
        }

        bool isGlobal = (currentSymbolTable->parent == nullptr);

        if (isGlobal || isStatic)
        {
            IrConstant *init = nullptr;

            if (varDef->children.back()->name == "InitVal")
            {
                ASTNode *initVal = varDef->children.back().get();
                if (dims.empty())
                {
                    if (initVal->children[0]->name == "Exp")
                    {
                        int val = evaluateConstExp(initVal->children[0].get());
                        init = IrConstantInt::get(val);
                        sym->constVal = val;
                    }
                }
                else
                {
                    int totalSize = 1;
                    for (size_t k = 0; k < dims.size(); ++k)
                    {
                        int s = 1;
                        for (size_t j = k + 1; j < dims.size(); ++j)
                            s *= dims[j];
                        if (k == 0)
                            totalSize = dims[0] * s;
                    }

                    std::vector<IrConstant *> flatVals;
                    getGlobalInitVals(initVal, flatVals);
                    while (flatVals.size() < (size_t)totalSize)
                        flatVals.push_back(IrConstantInt::get(0));

                    for (auto *val : flatVals)
                    {
                        if (val->type->isInt32())
                        {
                            sym->arrayValues.push_back(((IrConstantInt *)val)->value);
                        }
                        else
                        {
                            sym->arrayValues.push_back(0);
                        }
                    }

                    int offset = 0;
                    std::function<IrConstant *(IrType *)> reconstruct = [&](IrType *t) -> IrConstant *
                    {
                        if (t->isInt32())
                            return flatVals[offset++];
                        if (t->isArray())
                        {
                            IrArrayType *at = (IrArrayType *)t;
                            std::vector<IrConstant *> elms;
                            for (int i = 0; i < at->numElements; ++i)
                                elms.push_back(reconstruct(at->elementType));
                            return new IrConstantArray(t, elms);
                        }
                        return nullptr;
                    };
                    init = reconstruct(type);
                }
            }

            if (!init)
            {
                if (dims.empty())
                    init = IrConstantInt::get(0);
                else
                {
                    // Zero init recursively
                    std::function<IrConstant *(IrType *)> makeZero = [&](IrType *t) -> IrConstant *
                    {
                        if (t->isInt32())
                            return IrConstantInt::get(0);
                        if (t->isArray())
                        {
                            IrArrayType *at = (IrArrayType *)t;
                            std::vector<IrConstant *> elms;
                            for (int i = 0; i < at->numElements; ++i)
                                elms.push_back(makeZero(at->elementType));
                            return new IrConstantArray(t, elms);
                        }
                        return nullptr;
                    };
                    init = makeZero(type);
                }
            }

            std::string globalName = isGlobal ? "@" + name : getNewName("@" + currentFunctionName + "." + name);
            IrGlobalValue *gv = new IrGlobalValue(type, globalName, init, false);
            module->addGlobalValue(gv);
            sym->llvmValue = gv;
        }
        else
        {
            AllocaInstr *alloca = IrBuilder::createAlloca(type, getNewName(name + "_addr"));
            sym->llvmValue = alloca;

            if (varDef->children.back()->name == "InitVal")
            {
                ASTNode *initVal = varDef->children.back().get();
                if (dims.empty())
                {
                    if (initVal->children[0]->name == "Exp")
                    {
                        IrValue *val = visitExp(initVal->children[0].get());
                        if (val)
                        {
                            IrBuilder::insertInstr(new StoreInstr(val, alloca));
                        }
                        else
                        {
                            std::cerr << "Error: visitExp returned null for VarDecl " << name << std::endl;
                        }
                    }
                }
                else
                {
                    int totalSize = 1;
                    std::vector<int> strides;
                    for (size_t k = 0; k < dims.size(); ++k)
                    {
                        int s = 1;
                        for (size_t j = k + 1; j < dims.size(); ++j)
                            s *= dims[j];
                        strides.push_back(s);
                        if (k == 0)
                            totalSize = dims[0] * s;
                    }

                    std::vector<ASTNode *> flatExprs;
                    getLocalInitVals(initVal, flatExprs);

                    for (int i = 0; i < totalSize; ++i)
                    {
                        IrValue *val = nullptr;
                        if (i < (int)flatExprs.size())
                        {
                            val = visitExp(flatExprs[i]);
                        }
                        else
                        {
                            val = IrConstantInt::get(0);
                        }

                        std::vector<IrValue *> indices;
                        indices.push_back(IrConstantInt::get(0));
                        int temp = i;
                        for (int s : strides)
                        {
                            indices.push_back(IrConstantInt::get(temp / s));
                            temp %= s;
                        }

                        auto *gep = new GepInstr(alloca, indices, getNewName("gep"));
                        IrBuilder::insertInstr(gep);
                        IrBuilder::insertInstr(new StoreInstr(val, gep));
                    }
                }
            }
        }
    }
}

void IRGenerator::visitFuncDef(ASTNode *node)
{
    std::string funcName;
    for (const auto &child : node->children)
    {
        if (child->isToken && child->token.type == TokenType::IDENFR)
        {
            funcName = child->token.value;
            break;
        }
    }

    currentFunctionName = funcName; // Set current function name for static variables

    Symbol *sym = currentSymbolTable->GetLocalSymbol(funcName);
    if (!sym)
    {
        return;
    }

    tmpCounter = 0; // Reset temp counter for new function

    std::vector<IrType *> paramTypes;
    for (const auto &pType : sym->paramTypes)
    {
        if (pType == "Int")
            paramTypes.push_back(IrBaseType::getInt32());
        else if (pType == "IntArray")
            paramTypes.push_back(new IrPointerType(IrBaseType::getInt32()));
        else
            paramTypes.push_back(IrBaseType::getInt32());
    }

    IrType *retType = (sym->typeName == "VoidFunc") ? (IrType *)IrBaseType::getVoid() : (IrType *)IrBaseType::getInt32();

    IrFunction *func = new IrFunction(retType, paramTypes, "@" + funcName);
    module->addFunction(func);
    sym->llvmValue = func;

    IrBuilder::setFunction(func);
    IrBasicBlock *entry = IrBuilder::createBasicBlock("entry");
    IrBuilder::setBasicBlock(entry);

    enterScope();

    int paramIdx = 0;
    for (const auto &child : node->children)
    {
        if (child->name == "FuncFParams")
        {
            for (const auto &param : child->children)
            {
                if (param->name != "FuncFParam")
                    continue; // Skip commas

                std::string paramName;
                for (const auto &pChild : param->children)
                {
                    if (pChild->isToken && pChild->token.type == TokenType::IDENFR)
                    {
                        paramName = pChild->token.value;
                        break;
                    }
                }

                Symbol *paramSym = currentSymbolTable->GetLocalSymbol(paramName);
                if (paramSym)
                {
                    if (paramIdx < (int)func->params.size())
                    {
                        IrValue *argVal = func->params[paramIdx];
                        AllocaInstr *alloca = IrBuilder::createAlloca(argVal->type, getNewName(paramName + "_addr"));
                        IrBuilder::insertInstr(new StoreInstr(argVal, alloca));
                        paramSym->llvmValue = alloca;
                    }
                }
                paramIdx++;
            }
        }
    }

    for (const auto &child : node->children)
    {
        if (child->name == "Block")
        {
            visitBlock(child.get(), false);
            break;
        }
    }

    if (entry->instructions.empty() || entry->instructions.back()->instrType != InstrType::RET)
    {
        if (IrBuilder::currentBlock->instructions.empty() || IrBuilder::currentBlock->instructions.back()->instrType != InstrType::RET)
        {
            if (retType->isVoid())
                IrBuilder::insertInstr(new ReturnInstr(nullptr));
            else
                IrBuilder::insertInstr(new ReturnInstr(IrConstantInt::get(0)));
        }
    }

    exitScope();
}

void IRGenerator::visitMainFuncDef(ASTNode *node)
{
    currentFunctionName = "main"; // Set current function name for static variables
    tmpCounter = 0;               // Reset temp counter for main
    std::vector<IrType *> paramTypes;
    IrFunction *func = new IrFunction(IrBaseType::getInt32(), paramTypes, "@main");
    module->addFunction(func);
    IrBuilder::setFunction(func);

    Symbol *sym = currentSymbolTable->GetLocalSymbol("main");
    if (sym)
        sym->llvmValue = func;

    IrBasicBlock *entry = IrBuilder::createBasicBlock("entry");
    IrBuilder::setBasicBlock(entry);

    enterScope();

    for (const auto &child : node->children)
    {
        if (child->name == "Block")
        {
            visitBlock(child.get(), false);
            break;
        }
    }

    if (entry->instructions.empty() || entry->instructions.back()->instrType != InstrType::RET)
    {
        if (IrBuilder::currentBlock->instructions.empty() || IrBuilder::currentBlock->instructions.back()->instrType != InstrType::RET)
        {
            IrBuilder::insertInstr(new ReturnInstr(IrConstantInt::get(0)));
        }
    }

    exitScope();
}

void IRGenerator::visitBlock(ASTNode *node, bool createScope)
{
    if (createScope)
        enterScope();

    for (const auto &child : node->children)
    {
        if (child->name == "BlockItem")
        {
            if (child->children[0]->name == "Decl")
            {
                visitDecl(child->children[0].get());
            }
            else
            {
                visitStmt(child->children[0].get());
            }
        }
    }

    if (createScope)
        exitScope();
}

void IRGenerator::visitStmt(ASTNode *node)
{
    if (node->children[0]->name == "LVal")
    {
        IrValue *lhs = visitLVal(node->children[0].get(), true);
        IrValue *rhs = visitExp(node->children[2].get());
        if (lhs && rhs)
        {
            IrBuilder::insertInstr(new StoreInstr(rhs, lhs));
        }
        else
        {
            std::cerr << "Error: visitStmt LVal assignment failed (lhs=" << lhs << ", rhs=" << rhs << ")" << std::endl;
        }
    }
    else if (node->children[0]->isToken && node->children[0]->token.type == TokenType::RETURNTK)
    {
        if (node->children.size() > 2 && node->children[1]->name == "Exp")
        {
            IrValue *val = visitExp(node->children[1].get());
            if (val)
            {
                IrBuilder::insertInstr(new ReturnInstr(val));
            }
            else
            {
                std::cerr << "Error: visitStmt Return Exp failed" << std::endl;
            }
        }
        else
        {
            IrBuilder::insertInstr(new ReturnInstr(nullptr));
        }
    }
    else if (node->children[0]->name == "Block")
    {
        visitBlock(node->children[0].get());
    }
    else if (node->children[0]->isToken && node->children[0]->token.type == TokenType::IFTK)
    {
        IrBasicBlock *trueBlock = IrBuilder::createBasicBlock(getNewName("if_true"));
        IrBasicBlock *falseBlock = IrBuilder::createBasicBlock(getNewName("if_false"));
        IrBasicBlock *nextBlock = IrBuilder::createBasicBlock(getNewName("if_next"));

        visitCond(node->children[2].get(), trueBlock, falseBlock);

        IrBuilder::setBasicBlock(trueBlock);
        visitStmt(node->children[4].get());
        if (IrBuilder::currentBlock->instructions.empty() || IrBuilder::currentBlock->instructions.back()->instrType != InstrType::RET)
        {
            IrBuilder::insertInstr(new JumpInstr(nextBlock));
        }

        IrBuilder::setBasicBlock(falseBlock);
        if (node->children.size() > 5 && node->children[5]->isToken && node->children[5]->token.type == TokenType::ELSETK)
        {
            visitStmt(node->children[6].get());
        }
        if (IrBuilder::currentBlock->instructions.empty() || IrBuilder::currentBlock->instructions.back()->instrType != InstrType::RET)
        {
            IrBuilder::insertInstr(new JumpInstr(nextBlock));
        }

        IrBuilder::setBasicBlock(nextBlock);
    }
    else if (node->children[0]->isToken && node->children[0]->token.type == TokenType::FORTK)
    {
        // for ( [ForStmt] ; [Cond] ; [ForStmt] ) Stmt
        // Children: for, (, [ForStmt], ;, [Cond], ;, [ForStmt], ), Stmt

        int childIdx = 2;
        ASTNode *init = nullptr;
        ASTNode *cond = nullptr;
        ASTNode *step = nullptr;
        ASTNode *body = nullptr;

        if (node->children[childIdx]->name == "ForStmt")
        {
            init = node->children[childIdx].get();
            childIdx++;
        }
        childIdx++; // ;

        if (node->children[childIdx]->name == "Cond")
        {
            cond = node->children[childIdx].get();
            childIdx++;
        }
        childIdx++; // ;

        if (node->children[childIdx]->name == "ForStmt")
        {
            step = node->children[childIdx].get();
            childIdx++;
        }
        childIdx++; // )

        body = node->children[childIdx].get();

        if (init)
            visitForStmt(init);

        IrBasicBlock *condBlock = IrBuilder::createBasicBlock(getNewName("for_cond"));
        IrBasicBlock *bodyBlock = IrBuilder::createBasicBlock(getNewName("for_body"));
        IrBasicBlock *stepBlock = IrBuilder::createBasicBlock(getNewName("for_step"));
        IrBasicBlock *nextBlock = IrBuilder::createBasicBlock(getNewName("for_next"));

        IrBuilder::insertInstr(new JumpInstr(condBlock));

        IrBuilder::setBasicBlock(condBlock);
        if (cond)
        {
            visitCond(cond, bodyBlock, nextBlock);
        }
        else
        {
            IrBuilder::insertInstr(new JumpInstr(bodyBlock));
        }

        loopStack.push({stepBlock, nextBlock});

        IrBuilder::setBasicBlock(bodyBlock);
        visitStmt(body);
        if (IrBuilder::currentBlock->instructions.empty() || IrBuilder::currentBlock->instructions.back()->instrType != InstrType::RET)
        {
            IrBuilder::insertInstr(new JumpInstr(stepBlock));
        }

        loopStack.pop();

        IrBuilder::setBasicBlock(stepBlock);
        if (step)
            visitForStmt(step);
        IrBuilder::insertInstr(new JumpInstr(condBlock));

        IrBuilder::setBasicBlock(nextBlock);
    }
    else if (node->children[0]->isToken && node->children[0]->token.type == TokenType::WHILETK)
    {
        IrBasicBlock *condBlock = IrBuilder::createBasicBlock(getNewName("while_cond"));
        IrBasicBlock *bodyBlock = IrBuilder::createBasicBlock(getNewName("while_body"));
        IrBasicBlock *nextBlock = IrBuilder::createBasicBlock(getNewName("while_next"));

        IrBuilder::insertInstr(new JumpInstr(condBlock));

        IrBuilder::setBasicBlock(condBlock);
        visitCond(node->children[2].get(), bodyBlock, nextBlock);

        loopStack.push({condBlock, nextBlock});

        IrBuilder::setBasicBlock(bodyBlock);
        visitStmt(node->children[4].get());
        if (IrBuilder::currentBlock->instructions.empty() || IrBuilder::currentBlock->instructions.back()->instrType != InstrType::RET)
        {
            IrBuilder::insertInstr(new JumpInstr(condBlock));
        }

        loopStack.pop();

        IrBuilder::setBasicBlock(nextBlock);
    }
    else if (node->children[0]->isToken && node->children[0]->token.type == TokenType::BREAKTK)
    {
        if (!loopStack.empty())
        {
            IrBuilder::insertInstr(new JumpInstr(loopStack.top().second));
        }
    }
    else if (node->children[0]->isToken && node->children[0]->token.type == TokenType::CONTINUETK)
    {
        if (!loopStack.empty())
        {
            IrBuilder::insertInstr(new JumpInstr(loopStack.top().first));
        }
    }
    else if (node->children[0]->isToken && node->children[0]->token.type == TokenType::PRINTFTK)
    {
        std::string format = node->children[2]->token.value;
        if (format.size() >= 2)
            format = format.substr(1, format.size() - 2);

        std::vector<IrValue *> args;
        for (size_t i = 3; i < node->children.size(); ++i)
        {
            if (node->children[i]->name == "Exp")
            {
                args.push_back(visitExp(node->children[i].get()));
            }
        }

        int argIdx = 0;
        for (size_t i = 0; i < format.size(); ++i)
        {
            if (format[i] == '%' && i + 1 < format.size())
            {
                if (format[i + 1] == 'd')
                {
                    if (argIdx < (int)args.size())
                    {
                        Symbol *putintSym = findSymbol("putint");
                        if (putintSym && putintSym->llvmValue)
                        {
                            IrBuilder::insertInstr(new CallInstr((IrFunction *)putintSym->llvmValue, {args[argIdx++]}, getNewName("call")));
                        }
                    }
                    i++;
                }
                else if (format[i + 1] == 'c')
                {
                    if (argIdx < (int)args.size())
                    {
                        Symbol *putchSym = findSymbol("putch");
                        if (putchSym && putchSym->llvmValue)
                        {
                            IrBuilder::insertInstr(new CallInstr((IrFunction *)putchSym->llvmValue, {args[argIdx++]}, getNewName("call")));
                        }
                    }
                    i++;
                }
                else if (format[i + 1] == '%')
                {
                    Symbol *putchSym = findSymbol("putch");
                    if (putchSym && putchSym->llvmValue)
                    {
                        IrBuilder::insertInstr(new CallInstr((IrFunction *)putchSym->llvmValue, {IrConstantInt::get('%')}, getNewName("call")));
                    }
                    i++;
                }
                else
                {
                    Symbol *putchSym = findSymbol("putch");
                    if (putchSym && putchSym->llvmValue)
                    {
                        IrBuilder::insertInstr(new CallInstr((IrFunction *)putchSym->llvmValue, {IrConstantInt::get('%')}, getNewName("call")));
                    }
                }
            }
            else if (format[i] == '\\' && i + 1 < format.size())
            {
                int charCode = format[i + 1];
                if (format[i + 1] == 'n')
                    charCode = 10;
                else if (format[i + 1] == 't')
                    charCode = 9;
                else if (format[i + 1] == '\"')
                    charCode = 34;
                else if (format[i + 1] == '\\')
                    charCode = 92;
                else if (format[i + 1] == '0')
                    charCode = 0;

                Symbol *putchSym = findSymbol("putch");
                if (putchSym && putchSym->llvmValue)
                {
                    IrBuilder::insertInstr(new CallInstr((IrFunction *)putchSym->llvmValue, {IrConstantInt::get(charCode)}, getNewName("call")));
                }
                i++;
            }
            else
            {
                Symbol *putchSym = findSymbol("putch");
                if (putchSym && putchSym->llvmValue)
                {
                    IrBuilder::insertInstr(new CallInstr((IrFunction *)putchSym->llvmValue, {IrConstantInt::get(format[i])}, getNewName("call")));
                }
            }
        }
    }
    else if (node->children[0]->name == "Exp")
    {
        visitExp(node->children[0].get());
    }
}

IrValue *IRGenerator::visitExp(ASTNode *node)
{
    return visitAddExp(node->children[0].get());
}

IrValue *IRGenerator::visitAddExp(ASTNode *node)
{
    if (node->children.size() == 1)
    {
        return visitMulExp(node->children[0].get());
    }
    else
    {
        IrValue *lhs = visitAddExp(node->children[0].get());
        IrValue *rhs = visitMulExp(node->children[2].get());
        std::string op = node->children[1]->token.value;
        InstrType type = (op == "+") ? InstrType::ADD : InstrType::SUB;
        auto *instr = new AluInstr(type, lhs, rhs, getNewName("tmp"));
        IrBuilder::insertInstr(instr);
        return instr;
    }
}

IrValue *IRGenerator::visitMulExp(ASTNode *node)
{
    if (node->children.size() == 1)
    {
        return visitUnaryExp(node->children[0].get());
    }
    else
    {
        IrValue *lhs = visitMulExp(node->children[0].get());
        IrValue *rhs = visitUnaryExp(node->children[2].get());
        std::string op = node->children[1]->token.value;
        InstrType type;
        if (op == "*")
            type = InstrType::MUL;
        else if (op == "/")
            type = InstrType::SDIV;
        else
            type = InstrType::SREM;

        auto *instr = new AluInstr(type, lhs, rhs, getNewName("tmp"));
        IrBuilder::insertInstr(instr);
        return instr;
    }
}

IrValue *IRGenerator::visitUnaryExp(ASTNode *node)
{
    if (node->children[0]->name == "PrimaryExp")
    {
        return visitPrimaryExp(node->children[0].get());
    }
    else if (node->children[0]->name == "UnaryOp")
    {
        std::string op = node->children[0]->children[0]->token.value;
        IrValue *val = visitUnaryExp(node->children[1].get());
        if (op == "+")
            return val;
        if (op == "-")
        {
            auto *instr = new AluInstr(InstrType::SUB, IrConstantInt::get(0), val, getNewName("neg"));
            IrBuilder::insertInstr(instr);
            return instr;
        }
        if (op == "!")
        {
            auto *instr = new IcmpInstr(IcmpCond::EQ, val, IrConstantInt::get(0), getNewName("not"));
            IrBuilder::insertInstr(instr);
            auto *zext = new ZextInstr(instr, IrBaseType::getInt32(), getNewName("zext"));
            IrBuilder::insertInstr(zext);
            return zext;
        }
    }
    else if (node->children[0]->isToken && node->children[0]->token.type == TokenType::IDENFR)
    {
        std::string funcName = node->children[0]->token.value;
        Symbol *sym = findSymbol(funcName);
        if (!sym || !sym->llvmValue)
        {
            std::cerr << "Error: Symbol not found or no llvmValue in visitUnaryExp (call): " << funcName << std::endl;
            return nullptr;
        }

        IrFunction *func = (IrFunction *)sym->llvmValue;
        std::vector<IrValue *> args;

        if (node->children.size() > 3 && node->children[2]->name == "FuncRParams")
        {
            ASTNode *params = node->children[2].get();
            for (size_t i = 0; i < params->children.size(); i += 2)
            {
                IrValue *arg = visitExp(params->children[i].get());
                args.push_back(arg);
            }
        }

        auto *call = new CallInstr(func, args, getNewName("call"));
        IrBuilder::insertInstr(call);
        return call;
    }
    std::cerr << "Error: visitUnaryExp fell through for node with child: " << node->children[0]->name << std::endl;
    return nullptr;
}

IrValue *IRGenerator::visitPrimaryExp(ASTNode *node)
{
    if (node->children[0]->name == "LVal")
    {
        return visitLVal(node->children[0].get(), false);
    }
    else if (node->children[0]->name == "Number")
    {
        int val = std::stoi(node->children[0]->children[0]->token.value);
        return IrConstantInt::get(val);
    }
    else
    {
        return visitExp(node->children[1].get());
    }
}

IrValue *IRGenerator::visitLVal(ASTNode *node, bool isLeft)
{
    std::string name = node->children[0]->token.value;
    Symbol *sym = findSymbol(name);
    if (!sym)
    {
        std::cerr << "Error: Symbol not found in visitLVal: " << name << std::endl;
        return nullptr;
    }

    IrValue *ptr = sym->llvmValue;
    std::vector<IrValue *> indices;

    if (node->children.size() > 1)
    {
        for (size_t i = 1; i < node->children.size(); ++i)
        {
            if (node->children[i]->isToken && node->children[i]->token.type == TokenType::LBRACK)
            {
                IrValue *idx = visitExp(node->children[i + 1].get());
                indices.push_back(idx);
                i += 2;
            }
        }

        if (ptr->type->isPointer() && ((IrPointerType *)ptr->type)->pointedType->isArray())
        {
            indices.insert(indices.begin(), IrConstantInt::get(0));
        }
        else if (ptr->type->isPointer() && ((IrPointerType *)ptr->type)->pointedType->isPointer())
        {
            auto *loadPtr = new LoadInstr(ptr, getNewName("ptr_load"));
            IrBuilder::insertInstr(loadPtr);
            ptr = loadPtr;
        }

        if (!indices.empty())
        {
            auto *gep = new GepInstr(ptr, indices, getNewName("gep"));
            IrBuilder::insertInstr(gep);
            ptr = gep;
        }
    }
    else
    {
        if (ptr->type->isPointer() && ((IrPointerType *)ptr->type)->pointedType->isArray())
        {
            std::vector<IrValue *> zeros = {IrConstantInt::get(0), IrConstantInt::get(0)};
            auto *gep = new GepInstr(ptr, zeros, getNewName("gep_decay"));
            IrBuilder::insertInstr(gep);
            return gep;
        }
    }

    if (isLeft)
    {
        return ptr;
    }
    else
    {
        if (ptr->type->isPointer() && ((IrPointerType *)ptr->type)->pointedType->isArray())
        {
            std::vector<IrValue *> zeros = {IrConstantInt::get(0), IrConstantInt::get(0)};
            auto *gep = new GepInstr(ptr, zeros, getNewName("gep_decay"));
            IrBuilder::insertInstr(gep);
            return gep;
        }
        else if (ptr->type->isPointer() && ((IrPointerType *)ptr->type)->pointedType->isInt32())
        {
            auto *load = new LoadInstr(ptr, getNewName("load_" + name));
            IrBuilder::insertInstr(load);
            return load;
        }
        else if (ptr->type->isPointer() && ((IrPointerType *)ptr->type)->pointedType->isPointer())
        {
            auto *load = new LoadInstr(ptr, getNewName("load_ptr_" + name));
            IrBuilder::insertInstr(load);
            return load;
        }
        return ptr;
    }
}

void IRGenerator::visitCond(ASTNode *node, IrBasicBlock *trueBlock, IrBasicBlock *falseBlock)
{
    visitLOrExp(node->children[0].get(), trueBlock, falseBlock);
}

void IRGenerator::visitLOrExp(ASTNode *node, IrBasicBlock *trueBlock, IrBasicBlock *falseBlock)
{
    if (node->children.size() == 1)
    {
        visitLAndExp(node->children[0].get(), trueBlock, falseBlock);
    }
    else
    {
        IrBasicBlock *nextBlock = IrBuilder::createBasicBlock(getNewName("or_next"));
        visitLOrExp(node->children[0].get(), trueBlock, nextBlock);

        IrBuilder::setBasicBlock(nextBlock);
        visitLAndExp(node->children[2].get(), trueBlock, falseBlock);
    }
}

void IRGenerator::visitLAndExp(ASTNode *node, IrBasicBlock *trueBlock, IrBasicBlock *falseBlock)
{
    if (node->children.size() == 1)
    {
        IrValue *val = visitEqExp(node->children[0].get());
        if (val->type->isInt32())
        {
            auto *cmp = new IcmpInstr(IcmpCond::NE, val, IrConstantInt::get(0), getNewName("cond"));
            IrBuilder::insertInstr(cmp);
            val = cmp;
        }
        IrBuilder::insertInstr(new BranchInstr(val, trueBlock, falseBlock));
    }
    else
    {
        IrBasicBlock *nextBlock = IrBuilder::createBasicBlock(getNewName("and_next"));
        visitLAndExp(node->children[0].get(), nextBlock, falseBlock);

        IrBuilder::setBasicBlock(nextBlock);
        IrValue *val = visitEqExp(node->children[2].get());
        if (val->type->isInt32())
        {
            auto *cmp = new IcmpInstr(IcmpCond::NE, val, IrConstantInt::get(0), getNewName("cond"));
            IrBuilder::insertInstr(cmp);
            val = cmp;
        }
        IrBuilder::insertInstr(new BranchInstr(val, trueBlock, falseBlock));
    }
}

IrValue *IRGenerator::visitEqExp(ASTNode *node)
{
    if (node->children.size() == 1)
    {
        return visitRelExp(node->children[0].get());
    }
    IrValue *lhs = visitEqExp(node->children[0].get());
    IrValue *rhs = visitRelExp(node->children[2].get());

    if (lhs->type->isInt1() && rhs->type->isInt32())
    {
        auto *zext = new ZextInstr(lhs, IrBaseType::getInt32(), getNewName("zext"));
        IrBuilder::insertInstr(zext);
        lhs = zext;
    }
    else if (lhs->type->isInt32() && rhs->type->isInt1())
    {
        auto *zext = new ZextInstr(rhs, IrBaseType::getInt32(), getNewName("zext"));
        IrBuilder::insertInstr(zext);
        rhs = zext;
    }

    std::string op = node->children[1]->token.value;
    IcmpCond cond = (op == "==") ? IcmpCond::EQ : IcmpCond::NE;
    auto *instr = new IcmpInstr(cond, lhs, rhs, getNewName("tmp_eq"));
    IrBuilder::insertInstr(instr);
    return instr;
}

IrValue *IRGenerator::visitRelExp(ASTNode *node)
{
    if (node->children.size() == 1)
    {
        return visitAddExp(node->children[0].get());
    }
    IrValue *lhs = visitRelExp(node->children[0].get());
    IrValue *rhs = visitAddExp(node->children[2].get());

    if (lhs->type->isInt1() && rhs->type->isInt32())
    {
        auto *zext = new ZextInstr(lhs, IrBaseType::getInt32(), getNewName("zext"));
        IrBuilder::insertInstr(zext);
        lhs = zext;
    }
    else if (lhs->type->isInt32() && rhs->type->isInt1())
    {
        auto *zext = new ZextInstr(rhs, IrBaseType::getInt32(), getNewName("zext"));
        IrBuilder::insertInstr(zext);
        rhs = zext;
    }

    std::string op = node->children[1]->token.value;
    IcmpCond cond;
    if (op == "<")
        cond = IcmpCond::SLT;
    else if (op == ">")
        cond = IcmpCond::SGT;
    else if (op == "<=")
        cond = IcmpCond::SLE;
    else
        cond = IcmpCond::SGE;

    auto *instr = new IcmpInstr(cond, lhs, rhs, getNewName("tmp_rel"));
    IrBuilder::insertInstr(instr);
    return instr;
}

int IRGenerator::evaluateConstExp(ASTNode *node)
{
    if (!node)
        return 0;
    if (node->name == "ConstExp" || node->name == "Exp")
        return evaluateConstExp(node->children[0].get());

    if (node->name == "AddExp")
    {
        if (node->children.size() == 1)
            return evaluateConstExp(node->children[0].get());
        int lhs = evaluateConstExp(node->children[0].get());
        int rhs = evaluateConstExp(node->children[2].get());
        if (node->children[1]->token.value == "+")
            return lhs + rhs;
        else
            return lhs - rhs;
    }

    if (node->name == "MulExp")
    {
        if (node->children.size() == 1)
            return evaluateConstExp(node->children[0].get());
        int lhs = evaluateConstExp(node->children[0].get());
        int rhs = evaluateConstExp(node->children[2].get());
        std::string op = node->children[1]->token.value;
        if (op == "*")
            return lhs * rhs;
        if (op == "/")
            return rhs ? lhs / rhs : 0;
        if (op == "%")
            return rhs ? lhs % rhs : 0;
        return 0;
    }

    if (node->name == "UnaryExp")
    {
        if (node->children[0]->name == "PrimaryExp")
            return evaluateConstExp(node->children[0].get());
        if (node->children[0]->name == "UnaryOp")
        {
            int val = evaluateConstExp(node->children[1].get());
            std::string op = node->children[0]->children[0]->token.value;
            if (op == "+")
                return val;
            if (op == "-")
                return -val;
            if (op == "!")
                return !val;
        }
        return 0;
    }

    if (node->name == "PrimaryExp")
    {
        if (node->children[0]->name == "LVal")
            return evaluateConstExp(node->children[0].get());
        if (node->children[0]->name == "Number")
        {
            return std::stoi(node->children[0]->children[0]->token.value);
        }
        if (node->children[0]->isToken && node->children[0]->token.type == TokenType::LPARENT)
        {
            return evaluateConstExp(node->children[1].get());
        }
    }

    if (node->name == "LVal")
    {
        std::string name = node->children[0]->token.value;
        Symbol *sym = findSymbol(name);
        if (sym)
        {
            if (node->children.size() > 1)
            {
                // Handle 1D array access for now
                if (node->children.size() >= 4 && node->children[1]->token.type == TokenType::LBRACK)
                {
                    int index = evaluateConstExp(node->children[2].get());
                    if (index >= 0 && index < (int)sym->arrayValues.size())
                    {
                        return sym->arrayValues[index];
                    }
                }
            }
            else if (sym->isConst)
            {
                return sym->constVal;
            }
        }
    }

    return 0;
}

void IRGenerator::getGlobalInitVals(ASTNode *initVal, std::vector<IrConstant *> &vals)
{
    if (initVal->children.size() > 0 && initVal->children[0]->isToken && initVal->children[0]->token.type == TokenType::LBRACE)
    {
        for (const auto &child : initVal->children)
        {
            if (child->name == "InitVal" || child->name == "ConstInitVal")
            {
                getGlobalInitVals(child.get(), vals);
            }
            else if (child->name == "Exp" || child->name == "ConstExp")
            {
                int val = evaluateConstExp(child.get());
                vals.push_back(IrConstantInt::get(val));
            }
        }
    }
    else
    {
        // It's an expression
        ASTNode *exp = initVal->children[0].get();
        int val = evaluateConstExp(exp);
        vals.push_back(IrConstantInt::get(val));
    }
}

void IRGenerator::getLocalInitVals(ASTNode *initVal, std::vector<ASTNode *> &exprs)
{
    if (initVal->children.size() > 0 && initVal->children[0]->isToken && initVal->children[0]->token.type == TokenType::LBRACE)
    {
        for (const auto &child : initVal->children)
        {
            if (child->name == "InitVal" || child->name == "ConstInitVal")
            {
                getLocalInitVals(child.get(), exprs);
            }
            else if (child->name == "Exp" || child->name == "ConstExp")
            {
                exprs.push_back(child.get());
            }
        }
    }
    else
    {
        // It's an expression
        exprs.push_back(initVal->children[0].get());
    }
}

void IRGenerator::visitForStmt(ASTNode *node)
{
    // ForStmt -> LVal '=' Exp { ',' LVal '=' Exp }
    for (size_t i = 0; i < node->children.size(); ++i)
    {
        if (node->children[i]->name == "LVal")
        {
            IrValue *lhs = visitLVal(node->children[i].get(), true);
            // Next should be ASSIGN
            // Next should be Exp
            if (i + 2 < node->children.size() && node->children[i + 2]->name == "Exp")
            {
                IrValue *rhs = visitExp(node->children[i + 2].get());
                if (lhs && rhs)
                {
                    IrBuilder::insertInstr(new StoreInstr(rhs, lhs));
                }
            }
            i += 2; // Skip ASSIGN and Exp
        }
    }
}
