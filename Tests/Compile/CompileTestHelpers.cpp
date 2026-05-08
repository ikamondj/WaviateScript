#include "CompileTestHelpers.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace waviate::tests::compile
{
std::filesystem::path compileFixtureRoot()
{
    return std::filesystem::path { WAVIATESCRIPT_TEST_SOURCE_DIR } / "Tests" / "Compile" / "Fixtures";
}

CompileResult compileSource(const juce::String& shortName, const juce::String& source, const juce::String& extension)
{
    CompileResult result;
    result.pipeline = std::make_shared<waviate::compile::Pipeline>();
    result.shortName = shortName;
    result.extension = extension;

    try
    {
        const auto compiled = result.pipeline->compile(extension.toStdString(), source.toStdString());
        result.sampleShader = compiled.sampleShader;
        result.frequencyShader = compiled.frequencyShader;
    }
    catch (const std::exception& e)
    {
        result.errorMessage = e.what();
    }

    return result;
}

CompileResult compileFile(const std::filesystem::path& path)
{
    CompileResult result;
    result.pipeline = std::make_shared<waviate::compile::Pipeline>();
    result.shortName = juce::String(path.filename().string());
    result.extension = juce::String(path.extension().string());

    try
    {
        const auto compiled = result.pipeline->compileFile(path);
        result.sampleShader = compiled.sampleShader;
        result.frequencyShader = compiled.frequencyShader;
    }
    catch (const std::exception& e)
    {
        result.errorMessage = e.what();
    }

    return result;
}

CompileResult compileFixture(const juce::String& relativePath)
{
    return compileFile(compileFixtureRoot() / relativePath.toStdString());
}

float invokeSample(const CompileResult& result, const SampleInvocation& invocation)
{
    if (result.sampleShader == nullptr)
        throw std::runtime_error("Sample shader entry point was not emitted");

    const auto blockSize = std::max(1, invocation.blockSize);
    const auto sampleIndex = std::clamp(invocation.sampleInBlock, 0, blockSize - 1);
    const auto inputChannels = std::max(1, invocation.inputChannelCount);
    const auto outputChannels = std::max(1, invocation.outputChannelCount);
    const auto channelIndex = std::clamp(invocation.channel, 0, outputChannels - 1);

    std::vector<std::vector<float>> inputStorage(static_cast<size_t>(inputChannels), std::vector<float>(static_cast<size_t>(blockSize), invocation.incomingSample));
    std::vector<std::vector<float>> outputStorage(static_cast<size_t>(outputChannels), std::vector<float>(static_cast<size_t>(blockSize), invocation.currentSample));
    std::vector<const float*> inputPointers;
    std::vector<float*> outputPointers;
    inputPointers.reserve(static_cast<size_t>(inputChannels));
    outputPointers.reserve(static_cast<size_t>(outputChannels));

    for (auto& channel : inputStorage)
        inputPointers.push_back(channel.data());

    for (auto& channel : outputStorage)
        outputPointers.push_back(channel.data());

    std::array<uint8_t, 128> midiNoteOn {};
    std::array<uint8_t, 128> midiCcValue {};
    std::array<juce::uint64, 128> sampleWhenMidiNoteOn {};
    std::array<juce::uint64, 128> sampleWhenMidiNoteOff {};
    std::array<juce::uint64, 128> sampleWhenCcValueChanged {};
    std::array<bool, 128> sustainDeferred {};

    WaviateSampleInput input {};
    input.samplesSinceAppStart = invocation.samplesSinceAppStart;
    input.sampleInBlock = sampleIndex;
    input.blockSize = blockSize;
    input.inputChannelCount = inputChannels;
    input.sideChainChannelCount = 0;
    input.sampleMemoryCount = 0;
    input.channelCount = outputChannels;
    input.channel = static_cast<uint8_t>(channelIndex);
    input.midiNoteOn = midiNoteOn.data();
    input.midiCCValue = midiCcValue.data();
    input.sampleWhenMidiNoteOn = sampleWhenMidiNoteOn.data();
    input.sampleWhenMidiNoteOff = sampleWhenMidiNoteOff.data();
    input.sampleWhenCCValueChanged = sampleWhenCcValueChanged.data();
    input.sustain = false;
    input.sustainDefer = sustainDeferred.data();
    input.controllerCount = 0;
    input.controllerButtonMask = nullptr;
    input.sampleWhenControllerButtonChanged = nullptr;
    input.controllerButtonCount = 0;
    input.controllerAxisValue = nullptr;
    input.sampleWhenControllerAxisChanged = nullptr;
    input.controllerAxisCount = 0;
    input.sampleRate = invocation.sampleRate;
    input.previousSamples = nullptr;
    input.inputDeviceSamples = inputPointers.data();
    input.inputSideChainSamples = nullptr;
    input.currentSampleData = outputPointers.data();

    const auto value = result.sampleShader(&input, nullptr);
    return outputStorage[static_cast<size_t>(channelIndex)][static_cast<size_t>(sampleIndex)] != invocation.currentSample
        ? outputStorage[static_cast<size_t>(channelIndex)][static_cast<size_t>(sampleIndex)]
        : value;
}

void expectCompileSuccess(juce::UnitTest& test, const CompileResult& result)
{
    test.expect(result.succeeded(), "Compilation failed: " + result.errorMessage);

    if (result.succeeded())
        test.expect(result.hasEntryPoints(), "Compilation succeeded but emitted no sample/frequency entry points");
}

void expectCompileFailure(juce::UnitTest& test, const CompileResult& result, const juce::String& messageSubstring)
{
    test.expect(!result.succeeded() || !result.hasEntryPoints(), "Compilation unexpectedly succeeded");

    if (messageSubstring.isNotEmpty() && !result.errorMessage.isEmpty())
        test.expect(result.errorMessage.containsIgnoreCase(messageSubstring), "Unexpected error: " + result.errorMessage);
}
} // namespace waviate::tests::compile
