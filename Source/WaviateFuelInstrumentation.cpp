#include "WaviateFuelInstrumentation.h"

#include <cstdint>
#include <vector>

#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

namespace
{
constexpr uint64_t functionEntryCost = 16;
constexpr uint64_t basicBlockEntryCost = 1;
constexpr uint64_t loopBackedgeCost = 32;

bool shouldSkipFunction (const llvm::Function& function)
{
    if (function.isDeclaration() || function.empty() || function.isIntrinsic())
        return true;

    const auto name = function.getName();
    return name == "waviate_consume_fuel"
        || name == "waviate_fuel_trap"
        || name.starts_with ("llvm.");
}

llvm::Constant* getDefaultReturnValue (llvm::Type* returnType)
{
    if (returnType->isVoidTy())
        return nullptr;

    return llvm::Constant::getNullValue (returnType);
}

bool replaceUnreachableTerminatorsWithDefaultReturns (llvm::Function& function)
{
    std::vector<llvm::UnreachableInst*> unreachableTerminators;

    for (auto& block : function)
        if (auto* unreachable = llvm::dyn_cast<llvm::UnreachableInst> (block.getTerminator()))
            unreachableTerminators.push_back (unreachable);

    for (auto* unreachable : unreachableTerminators)
    {
        llvm::IRBuilder<> builder (unreachable);

        if (auto* defaultValue = getDefaultReturnValue (function.getReturnType()))
            builder.CreateRet (defaultValue);
        else
            builder.CreateRetVoid();

        unreachable->eraseFromParent();
    }

    return ! unreachableTerminators.empty();
}

bool removeNoreturnAssumptions (llvm::Function& function)
{
    auto changed = false;

    if (function.hasFnAttribute (llvm::Attribute::NoReturn))
    {
        function.removeFnAttr (llvm::Attribute::NoReturn);
        changed = true;
    }

    for (auto& block : function)
    {
        for (auto& instruction : block)
        {
            if (auto* call = llvm::dyn_cast<llvm::CallBase> (&instruction);
                call != nullptr && call->hasFnAttr (llvm::Attribute::NoReturn))
            {
                call->removeFnAttr (llvm::Attribute::NoReturn);
                changed = true;
            }
        }
    }

    return changed;
}

bool insertFuelGuard (llvm::Instruction& insertionPoint,
                      uint64_t cost,
                      llvm::FunctionCallee consumeFuel,
                      llvm::FunctionCallee trapFuel)
{
    auto& context = insertionPoint.getContext();
    auto* function = insertionPoint.getFunction();
    auto* returnType = function->getReturnType();

    if (returnType->isTokenTy())
        return false;

    llvm::IRBuilder<> builder (&insertionPoint);
    auto* costValue = llvm::ConstantInt::get (llvm::Type::getInt64Ty (context), cost);
    auto* ok = builder.CreateCall (consumeFuel, costValue, "waviate.fuel.ok");
    auto* exhausted = builder.CreateICmpEQ (ok,
                                           llvm::ConstantInt::get (llvm::Type::getInt8Ty (context), 0),
                                           "waviate.fuel.exhausted");

    auto* thenTerminator = llvm::SplitBlockAndInsertIfThen (exhausted, &insertionPoint, false);
    llvm::IRBuilder<> failBuilder (thenTerminator);
    failBuilder.CreateCall (trapFuel);

    if (auto* defaultValue = getDefaultReturnValue (returnType))
        failBuilder.CreateRet (defaultValue);
    else
        failBuilder.CreateRetVoid();

    thenTerminator->eraseFromParent();
    return true;
}

bool instrumentFunction (llvm::Function& function,
                         llvm::FunctionCallee consumeFuel,
                         llvm::FunctionCallee trapFuel)
{
    auto changed = removeNoreturnAssumptions (function);
    changed |= replaceUnreachableTerminatorsWithDefaultReturns (function);

    llvm::DominatorTree dominatorTree (function);
    llvm::SmallPtrSet<llvm::BasicBlock*, 16> loopHeaders;

    for (auto& block : function)
    {
        const auto* terminator = block.getTerminator();

        for (unsigned i = 0; i < terminator->getNumSuccessors(); ++i)
        {
            auto* successor = terminator->getSuccessor (i);

            if (dominatorTree.dominates (successor, &block))
                loopHeaders.insert (successor);
        }
    }

    struct Insertion final
    {
        llvm::Instruction* instruction = nullptr;
        uint64_t cost = 0;
    };

    std::vector<Insertion> insertions;
    insertions.reserve (function.size());

    for (auto& block : function)
    {
        auto insertion = block.getFirstInsertionPt();
        if (&block == &function.getEntryBlock())
        {
            while (insertion != block.end() && llvm::isa<llvm::AllocaInst> (*insertion))
                ++insertion;
        }

        if (insertion == block.end())
            continue;

        auto cost = basicBlockEntryCost;

        if (&block == &function.getEntryBlock())
            cost += functionEntryCost;

        if (loopHeaders.contains (&block))
            cost += loopBackedgeCost;

        insertions.push_back ({ &*insertion, cost });
    }

    for (auto& insertion : insertions)
        changed |= insertFuelGuard (*insertion.instruction, insertion.cost, consumeFuel, trapFuel);

    return changed;
}

bool instrumentModule (llvm::Module& module)
{
    auto& context = module.getContext();
    auto* int8Type = llvm::Type::getInt8Ty (context);
    auto* int64Type = llvm::Type::getInt64Ty (context);
    auto* voidType = llvm::Type::getVoidTy (context);

    auto consumeFuel = module.getOrInsertFunction ("waviate_consume_fuel", int8Type, int64Type);
    auto trapFuel = module.getOrInsertFunction ("waviate_fuel_trap", voidType);

    auto changed = false;

    for (auto& function : module)
        if (! shouldSkipFunction (function))
            changed |= instrumentFunction (function, consumeFuel, trapFuel);

    return changed;
}

class WaviateFuelInstrumentationPass final
    : public llvm::PassInfoMixin<WaviateFuelInstrumentationPass>
{
public:
    llvm::PreservedAnalyses run (llvm::Module& module, llvm::ModuleAnalysisManager&)
    {
        return instrumentModule (module) ? llvm::PreservedAnalyses::none()
                                         : llvm::PreservedAnalyses::all();
    }
};
} // namespace

namespace waviate::safety
{
bool instrumentModuleWithFuel (llvm::Module& module)
{
    llvm::ModuleAnalysisManager analysisManager;
    WaviateFuelInstrumentationPass pass;
    const auto preserved = pass.run (module, analysisManager);
    return ! preserved.areAllPreserved();
}
} // namespace waviate::safety
