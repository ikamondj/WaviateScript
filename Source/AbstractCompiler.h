#pragma once
#include "WaviateInput.h"

#include <cstdint>
#include <string>

using SampleShader = float (*) (const WaviateSampleInput*, WaviateSampleStateWriter*);
using FrequencyShader = WaviateComplex (*) (const WaviateFrequencyInput*, WaviateFrequencyStateWriter*);
using ShaderSetFuelBudgetFn = void (*) (uint64_t);
using ShaderGetFuelRemainingFn = uint64_t (*) ();
using ShaderGetFuelExhaustedFn = uint32_t (*) ();

struct ShaderRuntimeControls
{
    ShaderSetFuelBudgetFn setFuelBudget = nullptr;
    ShaderGetFuelRemainingFn getFuelRemaining = nullptr;
    ShaderGetFuelExhaustedFn getFuelExhausted = nullptr;

    [[nodiscard]] bool hasFuelMetering() const noexcept
    {
        return setFuelBudget != nullptr && getFuelExhausted != nullptr;
    }

    void beginBlock(uint64_t budget) const noexcept
    {
        if (setFuelBudget != nullptr)
            setFuelBudget(budget);
    }

    [[nodiscard]] uint64_t fuelRemaining() const noexcept
    {
        return getFuelRemaining != nullptr ? getFuelRemaining() : 0;
    }

    [[nodiscard]] bool isFuelExhausted() const noexcept
    {
        return getFuelExhausted != nullptr && getFuelExhausted() != 0;
    }
};

class AbstractCompiler {
public:
    virtual ~AbstractCompiler() = default;
    virtual void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) = 0;
    [[nodiscard]] virtual ShaderRuntimeControls getRuntimeControls() const noexcept { return {}; }
    virtual void unloadActiveScript() noexcept {}
};
