#include "WaviateSafetyValidator.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>

namespace
{
constexpr uint64_t maxSingleStaticAllocaBytes = 4096;
constexpr uint64_t maxFunctionStaticAllocaBytes = 16384;

bool isAllowedExternalFunction (llvm::StringRef name)
{
    static constexpr llvm::StringLiteral pureMathFunctions[] = {
        "acos", "acosf", "asin", "asinf", "atan", "atanf", "atan2", "atan2f",
        "cos", "cosf", "sin", "sinf", "tan", "tanf",
        "acosh", "acoshf", "asinh", "asinhf", "atanh", "atanhf",
        "cosh", "coshf", "sinh", "sinhf", "tanh", "tanhf",
        "exp", "expf", "exp2", "exp2f", "expm1", "expm1f",
        "frexp", "frexpf", "ilogb", "ilogbf", "ldexp", "ldexpf",
        "log", "logf", "log10", "log10f", "log1p", "log1pf", "log2", "log2f",
        "logb", "logbf", "modf", "modff", "scalbn", "scalbnf", "scalbln", "scalblnf",
        "cbrt", "cbrtf", "fabs", "fabsf", "hypot", "hypotf", "pow", "powf",
        "sqrt", "sqrtf", "erf", "erff", "erfc", "erfcf", "lgamma", "lgammaf",
        "tgamma", "tgammaf", "ceil", "ceilf", "floor", "floorf",
        "nearbyint", "nearbyintf", "rint", "rintf", "lrint", "lrintf",
        "llrint", "llrintf", "round", "roundf", "lround", "lroundf",
        "llround", "llroundf", "trunc", "truncf", "fmod", "fmodf",
        "remainder", "remainderf", "remquo", "remquof", "copysign", "copysignf",
        "nan", "nanf", "nextafter", "nextafterf", "nexttoward", "nexttowardf",
        "fmax", "fmaxf", "fmin", "fminf", "fdim", "fdimf", "fma", "fmaf"
    };

    if (name == "waviate_consume_fuel"
        || name == "waviate_fuel_trap"
        || name == "__waviate_internal_arena_allocate"
        || name == "__waviate_internal_arena_generation"
        || name == "waviate_load_audio_from_location")
    {
        return true;
    }

    for (const auto allowed : pureMathFunctions)
        if (name == allowed)
            return true;

    return false;
}

void addDiagnostic (std::ostringstream& diagnostics, const std::string& message)
{
    diagnostics << "- " << message << '\n';
}

std::string functionName (const llvm::Function& function)
{
    return function.getName().empty() ? std::string ("<anonymous>") : function.getName().str();
}

bool constantContainsForbiddenAddressCast (const llvm::Constant* constant)
{
    if (constant == nullptr)
        return false;

    if (const auto* expression = llvm::dyn_cast<llvm::ConstantExpr> (constant))
    {
        const auto opcode = expression->getOpcode();

        if (opcode == llvm::Instruction::IntToPtr || opcode == llvm::Instruction::PtrToInt)
            return true;
    }

    for (const auto& operand : constant->operands())
        if (constantContainsForbiddenAddressCast (llvm::dyn_cast<llvm::Constant> (operand.get())))
            return true;

    return false;
}

bool validateGlobals (const llvm::Module& module, std::ostringstream& diagnostics)
{
    auto ok = true;

    for (const auto& global : module.globals())
    {
        const auto name = global.getName();

        if (name == "llvm.used" || name == "llvm.compiler.used")
            continue;

        if (name == "llvm.global_ctors" || name == "llvm.global_dtors")
        {
            addDiagnostic (diagnostics, "global constructors/destructors are not allowed in shader code");
            ok = false;
            continue;
        }

        if (global.isDeclaration())
        {
            addDiagnostic (diagnostics, "external global '" + name.str() + "' is not allowed");
            ok = false;
            continue;
        }

        if (! global.isConstant())
        {
            addDiagnostic (diagnostics, "mutable global/static storage '" + name.str()
                                      + "' is not allowed; persistent script state must use a future Waviate API");
            ok = false;
        }

        if (constantContainsForbiddenAddressCast (global.getInitializer()))
        {
            addDiagnostic (diagnostics, "global '" + name.str()
                                      + "' contains an int/pointer address cast");
            ok = false;
        }
    }

    for (const auto& alias : module.aliases())
    {
        addDiagnostic (diagnostics, "global alias '" + alias.getName().str() + "' is not allowed");
        ok = false;
    }

    for (const auto& ifunc : module.ifuncs())
    {
        addDiagnostic (diagnostics, "indirect function '" + ifunc.getName().str() + "' is not allowed");
        ok = false;
    }

    return ok;
}

bool validateExternalFunctions (const llvm::Module& module, std::ostringstream& diagnostics)
{
    auto ok = true;

    for (const auto& function : module.functions())
    {
        if (! function.isDeclaration() || function.isIntrinsic())
            continue;

        if (! isAllowedExternalFunction (function.getName()))
        {
            addDiagnostic (diagnostics, "external function '" + functionName (function)
                                      + "' is not an audited Waviate runtime capability");
            ok = false;
        }
    }

    return ok;
}

bool validateAlloca (const llvm::Module& module, std::ostringstream& diagnostics)
{
    const auto& dataLayout = module.getDataLayout();
    auto ok = true;

    for (const auto& function : module.functions())
    {
        if (function.isDeclaration())
            continue;

        uint64_t functionStackBytes = 0;

        for (const auto& instruction : llvm::instructions (function))
        {
            const auto* alloca = llvm::dyn_cast<llvm::AllocaInst> (&instruction);

            if (alloca == nullptr)
                continue;

            if (! alloca->isStaticAlloca())
            {
                addDiagnostic (diagnostics, "dynamic stack allocation/VLA in '" + functionName (function)
                                          + "' is not allowed; use wav.newArray<T>(n)");
                ok = false;
                continue;
            }

            const auto* arraySize = llvm::dyn_cast<llvm::ConstantInt> (alloca->getArraySize());
            if (arraySize == nullptr)
            {
                addDiagnostic (diagnostics, "non-constant stack allocation in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            const auto typeSize = dataLayout.getTypeAllocSize (alloca->getAllocatedType());
            if (typeSize.isScalable())
            {
                addDiagnostic (diagnostics, "scalable stack allocation in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            const auto elementBytes = static_cast<uint64_t> (typeSize.getFixedValue());
            const auto count = arraySize->getZExtValue();

            if (elementBytes != 0 && count > (UINT64_MAX / elementBytes))
            {
                addDiagnostic (diagnostics, "stack allocation size overflow in '" + functionName (function)
                                          + "' is not allowed; use wav.newArray<T>(n)");
                ok = false;
                continue;
            }

            const auto totalBytes = elementBytes * count;

            if (functionStackBytes > UINT64_MAX - totalBytes)
            {
                addDiagnostic (diagnostics, "estimated stack frame overflow in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            functionStackBytes += totalBytes;

            if (totalBytes > maxSingleStaticAllocaBytes)
            {
                addDiagnostic (diagnostics, "stack allocation of " + std::to_string (totalBytes)
                                          + " bytes in '" + functionName (function)
                                          + "' exceeds the " + std::to_string (maxSingleStaticAllocaBytes)
                                          + " byte per-object limit; use wav.newArray<T>(n)");
                ok = false;
            }
        }

        if (functionStackBytes > maxFunctionStaticAllocaBytes)
        {
            addDiagnostic (diagnostics, "estimated stack frame of " + std::to_string (functionStackBytes)
                                      + " bytes in '" + functionName (function)
                                      + "' exceeds the " + std::to_string (maxFunctionStaticAllocaBytes)
                                      + " byte function limit");
            ok = false;
        }
    }

    return ok;
}

bool validateInstructions (const llvm::Module& module, std::ostringstream& diagnostics)
{
    auto ok = true;

    for (const auto& function : module.functions())
    {
        if (function.isDeclaration())
            continue;

        for (const auto& instruction : llvm::instructions (function))
        {
            if (llvm::isa<llvm::IntToPtrInst> (instruction) || llvm::isa<llvm::PtrToIntInst> (instruction))
            {
                addDiagnostic (diagnostics, "int/pointer address casts in '" + functionName (function)
                                          + "' are not allowed");
                ok = false;
                continue;
            }

            if (llvm::isa<llvm::IndirectBrInst> (instruction))
            {
                addDiagnostic (diagnostics, "indirect branches in '" + functionName (function)
                                          + "' are not allowed");
                ok = false;
                continue;
            }

            if (llvm::isa<llvm::InvokeInst> (instruction)
                || llvm::isa<llvm::ResumeInst> (instruction)
                || llvm::isa<llvm::LandingPadInst> (instruction)
                || llvm::isa<llvm::CatchSwitchInst> (instruction)
                || llvm::isa<llvm::CatchPadInst> (instruction)
                || llvm::isa<llvm::CleanupPadInst> (instruction))
            {
                addDiagnostic (diagnostics, "exceptions/EH control flow in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            if (llvm::isa<llvm::FenceInst> (instruction)
                || llvm::isa<llvm::AtomicCmpXchgInst> (instruction)
                || llvm::isa<llvm::AtomicRMWInst> (instruction))
            {
                addDiagnostic (diagnostics, "atomic/synchronization instruction in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            if (const auto* load = llvm::dyn_cast<llvm::LoadInst> (&instruction);
                load != nullptr && (load->isAtomic() || load->isVolatile()))
            {
                addDiagnostic (diagnostics, "atomic/volatile load in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            if (const auto* store = llvm::dyn_cast<llvm::StoreInst> (&instruction);
                store != nullptr && (store->isAtomic() || store->isVolatile()))
            {
                addDiagnostic (diagnostics, "atomic/volatile store in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            const auto* call = llvm::dyn_cast<llvm::CallBase> (&instruction);
            if (call == nullptr)
                continue;

            if (llvm::isa<llvm::InlineAsm> (call->getCalledOperand()))
            {
                addDiagnostic (diagnostics, "inline assembly in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            const auto* calledFunction = call->getCalledFunction();
            if (calledFunction == nullptr)
            {
                addDiagnostic (diagnostics, "indirect/function-pointer call in '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
                continue;
            }

            if (calledFunction->isDeclaration()
                && ! calledFunction->isIntrinsic()
                && ! isAllowedExternalFunction (calledFunction->getName()))
            {
                addDiagnostic (diagnostics, "call to external function '" + functionName (*calledFunction)
                                          + "' from '" + functionName (function)
                                          + "' is not allowed");
                ok = false;
            }
        }
    }

    return ok;
}

bool validateNoRecursion (const llvm::Module& module, std::ostringstream& diagnostics)
{
    std::unordered_map<const llvm::Function*, std::vector<const llvm::Function*>> graph;

    for (const auto& function : module.functions())
    {
        if (! function.isDeclaration())
            graph.emplace (&function, std::vector<const llvm::Function*> {});
    }

    for (const auto& [function, _] : graph)
    {
        for (const auto& instruction : llvm::instructions (function))
        {
            const auto* call = llvm::dyn_cast<llvm::CallBase> (&instruction);
            if (call == nullptr)
                continue;

            const auto* called = call->getCalledFunction();
            if (called != nullptr && graph.find (called) != graph.end())
                graph[function].push_back (called);
        }
    }

    enum class VisitState
    {
        unvisited,
        visiting,
        visited
    };

    std::unordered_map<const llvm::Function*, VisitState> states;
    for (const auto& [function, _] : graph)
        states.emplace (function, VisitState::unvisited);

    std::vector<const llvm::Function*> stack;
    auto ok = true;

    std::function<void (const llvm::Function*)> visit = [&] (const llvm::Function* function)
    {
        if (! ok)
            return;

        states[function] = VisitState::visiting;
        stack.push_back (function);

        for (const auto* callee : graph[function])
        {
            const auto state = states[callee];

            if (state == VisitState::visiting)
            {
                addDiagnostic (diagnostics, "recursion is not allowed; cycle reaches '" + functionName (*callee) + "'");
                ok = false;
                return;
            }

            if (state == VisitState::unvisited)
                visit (callee);

            if (! ok)
                return;
        }

        stack.pop_back();
        states[function] = VisitState::visited;
    };

    for (const auto& [function, state] : states)
        if (state == VisitState::unvisited)
            visit (function);

    return ok;
}
} // namespace

namespace waviate::safety
{
ModuleSafetyValidationResult validateModuleSafety (const llvm::Module& module)
{
    std::ostringstream diagnostics;
    auto ok = true;

    ok &= validateGlobals (module, diagnostics);
    ok &= validateExternalFunctions (module, diagnostics);
    ok &= validateInstructions (module, diagnostics);
    ok &= validateAlloca (module, diagnostics);
    ok &= validateNoRecursion (module, diagnostics);

    ModuleSafetyValidationResult result;
    result.succeeded = ok;
    result.diagnostics = diagnostics.str();
    return result;
}
} // namespace waviate::safety
