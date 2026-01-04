#include "Mem2Reg.hpp"

#include "../midend/llvm/IrModule.hpp"
#include "../midend/llvm/value/IrFunction.hpp"
#include "../midend/llvm/value/IrBasicBlock.hpp"
#include "../midend/llvm/type/IrPointerType.hpp"
#include "../midend/llvm/type/IrBaseType.hpp"
#include "../midend/llvm/instr/AllocaInstr.hpp"
#include "../midend/llvm/instr/LoadInstr.hpp"
#include "../midend/llvm/instr/StoreInstr.hpp"
#include "../midend/llvm/instr/BranchInstr.hpp"
#include "../midend/llvm/instr/JumpInstr.hpp"
#include "../midend/llvm/instr/ReturnInstr.hpp"
#include "../midend/llvm/instr/PhiInstr.hpp"
#include "../midend/llvm/value/IrConstantInt.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace optimize
{

    namespace
    {

        struct Cfg
        {
            std::vector<IrBasicBlock *> blocks; // reachable blocks in a stable order
            std::unordered_map<IrBasicBlock *, std::vector<IrBasicBlock *>> succ;
            std::unordered_map<IrBasicBlock *, std::vector<IrBasicBlock *>> pred;
        };

        static void detachInstrOperands(Instr *instr);

        static Instr *getTerminator(IrBasicBlock *bb)
        {
            if (bb->instructions.empty())
                return nullptr;
            return bb->instructions.back();
        }

        static Cfg buildCfg(IrFunction *func)
        {
            Cfg cfg;
            if (func->blocks.empty())
                return cfg;

            // Reachability from entry
            auto *entry = func->blocks.front();
            std::vector<IrBasicBlock *> stack;
            std::unordered_set<IrBasicBlock *> visited;

            auto addSucc = [&](IrBasicBlock *from, IrBasicBlock *to)
            {
                cfg.succ[from].push_back(to);
                cfg.pred[to].push_back(from);
            };

            stack.push_back(entry);
            visited.insert(entry);

            while (!stack.empty())
            {
                auto *bb = stack.back();
                stack.pop_back();
                cfg.blocks.push_back(bb);

                Instr *term = getTerminator(bb);
                if (!term)
                    continue;

                if (term->instrType == InstrType::BR)
                {
                    auto *t = dynamic_cast<IrBasicBlock *>(term->getOperand(1));
                    auto *f = dynamic_cast<IrBasicBlock *>(term->getOperand(2));
                    if (t)
                    {
                        addSucc(bb, t);
                        if (!visited.count(t))
                        {
                            visited.insert(t);
                            stack.push_back(t);
                        }
                    }
                    if (f)
                    {
                        addSucc(bb, f);
                        if (!visited.count(f))
                        {
                            visited.insert(f);
                            stack.push_back(f);
                        }
                    }
                }
                else if (term->instrType == InstrType::JUMP)
                {
                    auto *to = dynamic_cast<IrBasicBlock *>(term->getOperand(0));
                    if (to)
                    {
                        addSucc(bb, to);
                        if (!visited.count(to))
                        {
                            visited.insert(to);
                            stack.push_back(to);
                        }
                    }
                }
                else
                {
                    // RET or others: no succ
                }
            }

            // Ensure all reachable blocks have entries
            for (auto *bb : cfg.blocks)
            {
                (void)cfg.succ[bb];
                (void)cfg.pred[bb];
            }
            return cfg;
        }

        static bool isTerminatorInstr(const Instr *instr)
        {
            if (!instr)
                return false;
            return instr->instrType == InstrType::BR || instr->instrType == InstrType::JUMP || instr->instrType == InstrType::RET;
        }

        // Some IR builders leave redundant instructions after a terminator (e.g.,
        // break/continue lowering emitting multiple jumps). These instructions are
        // unreachable and will confuse CFG/dominator-based passes if we treat the
        // last instruction as the terminator.
        static void truncateAfterFirstTerminator(IrFunction *func)
        {
            if (!func)
                return;
            for (auto *bb : func->blocks)
            {
                if (!bb)
                    continue;
                bool seenTerm = false;
                for (auto it = bb->instructions.begin(); it != bb->instructions.end();)
                {
                    Instr *instr = *it;
                    if (!seenTerm)
                    {
                        if (isTerminatorInstr(instr))
                            seenTerm = true;
                        ++it;
                        continue;
                    }

                    // Erase unreachable tail.
                    detachInstrOperands(instr);
                    it = bb->instructions.erase(it);
                }
            }
        }

        static std::unordered_map<IrBasicBlock *, std::unordered_set<IrBasicBlock *>>
        computeDominators(const Cfg &cfg, IrBasicBlock *entry)
        {
            std::unordered_map<IrBasicBlock *, std::unordered_set<IrBasicBlock *>> dom;
            std::unordered_set<IrBasicBlock *> all;
            for (auto *b : cfg.blocks)
                all.insert(b);

            for (auto *b : cfg.blocks)
            {
                if (b == entry)
                    dom[b] = {b};
                else
                    dom[b] = all;
            }

            bool changed = true;
            while (changed)
            {
                changed = false;
                for (auto *b : cfg.blocks)
                {
                    if (b == entry)
                        continue;

                    // Intersect dominators of predecessors
                    std::unordered_set<IrBasicBlock *> newDom;
                    bool first = true;
                    for (auto *p : cfg.pred.at(b))
                    {
                        if (first)
                        {
                            newDom = dom[p];
                            first = false;
                        }
                        else
                        {
                            std::unordered_set<IrBasicBlock *> tmp;
                            for (auto *x : newDom)
                            {
                                if (dom[p].count(x))
                                    tmp.insert(x);
                            }
                            newDom.swap(tmp);
                        }
                    }

                    if (first)
                    {
                        // Unreachable-by-preds in reachable set shouldn't happen, but keep self.
                        newDom.clear();
                    }
                    newDom.insert(b);

                    if (newDom != dom[b])
                    {
                        dom[b].swap(newDom);
                        changed = true;
                    }
                }
            }

            return dom;
        }

        static std::unordered_map<IrBasicBlock *, IrBasicBlock *>
        computeIdom(const Cfg &cfg,
                    IrBasicBlock *entry,
                    const std::unordered_map<IrBasicBlock *, std::unordered_set<IrBasicBlock *>> &dom)
        {
            std::unordered_map<IrBasicBlock *, IrBasicBlock *> idom;
            idom[entry] = nullptr;

            for (auto *b : cfg.blocks)
            {
                if (b == entry)
                    continue;
                std::vector<IrBasicBlock *> candidates;
                for (auto *d : dom.at(b))
                {
                    if (d != b)
                        candidates.push_back(d);
                }
                IrBasicBlock *best = nullptr;
                for (auto *c : candidates)
                {
                    // Pick the strict dominator that is dominated by all other strict dominators
                    // (i.e., the closest one to b).
                    bool ok = true;
                    for (auto *other : candidates)
                    {
                        if (other == c)
                            continue;
                        if (!dom.at(c).count(other))
                        {
                            ok = false;
                            break;
                        }
                    }
                    if (ok)
                    {
                        best = c;
                        break;
                    }
                }
                idom[b] = best;
            }

            return idom;
        }

        static std::unordered_map<IrBasicBlock *, std::vector<IrBasicBlock *>>
        buildDomTreeChildren(const Cfg &cfg, const std::unordered_map<IrBasicBlock *, IrBasicBlock *> &idom)
        {
            std::unordered_map<IrBasicBlock *, std::vector<IrBasicBlock *>> children;
            for (auto *b : cfg.blocks)
            {
                (void)children[b];
            }
            for (auto *b : cfg.blocks)
            {
                auto it = idom.find(b);
                if (it == idom.end())
                    continue;
                IrBasicBlock *parent = it->second;
                if (parent)
                    children[parent].push_back(b);
            }
            return children;
        }

        static std::unordered_map<IrBasicBlock *, std::unordered_set<IrBasicBlock *>>
        computeDominanceFrontier(const Cfg &cfg,
                                 const std::unordered_map<IrBasicBlock *, IrBasicBlock *> &idom,
                                 const std::unordered_map<IrBasicBlock *, std::vector<IrBasicBlock *>> &children)
        {
            std::unordered_map<IrBasicBlock *, std::unordered_set<IrBasicBlock *>> df;
            for (auto *b : cfg.blocks)
                df[b] = {};

            // Local DF
            for (auto *b : cfg.blocks)
            {
                for (auto *s : cfg.succ.at(b))
                {
                    if (idom.at(s) != b)
                        df[b].insert(s);
                }
            }

            // Up DF
            // Post-order traversal of dom tree: easiest do repeated until stable due to small size
            bool changed = true;
            while (changed)
            {
                changed = false;
                for (auto *b : cfg.blocks)
                {
                    for (auto *c : children.at(b))
                    {
                        for (auto *w : df[c])
                        {
                            if (idom.at(w) != b && !df[b].count(w))
                            {
                                df[b].insert(w);
                                changed = true;
                            }
                        }
                    }
                }
            }

            return df;
        }

        static bool isPromotable(AllocaInstr *allocaInstr)
        {
            // Only promote scalars (no arrays). Keep pointers/arrays in memory.
            auto *pty = dynamic_cast<IrPointerType *>(allocaInstr->type);
            if (!pty)
                return false;
            if (pty->pointedType->isArray())
                return false;

            // Must be used only by load/store with the alloca as the pointer operand.
            for (auto *use : allocaInstr->useList)
            {
                auto *user = use->user;
                auto *instr = dynamic_cast<Instr *>(user);
                if (!instr)
                    return false;
                if (instr->instrType == InstrType::LOAD)
                {
                    if (instr->getOperand(0) != allocaInstr)
                        return false;
                }
                else if (instr->instrType == InstrType::STORE)
                {
                    if (instr->getOperand(1) != allocaInstr)
                        return false;
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

        static IrConstantInt *makeZero(IrType *t)
        {
            if (t->isInt1())
                return IrConstantInt::get1(false);
            if (t->isInt8())
                return new IrConstantInt(IrBaseType::getInt8(), 0);
            return IrConstantInt::get(0);
        }

        static bool isPhi(Instr *instr)
        {
            return instr && instr->instrType == InstrType::PHI;
        }

        static void insertPhiAtBlockStart(IrBasicBlock *bb, PhiInstr *phi)
        {
            // Insert after existing phi nodes.
            auto it = bb->instructions.begin();
            while (it != bb->instructions.end() && isPhi(*it))
            {
                ++it;
            }
            bb->instructions.insert(it, phi);
            phi->parentBlock = bb;
        }

        static void detachInstrOperands(Instr *instr)
        {
            for (auto *opUse : instr->operandList)
            {
                if (opUse && opUse->value)
                {
                    opUse->value->useList.remove(opUse);
                }
            }
        }

    } // namespace

    void Mem2RegPass::run(IrModule *module)
    {
        if (!module)
            return;

        int phiCounter = 0;

        for (auto *func : module->functions)
        {
            if (!func || func->isBuiltin)
                continue;
            if (func->blocks.empty())
                continue;

            truncateAfterFirstTerminator(func);

            Cfg cfg = buildCfg(func);
            if (cfg.blocks.empty())
                continue;

            auto *entry = func->blocks.front();
            auto dom = computeDominators(cfg, entry);
            auto idom = computeIdom(cfg, entry, dom);
            auto children = buildDomTreeChildren(cfg, idom);
            auto df = computeDominanceFrontier(cfg, idom, children);

            // Collect promotable allocas
            std::vector<AllocaInstr *> promotable;
            std::unordered_set<AllocaInstr *> promotableSet;
            for (auto *bb : cfg.blocks)
            {
                for (auto *instr : bb->instructions)
                {
                    if (instr && instr->parentBlock == nullptr)
                        instr->parentBlock = bb;
                    if (auto *allocaInstr = dynamic_cast<AllocaInstr *>(instr))
                    {
                        if (isPromotable(allocaInstr))
                            promotable.push_back(allocaInstr);
                    }
                }
            }
            for (auto *a : promotable)
                promotableSet.insert(a);
            if (promotable.empty())
                continue;

            // For each alloca, place phi nodes
            std::unordered_map<AllocaInstr *, std::unordered_map<IrBasicBlock *, PhiInstr *>> phiFor;

            for (auto *a : promotable)
            {
                std::unordered_set<IrBasicBlock *> defBlocks;
                for (auto *use : a->useList)
                {
                    auto *instr = dynamic_cast<Instr *>(use->user);
                    if (instr && instr->instrType == InstrType::STORE && instr->getOperand(1) == a)
                    {
                        if (instr->parentBlock)
                            defBlocks.insert(instr->parentBlock);
                    }
                }

                std::vector<IrBasicBlock *> work(defBlocks.begin(), defBlocks.end());
                std::unordered_set<IrBasicBlock *> hasPhi;

                while (!work.empty())
                {
                    auto *x = work.back();
                    work.pop_back();

                    for (auto *y : df[x])
                    {
                        if (hasPhi.count(y))
                            continue;
                        auto *pty = dynamic_cast<IrPointerType *>(a->type);
                        auto *phi = new PhiInstr(pty->pointedType, "%phi" + std::to_string(phiCounter++));
                        insertPhiAtBlockStart(y, phi);
                        phiFor[a][y] = phi;
                        hasPhi.insert(y);
                        if (!defBlocks.count(y))
                        {
                            work.push_back(y);
                        }
                    }
                }
            }

            // Rename using dominator tree DFS
            std::unordered_map<AllocaInstr *, std::vector<IrValue *>> stacks;
            for (auto *a : promotable)
            {
                auto *pty = dynamic_cast<IrPointerType *>(a->type);
                stacks[a].push_back(makeZero(pty->pointedType));
            }

            std::function<void(IrBasicBlock *)> rename;
            rename = [&](IrBasicBlock *bb)
            {
                std::unordered_map<AllocaInstr *, int> pushed;

                // Push phi defs
                for (auto it = bb->instructions.begin(); it != bb->instructions.end(); ++it)
                {
                    auto *instr = *it;
                    if (!isPhi(instr))
                        break;

                    auto *phi = dynamic_cast<PhiInstr *>(instr);
                    if (!phi)
                        continue;

                    for (auto *a : promotable)
                    {
                        auto itPhi = phiFor[a].find(bb);
                        if (itPhi != phiFor[a].end() && itPhi->second == phi)
                        {
                            stacks[a].push_back(phi);
                            pushed[a]++;
                            break;
                        }
                    }
                }

                // Process instructions
                for (auto it = bb->instructions.begin(); it != bb->instructions.end();)
                {
                    Instr *instr = *it;

                    // Skip phi nodes
                    if (isPhi(instr))
                    {
                        ++it;
                        continue;
                    }

                    bool erased = false;

                    if (auto *load = dynamic_cast<LoadInstr *>(instr))
                    {
                        auto *ptr = load->getOperand(0);
                        auto *a = dynamic_cast<AllocaInstr *>(ptr);
                        if (a && promotableSet.count(a))
                        {
                            IrValue *repl = stacks[a].empty() ? makeZero(load->type) : stacks[a].back();
                            load->replaceAllUsesWith(repl);
                            detachInstrOperands(load);
                            it = bb->instructions.erase(it);
                            erased = true;
                        }
                    }
                    else if (auto *store = dynamic_cast<StoreInstr *>(instr))
                    {
                        auto *ptr = store->getOperand(1);
                        auto *a = dynamic_cast<AllocaInstr *>(ptr);
                        if (a && promotableSet.count(a))
                        {
                            IrValue *val = store->getOperand(0);
                            stacks[a].push_back(val);
                            pushed[a]++;
                            detachInstrOperands(store);
                            it = bb->instructions.erase(it);
                            erased = true;
                        }
                    }

                    if (!erased)
                        ++it;
                }

                // Fill phi operands in successors
                for (auto *succ : cfg.succ[bb])
                {
                    for (auto *a : promotable)
                    {
                        auto itPhi = phiFor[a].find(succ);
                        if (itPhi == phiFor[a].end())
                            continue;
                        PhiInstr *phi = itPhi->second;
                        IrValue *incoming = stacks[a].empty() ? makeZero(phi->type) : stacks[a].back();
                        phi->addIncoming(incoming, bb);
                    }
                }

                // Recurse
                for (auto *child : children[bb])
                {
                    rename(child);
                }

                // Pop pushes
                for (auto &[a, cnt] : pushed)
                {
                    while (cnt-- > 0 && !stacks[a].empty())
                        stacks[a].pop_back();
                }
            };

            rename(entry);

            // Remove now-dead promotable allocas (if no uses remain)
            for (auto *bb : cfg.blocks)
            {
                for (auto it = bb->instructions.begin(); it != bb->instructions.end();)
                {
                    auto *instr = *it;
                    auto *allocaInstr = dynamic_cast<AllocaInstr *>(instr);
                    if (allocaInstr && promotableSet.count(allocaInstr))
                    {
                        if (allocaInstr->useList.empty())
                        {
                            detachInstrOperands(allocaInstr);
                            it = bb->instructions.erase(it);
                            continue;
                        }
                    }
                    ++it;
                }
            }
        }
    }

} // namespace optimize
