#pragma once

#include "../../Source/CompilePipeline.h"

#include <juce_core/juce_core.h>

#include <array>
#include <filesystem>
#include <memory>
#include <vector>

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
    bool sustain = false;
    std::array<uint8_t, 128> midiNoteOn {};
    std::array<uint8_t, 128> midiCcValue {};
    std::vector<std::vector<float>> inputChannels;
    std::vector<std::vector<float>> outputChannels;
    std::vector<std::vector<float>> sideChainChannels;
};

struct SampleExecutionResult
{
    float returnValue = 0.0f;
    float selectedOutputSample = 0.0f;
    std::vector<std::vector<float>> outputChannels;
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
SampleExecutionResult executeSample(const CompileResult& result, const SampleInvocation& invocation = {});
float invokeSample(const CompileResult& result, const SampleInvocation& invocation = {});
void expectCompileSuccess(juce::UnitTest& test, const CompileResult& result);
void expectCompileFailure(juce::UnitTest& test, const CompileResult& result, const juce::String& messageSubstring = {});
} // namespace waviate::tests::compile
