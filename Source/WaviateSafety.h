#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>

namespace waviate::safety
{
enum class FuelLimitPreset
{
    minimal = 0,
    low,
    medium,
    high,
    massive
};

struct FuelState final
{
    uint64_t remaining = 0;
    bool exhausted = false;
};

class ScopedFuelBudget final
{
public:
    explicit ScopedFuelBudget (FuelState& state) noexcept;
    ~ScopedFuelBudget() noexcept;

private:
    FuelState* previous = nullptr;
};

std::array<FuelLimitPreset, 5> getFuelLimitPresets() noexcept;
const char* getFuelLimitPresetId (FuelLimitPreset preset) noexcept;
juce::String getFuelLimitPresetName (FuelLimitPreset preset);
FuelLimitPreset fuelLimitPresetFromId (juce::StringRef id) noexcept;
uint64_t calculateFuelBudget (FuelLimitPreset preset, int shaderCallsForSample) noexcept;

void registerRuntimeSymbols();
bool isCurrentThreadFuelExhausted() noexcept;
} // namespace waviate::safety

extern "C" uint8_t waviate_consume_fuel (uint64_t amount) noexcept;
extern "C" void waviate_fuel_trap() noexcept;
