#include "IrValue.hpp"
#include "IrUse.hpp"
#include "IrUser.hpp"
#include <algorithm>

void IrValue::addUse(IrUse* use) {
    useList.push_back(use);
}

void IrValue::removeUse(IrUser* user) {
    useList.remove_if([user](IrUse* use) { return use->user == user; });
}

void IrValue::replaceAllUsesWith(IrValue* newValue) {
    for (auto* use : useList) {
        use->value = newValue;
        newValue->addUse(use);
    }
    useList.clear();
}

void IrUser::addOperand(IrValue* v) {
    auto* use = new IrUse(this, v);
    operandList.push_back(use);
    if (v) v->addUse(use);
}
