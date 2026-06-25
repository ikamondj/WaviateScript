#include "waviate_marketplace_compiler.h"

#include "CompilePipeline.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <string>

namespace
{
void writeDiagnostics(char* destination, size_t destinationSize, const std::string& message) noexcept
{
    if (destination == nullptr || destinationSize == 0)
        return;

    const auto copySize = std::min(destinationSize - 1, message.size());
    std::memcpy(destination, message.data(), copySize);
    destination[copySize] = '\0';
}
} // namespace

const char* waviate_marketplace_compiler_version(void)
{
    return "waviate-marketplace-native-compiler/0.1";
}

int waviate_marketplace_compile_wlsl(
    const char* source,
    char* diagnostics,
    size_t diagnostics_size)
{
    try
    {
        if (source == nullptr || source[0] == '\0')
        {
            writeDiagnostics(diagnostics, diagnostics_size, "source code is required");
            return 2;
        }

        waviate::compile::Pipeline pipeline;
        const auto output = pipeline.compile(".wlsl", std::string(source));
        if (! output.hasEntryPoints())
        {
            writeDiagnostics(
                diagnostics,
                diagnostics_size,
                "source compiled but did not export SampleProcess, FrequencyProcess, sample_process, or frequency_process");
            return 3;
        }

        writeDiagnostics(diagnostics, diagnostics_size, "");
        return 0;
    }
    catch (const std::exception& err)
    {
        writeDiagnostics(diagnostics, diagnostics_size, err.what());
        return 1;
    }
    catch (...)
    {
        writeDiagnostics(diagnostics, diagnostics_size, "unknown native compiler failure");
        return 1;
    }
}
