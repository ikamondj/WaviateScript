#include "WaviateSafety.h"

#include <algorithm>
#include <mutex>

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/DynamicLibrary.h>

namespace
{
thread_local waviate::safety::FuelState* currentFuelState = nullptr;

struct FuelProfile final
{
    waviate::safety::FuelLimitPreset preset;
    const char* id;
    const char* name;
    uint64_t baseBudget;
    uint64_t perShaderCallBudget;
};

constexpr FuelProfile fuelProfiles[] =
{
    { waviate::safety::FuelLimitPreset::minimal, "minimal", "Minimal", 8'000, 1'000 },
    { waviate::safety::FuelLimitPreset::low,     "low",     "Low",     32'000, 4'000 },
    { waviate::safety::FuelLimitPreset::medium,  "medium",  "Medium",  128'000, 16'000 },
    { waviate::safety::FuelLimitPreset::high,    "high",    "High",    512'000, 64'000 },
    { waviate::safety::FuelLimitPreset::massive, "massive", "Massive", 2'000'000, 256'000 }
};

const FuelProfile& profileFor (waviate::safety::FuelLimitPreset preset) noexcept
{
    for (const auto& profile : fuelProfiles)
        if (profile.preset == preset)
            return profile;

    return fuelProfiles[2];
}
} // namespace

namespace waviate::safety
{
ScopedFuelBudget::ScopedFuelBudget (FuelState& state) noexcept
{
    previous = currentFuelState;
    currentFuelState = &state;
}

ScopedFuelBudget::~ScopedFuelBudget() noexcept
{
    currentFuelState = previous;
}

std::array<FuelLimitPreset, 5> getFuelLimitPresets() noexcept
{
    return {
        FuelLimitPreset::minimal,
        FuelLimitPreset::low,
        FuelLimitPreset::medium,
        FuelLimitPreset::high,
        FuelLimitPreset::massive
    };
}

const char* getFuelLimitPresetId (FuelLimitPreset preset) noexcept
{
    return profileFor (preset).id;
}

juce::String getFuelLimitPresetName (FuelLimitPreset preset)
{
    return profileFor (preset).name;
}

FuelLimitPreset fuelLimitPresetFromId (juce::StringRef id) noexcept
{
    for (const auto& profile : fuelProfiles)
        if (id == juce::StringRef(profile.id))
            return profile.preset;

    return FuelLimitPreset::medium;
}

uint64_t calculateFuelBudget (FuelLimitPreset preset, int shaderCallsForSample) noexcept
{
    const auto& profile = profileFor (preset);
    const auto safeShaderCallsForSample = static_cast<uint64_t> (std::max (1, shaderCallsForSample));

    return profile.baseBudget + (safeShaderCallsForSample * profile.perShaderCallBudget);
}

void registerRuntimeSymbols()
{
    static std::once_flag once;
    std::call_once (once, [] {
        llvm::sys::DynamicLibrary::AddSymbol ("waviate_consume_fuel",
                                              reinterpret_cast<void*> (&waviate_consume_fuel));
        llvm::sys::DynamicLibrary::AddSymbol ("waviate_fuel_trap",
                                              reinterpret_cast<void*> (&waviate_fuel_trap));
    });
}

bool isCurrentThreadFuelExhausted() noexcept
{
    return currentFuelState != nullptr && currentFuelState->exhausted;
}
} // namespace waviate::safety

extern "C" uint8_t waviate_consume_fuel (uint64_t amount) noexcept
{
    auto* state = currentFuelState;

    if (state == nullptr)
        return 1;

    if (state->exhausted)
        return 0;

    if (amount == 0)
        return 1;

    if (state->remaining < amount)
    {
        state->remaining = 0;
        state->exhausted = true;
        return 0;
    }

    state->remaining -= amount;
    return 1;
}

extern "C" void waviate_fuel_trap() noexcept
{
    if (currentFuelState != nullptr)
        currentFuelState->exhausted = true;
}
