#pragma once

#include "AbstractCompiler.h"

#include <filesystem>
#include <memory>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace waviate::compile
{
enum class FuelLimitPreset
{
    Minimal = 0,
    Low,
    Medium,
    High,
    Massive
};

struct CompileOutput
{
    SampleShader sampleShader = nullptr;
    FrequencyShader frequencyShader = nullptr;
    ShaderRuntimeControls runtime;

    [[nodiscard]] bool hasEntryPoints() const noexcept
    {
        return sampleShader != nullptr || frequencyShader != nullptr;
    }
};

using CompilerMap = std::unordered_map<std::string, std::unique_ptr<AbstractCompiler>>;

CompilerMap createDefaultCompilers();
[[nodiscard]] std::string_view fuelLimitPresetId(FuelLimitPreset preset) noexcept;
[[nodiscard]] std::string_view fuelLimitPresetDisplayName(FuelLimitPreset preset) noexcept;
[[nodiscard]] FuelLimitPreset fuelLimitPresetFromId(std::string_view id) noexcept;
[[nodiscard]] uint64_t calculateFuelBudget(FuelLimitPreset preset, int blockSize, int channelCount) noexcept;

class Pipeline final
{
public:
    Pipeline();
    explicit Pipeline(CompilerMap compilers);

    [[nodiscard]] bool supportsExtension(std::string_view extension) const;

    CompileOutput compile(std::string extension, const std::string& source);
    CompileOutput compileFile(const std::filesystem::path& path);
    void unloadActiveScripts() noexcept;

private:
    CompilerMap compilers_;
};
} // namespace waviate::compile
