#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

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

class EphemeralArena final
{
public:
    static constexpr size_t defaultCapacityBytes = 128ull * 1024ull * 1024ull;

    explicit EphemeralArena (size_t capacityBytes = defaultCapacityBytes);

    bool isInitialized() const noexcept { return storage != nullptr; }
    size_t getCapacityBytes() const noexcept { return capacity; }
    size_t getUsedBytes() const noexcept { return offset; }
    uint64_t getGeneration() const noexcept { return generation; }
    bool wasExhausted() const noexcept { return exhausted; }

    void resetForPass() noexcept;
    void* allocate (uint64_t sizeBytes, uint64_t alignmentBytes) noexcept;

private:
    std::unique_ptr<std::byte[]> storage;
    size_t capacity = 0;
    size_t offset = 0;
    uint64_t generation = 0;
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

class ScopedArenaPass final
{
public:
    explicit ScopedArenaPass (EphemeralArena& arena) noexcept;
    ~ScopedArenaPass() noexcept;

private:
    EphemeralArena* previous = nullptr;
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
extern "C" void* __waviate_internal_arena_allocate (uint64_t sizeBytes, uint64_t alignmentBytes) noexcept;
extern "C" uint64_t __waviate_internal_arena_generation() noexcept;
