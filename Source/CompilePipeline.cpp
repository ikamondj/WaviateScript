#include "CompilePipeline.h"

#include "ClangExternalCompiler.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace waviate::compile
{
namespace
{
struct FuelLimitPresetInfo
{
    FuelLimitPreset preset;
    std::string_view id;
    std::string_view displayName;
    uint64_t fuelPerSampleChannel;
};

constexpr std::array<FuelLimitPresetInfo, 5> fuelLimitPresetInfos {{
    { FuelLimitPreset::Minimal, "minimal", "Minimal", 64 },
    { FuelLimitPreset::Low, "low", "Low", 256 },
    { FuelLimitPreset::Medium, "medium", "Medium", 1024 },
    { FuelLimitPreset::High, "high", "High", 4096 },
    { FuelLimitPreset::Massive, "massive", "Massive", 16384 },
}};

std::string normalizeExtension(std::string extension)
{
    if (extension.empty())
        throw std::invalid_argument("A shader extension is required");

    if (extension.front() != '.')
        extension.insert(extension.begin(), '.');

    std::transform(extension.begin(), extension.end(), extension.begin(), [] (const unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return extension;
}

std::string readTextFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (! input)
        throw std::runtime_error("Could not open shader file: " + path.string());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

const FuelLimitPresetInfo& findFuelLimitPresetInfo(FuelLimitPreset preset) noexcept
{
    for (const auto& info : fuelLimitPresetInfos)
        if (info.preset == preset)
            return info;

    return fuelLimitPresetInfos[2];
}

bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < lhs.size(); ++i)
    {
        const auto a = static_cast<unsigned char>(lhs[i]);
        const auto b = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(a) != std::tolower(b))
            return false;
    }

    return true;
}

uint64_t saturatedMultiply(uint64_t lhs, uint64_t rhs) noexcept
{
    if (lhs == 0 || rhs == 0)
        return 0;

    const auto maxValue = std::numeric_limits<uint64_t>::max();
    if (lhs > maxValue / rhs)
        return maxValue;

    return lhs * rhs;
}

uint64_t saturatedAdd(uint64_t lhs, uint64_t rhs) noexcept
{
    const auto maxValue = std::numeric_limits<uint64_t>::max();
    if (lhs > maxValue - rhs)
        return maxValue;

    return lhs + rhs;
}
} // namespace

CompilerMap createDefaultCompilers()
{
    CompilerMap compilers;
    compilers.emplace(".wlsl", std::make_unique<ClangCompiler<true>>());
    return compilers;
}

std::string_view fuelLimitPresetId(FuelLimitPreset preset) noexcept
{
    return findFuelLimitPresetInfo(preset).id;
}

std::string_view fuelLimitPresetDisplayName(FuelLimitPreset preset) noexcept
{
    return findFuelLimitPresetInfo(preset).displayName;
}

FuelLimitPreset fuelLimitPresetFromId(std::string_view id) noexcept
{
    for (const auto& info : fuelLimitPresetInfos)
        if (equalsIgnoreCase(id, info.id))
            return info.preset;

    return FuelLimitPreset::Medium;
}

uint64_t calculateFuelBudget(FuelLimitPreset preset, int blockSize, int channelCount) noexcept
{
    constexpr uint64_t fixedBlockReserve = 4096;
    const auto samples = static_cast<uint64_t>(std::max(1, blockSize));
    const auto channels = static_cast<uint64_t>(std::max(1, channelCount));
    const auto scaledBlockSize = saturatedMultiply(samples, channels);
    const auto scaledFuel = saturatedMultiply(scaledBlockSize, findFuelLimitPresetInfo(preset).fuelPerSampleChannel);
    return saturatedAdd(fixedBlockReserve, scaledFuel);
}

Pipeline::Pipeline()
    : Pipeline(createDefaultCompilers())
{
}

Pipeline::Pipeline(CompilerMap compilers)
    : compilers_(std::move(compilers))
{
}

bool Pipeline::supportsExtension(std::string_view extension) const
{
    return compilers_.contains(normalizeExtension(std::string(extension)));
}

CompileOutput Pipeline::compile(std::string extension, const std::string& source)
{
    const auto normalizedExtension = normalizeExtension(std::move(extension));
    const auto it = compilers_.find(normalizedExtension);

    if (it == compilers_.end())
        throw std::runtime_error("No compiler registered for " + normalizedExtension);

    CompileOutput result;
    it->second->compileSource(source, result.sampleShader, result.frequencyShader);
    result.runtime = it->second->getRuntimeControls();
    return result;
}

CompileOutput Pipeline::compileFile(const std::filesystem::path& path)
{
    return compile(path.extension().string(), readTextFile(path));
}

void Pipeline::unloadActiveScripts() noexcept
{
    for (auto& [extension, compiler] : compilers_)
    {
        static_cast<void>(extension);
        if (compiler != nullptr)
            compiler->unloadActiveScript();
    }
}
} // namespace waviate::compile
