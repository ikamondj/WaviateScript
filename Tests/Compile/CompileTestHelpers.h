#pragma once

#include "../../Source/CompilePipeline.h"

#include <juce_core/juce_core.h>

#include <filesystem>
#include <memory>

namespace waviate::tests::compile
{
struct SampleInvocation
{
    float incomingSample = 0.0f;
    float currentSample = 0.0f;
    int channel = 0;
    int sampleInBlock = 0;
    int blockSize = 1;
    int inputChannelCount = 1;
    int outputChannelCount = 1;
    float sampleRate = 48000.0f;
    juce::uint64 samplesSinceAppStart = 0;
};

struct CompileResult
{
    std::shared_ptr<waviate::compile::Pipeline> pipeline;
    juce::String shortName;
    juce::String extension = ".wlsl";
    juce::String errorMessage;
    SampleShader sampleShader = nullptr;
    FrequencyShader frequencyShader = nullptr;

    [[nodiscard]] bool succeeded() const noexcept { return errorMessage.isEmpty(); }
    [[nodiscard]] bool hasEntryPoints() const noexcept { return sampleShader != nullptr || frequencyShader != nullptr; }
    explicit operator bool() const noexcept { return succeeded() && hasEntryPoints(); }
};

std::filesystem::path compileFixtureRoot();
CompileResult compileSource(const juce::String& shortName, const juce::String& source, const juce::String& extension = ".wlsl");
CompileResult compileFile(const std::filesystem::path& path);
CompileResult compileFixture(const juce::String& relativePath);
float invokeSample(const CompileResult& result, const SampleInvocation& invocation = {});
void expectCompileSuccess(juce::UnitTest& test, const CompileResult& result);
void expectCompileFailure(juce::UnitTest& test, const CompileResult& result, const juce::String& messageSubstring = {});
} // namespace waviate::tests::compile
