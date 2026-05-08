#include "CompilePipeline.h"

#include "ClangExternalCompiler.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace waviate::compile
{
namespace
{
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
} // namespace

CompilerMap createDefaultCompilers()
{
    CompilerMap compilers;
    compilers.emplace(".wlsl", std::make_unique<ClangCompiler<true>>());
    return compilers;
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
    return result;
}

CompileOutput Pipeline::compileFile(const std::filesystem::path& path)
{
    return compile(path.extension().string(), readTextFile(path));
}
} // namespace waviate::compile
