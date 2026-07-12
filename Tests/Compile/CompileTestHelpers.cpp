#include "CompileTestHelpers.h"
#include "WaviateSafety.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace waviate::tests::compile
{
namespace
{
size_t maxChannelLength(const std::vector<std::vector<float>>& channels)
{
    size_t blockSize = 0;

    for (const auto& channel : channels)
        blockSize = std::max(blockSize, channel.size());

    return blockSize;
}

std::vector<std::vector<float>> buildChannels(std::vector<std::vector<float>> channels,
                                              const int channelCount,
                                              const int blockSize,
                                              const float defaultValue)
{
    if (channels.empty())
        channels.resize(static_cast<size_t>(channelCount), std::vector<float>(static_cast<size_t>(blockSize), defaultValue));

    if (channels.size() < static_cast<size_t>(channelCount))
        channels.resize(static_cast<size_t>(channelCount), std::vector<float>(static_cast<size_t>(blockSize), defaultValue));

    for (auto& channel : channels)
    {
        if (channel.size() < static_cast<size_t>(blockSize))
            channel.resize(static_cast<size_t>(blockSize), defaultValue);
    }

    return channels;
}
} // namespace

std::filesystem::path compileFixtureRoot()
{
    return std::filesystem::path { WAVIATESCRIPT_TEST_SOURCE_DIR } / "Tests" / "Compile" / "Fixtures";
}

CompileResult compileSource(const juce::String& shortName, const juce::String& source, const juce::String& extension)
{
    std::printf("[DEBUG] compileSource helper start: %s\n", shortName.toRawUTF8());
    std::fflush(stdout);
    CompileResult result;
    result.pipeline = std::make_shared<waviate::compile::Pipeline>();
    result.shortName = shortName;
    result.extension = extension;

    try
    {
        const auto compiled = result.pipeline->compile(extension.toStdString(), source.toStdString());
        result.sampleShader = compiled.sampleShader;
        result.frequencyShader = compiled.frequencyShader;
        result.runtime = compiled.runtime;
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
        result.runtime = compiled.runtime;
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

SampleExecutionResult executeSample(const CompileResult& result, const SampleInvocation& invocation)
{
    if (result.sampleShader == nullptr)
        throw std::runtime_error("Sample shader entry point was not emitted");

    const auto derivedBlockSize = std::max({ 1,
        invocation.blockSize,
        static_cast<int>(maxChannelLength(invocation.inputChannels)),
        static_cast<int>(maxChannelLength(invocation.outputChannels)),
        static_cast<int>(maxChannelLength(invocation.sideChainChannels)) });
    const auto blockSize = derivedBlockSize;
    const auto sampleIndex = std::clamp(invocation.sampleInBlock, 0, blockSize - 1);
    const auto inputChannels = std::max(1, invocation.inputChannels.empty() ? invocation.inputChannelCount
                                                                            : static_cast<int>(invocation.inputChannels.size()));
    const auto outputChannels = std::max(1, invocation.outputChannels.empty() ? invocation.outputChannelCount
                                                                              : static_cast<int>(invocation.outputChannels.size()));
    const auto sideChainChannels = std::max(0, static_cast<int>(invocation.sideChainChannels.size()));
    const auto channelIndex = std::clamp(invocation.channel, 0, outputChannels - 1);

    auto inputStorage = buildChannels(invocation.inputChannels, inputChannels, blockSize, invocation.incomingSample);
    auto outputStorage = buildChannels(invocation.outputChannels, outputChannels, blockSize, invocation.currentSample);
    auto sideChainStorage = buildChannels(invocation.sideChainChannels, sideChainChannels, blockSize, 0.0f);
    std::vector<const float*> inputPointers;
    std::vector<const float*> sideChainPointers;
    std::vector<float*> outputPointers;
    inputPointers.reserve(static_cast<size_t>(inputChannels));
    sideChainPointers.reserve(static_cast<size_t>(sideChainChannels));
    outputPointers.reserve(static_cast<size_t>(outputChannels));

    for (auto& channel : inputStorage)
        inputPointers.push_back(channel.data());

    for (auto& channel : sideChainStorage)
        sideChainPointers.push_back(channel.data());

    for (auto& channel : outputStorage)
        outputPointers.push_back(channel.data());

    auto midiNoteOn = invocation.midiNoteOn;
    auto midiCcValue = invocation.midiCcValue;
    auto sampleWhenMidiNoteOn = invocation.sampleWhenMidiNoteOn;
    auto sampleWhenMidiNoteOff = invocation.sampleWhenMidiNoteOff;
    std::array<juce::uint64, 128> sampleWhenCcValueChanged {};
    std::array<bool, 128> sustainDeferred {};

    WaviateSampleInput input {};
    input.samplesSinceAppStart = invocation.samplesSinceAppStart;
    input.sampleInBlock = sampleIndex;
    input.blockSize = blockSize;
    input.inputChannelCount = inputChannels;
    input.sideChainChannelCount = sideChainChannels;
    input.sampleMemoryCount = 0;
    input.channelCount = outputChannels;
    input.channel = static_cast<uint8_t>(channelIndex);
    input.midiNoteOn = midiNoteOn.data();
    input.midiCCValue = midiCcValue.data();
    input.sampleWhenMidiNoteOn = sampleWhenMidiNoteOn.data();
    input.sampleWhenMidiNoteOff = sampleWhenMidiNoteOff.data();
    input.sampleWhenCCValueChanged = sampleWhenCcValueChanged.data();
    input.midiNotePressOrder = invocation.midiNotePressOrder.data();
    input.midiNoteReleaseOrder = invocation.midiNoteReleaseOrder.data();
    input.midiVoiceOrder = invocation.midiVoiceOrder.data();
    input.midiNotePressCount = invocation.midiNotePressCount;
    input.midiNoteReleaseCount = invocation.midiNoteReleaseCount;
    input.midiVoiceCount = invocation.midiVoiceCount;
    input.sustain = invocation.sustain;
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
    input.inputSideChainSamples = sideChainPointers.empty() ? nullptr : sideChainPointers.data();
    input.currentSampleData = outputPointers.data();

    SampleExecutionResult executionResult;
    waviate::audio::ScopedAudioCacheBinding audioCacheBinding(invocation.audioCache);
    static thread_local waviate::safety::EphemeralArena arena;
    {
        waviate::safety::ScopedArenaPass arenaPass(arena);
        executionResult.returnValue = result.sampleShader(&input, nullptr);
    }

    if (result.runtime.isFuelExhausted())
    {
        executionResult.returnValue = 0.0f;
        outputStorage[static_cast<size_t>(channelIndex)][static_cast<size_t>(sampleIndex)] = 0.0f;
    }

    executionResult.selectedOutputSample = outputStorage[static_cast<size_t>(channelIndex)][static_cast<size_t>(sampleIndex)];
    executionResult.outputChannels = std::move(outputStorage);
    return executionResult;
}

float invokeSample(const CompileResult& result, const SampleInvocation& invocation)
{
    const auto executionResult = executeSample(result, invocation);
    return executionResult.selectedOutputSample != invocation.currentSample
        ? executionResult.selectedOutputSample
        : executionResult.returnValue;
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
