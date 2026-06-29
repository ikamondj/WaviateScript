#pragma once

#include <string_view>

namespace waviate::runtime
{
bool isAuditedExternalFunction (std::string_view name) noexcept;
void registerAuditedRuntimeSymbols();
} // namespace waviate::runtime
