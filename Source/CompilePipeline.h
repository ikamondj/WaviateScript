#pragma once

#include "AbstractCompiler.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace waviate::compile
{
struct CompileOutput
{
    SampleShader sampleShader = nullptr;
    FrequencyShader frequencyShader = nullptr;

    [[nodiscard]] bool hasEntryPoints() const noexcept
    {
        return sampleShader != nullptr || frequencyShader != nullptr;
    }
};

using CompilerMap = std::unordered_map<std::string, std::unique_ptr<AbstractCompiler>>;

CompilerMap createDefaultCompilers();

class Pipeline final
{
public:
    Pipeline();
    explicit Pipeline(CompilerMap compilers);

    [[nodiscard]] bool supportsExtension(std::string_view extension) const;

    CompileOutput compile(std::string extension, const std::string& source);
    CompileOutput compileFile(const std::filesystem::path& path);

private:
    CompilerMap compilers_;
};
} // namespace waviate::compile
