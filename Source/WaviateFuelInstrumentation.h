#pragma once

namespace llvm
{
class Module;
}

namespace waviate::safety
{
bool instrumentModuleWithFuel (llvm::Module& module);
}
