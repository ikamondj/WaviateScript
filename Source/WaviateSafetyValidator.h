#pragma once

#include <string>

namespace llvm
{
class Module;
}

namespace waviate::safety
{
struct ModuleSafetyValidationResult final
{
    bool succeeded = false;
    std::string diagnostics;

    explicit operator bool() const noexcept { return succeeded; }
};

ModuleSafetyValidationResult validateModuleSafety (const llvm::Module& module);
} // namespace waviate::safety
