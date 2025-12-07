#include "IrBuilder.hpp"

IrModule* IrBuilder::module = nullptr;
IrFunction* IrBuilder::currentFunction = nullptr;
IrBasicBlock* IrBuilder::currentBlock = nullptr;
