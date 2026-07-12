// Included by PluginProcessor.cpp. These small functions keep processBlock's hot path
// visible to the optimiser while separating block setup, sample DSP, and frequency DSP.

#include <juce_dsp/juce_dsp.h>

struct WaviateScriptAudioProcessor::AudioBlockContext
{
    juce::AudioBuffer<float>& buffer;
    juce::AudioBuffer<float> mainIn;
    juce::AudioBuffer<float> mainOut;
    juce::AudioBuffer<float> sideIn;
    int sampleCount = 0;
    int inputChannels = 0;
    int outputChannels = 0;
    bool hasSidechain = false;
    FrequencyDomainSettings frequencySettings;
};

struct WaviateScriptAudioProcessor::FrequencyWorkspace
{
    static constexpr int minimumOrder = 8;
    static constexpr int maximumOrder = 13;
    static constexpr int maximumSize = 1 << maximumOrder;

    std::array<std::unique_ptr<juce::dsp::FFT>, maximumOrder - minimumOrder + 1> plans;
    std::vector<std::vector<float>> inputTransform;
    std::vector<std::vector<float>> outputTransform;
    std::vector<std::vector<float>> sidechainTransform;
    std::vector<std::vector<WaviateComplex>> inputSpectrum;
    std::vector<std::vector<WaviateComplex>> outputSpectrum;
    std::vector<std::vector<WaviateComplex>> sidechainSpectrum;
    std::vector<const WaviateComplex*> inputPointers;
    std::vector<const WaviateComplex*> outputPointers;
    std::vector<const WaviateComplex*> sidechainPointers;
    std::vector<float> visualizerFrame;
    WaviateFrequencyInput input {};
};

inline void WaviateScriptAudioProcessor::setupCommonBlockData(AudioBlockContext& context) noexcept
{
    context.sampleCount = context.buffer.getNumSamples();
    context.mainIn = getBusBuffer(context.buffer, true, 0);
    context.mainOut = getBusBuffer(context.buffer, false, 0);
    context.inputChannels = context.mainIn.getNumChannels();
    context.outputChannels = context.mainOut.getNumChannels();

    wavInput->blockSize = context.sampleCount;
    wavInput->sampleRate = static_cast<float>(currentSampleRate);
    wavInput->samplesSinceAppStart = samplesSinceAppStart;
    wavInput->sampleInBlock = 0;
    wavInput->sustain = sustainDown;
    wavInput->previousSamples = nullptr;
    wavInput->sampleMemoryCount = 0;
    wavInput->inputDeviceSamples = context.mainIn.getArrayOfReadPointers();
    wavInput->currentSampleData = context.mainOut.getArrayOfWritePointers();
    wavInput->inputChannelCount = context.inputChannels;
    wavInput->channelCount = context.outputChannels;
}

inline void WaviateScriptAudioProcessor::setupNonPremiumCommonBlockData(AudioBlockContext&) noexcept {}
inline void WaviateScriptAudioProcessor::setupStandaloneInputData(AudioBlockContext& context) noexcept
{
#if JucePlugin_Build_Standalone
    setupNonPremiumStandaloneInputData(context);
   #ifdef WAV_SCRIPT_PREMIUM
    setupPremiumStandaloneInputData(context);
   #endif
#else
    juce::ignoreUnused(context);
#endif
}
inline void WaviateScriptAudioProcessor::setupNonPremiumStandaloneInputData(AudioBlockContext&) noexcept {}

#ifdef WAV_SCRIPT_PREMIUM
inline void WaviateScriptAudioProcessor::setupPremiumCommonBlockData(AudioBlockContext&) noexcept
{
    OSCInputEvent oscEvent;
    while (oscEventsQueue.popOne(oscEvent))
        oscInterface.receiveEventOnAudioThread(oscEvent);

    GameControllerEvent gamepadEvent;
    while (gamepadEventsQueue.popOne(gamepadEvent))
        gameControllerInterface.receiveEventOnAudioThread(gamepadEvent);
}
inline void WaviateScriptAudioProcessor::setupPremiumStandaloneInputData(AudioBlockContext&) noexcept {}
inline void WaviateScriptAudioProcessor::setupDawInputData(AudioBlockContext& context) noexcept
{
#if ! JucePlugin_Build_Standalone
    if (getBusCount(true) > 1)
        if (auto* bus = getBus(true, 1); bus != nullptr && bus->isEnabled())
        {
            context.sideIn = getBusBuffer(context.buffer, true, 1);
            context.hasSidechain = context.sideIn.getNumChannels() > 0;
        }
#else
    juce::ignoreUnused(context);
#endif
}
#endif

inline void WaviateScriptAudioProcessor::setupBlockData(AudioBlockContext& context) noexcept
{
    setupCommonBlockData(context);
    setupNonPremiumCommonBlockData(context);
#ifdef WAV_SCRIPT_PREMIUM
    setupPremiumCommonBlockData(context);
    setupDawInputData(context);
#endif
    setupStandaloneInputData(context);

    wavInput->inputSideChainSamples = context.hasSidechain ? context.sideIn.getArrayOfReadPointers() : nullptr;
    wavInput->sideChainChannelCount = context.hasSidechain ? context.sideIn.getNumChannels() : 0;

    for (int channel = context.inputChannels; channel < context.outputChannels; ++channel)
        context.mainOut.clear(channel, 0, context.sampleCount);
}

inline bool WaviateScriptAudioProcessor::processSamples(AudioBlockContext& context,
                                                         juce::MidiBuffer& midiMessages) noexcept
{
    for (const auto metadata : midiMessages)
        if (metadata.samplePosition >= 0 && metadata.samplePosition < context.sampleCount)
            midiBlockMessages[static_cast<size_t>(metadata.samplePosition)].push_back(metadata.getMessage());

    const auto shader = activeSampleShader.load(std::memory_order_acquire);
    if (shader == nullptr)
        return true;

    const auto runtime = loadRuntimeControls();
    if (runtime.hasFuelMetering())
        runtime.beginBlock(waviate::compile::calculateFuelBudget(getFuelLimitPreset(), context.sampleCount, context.outputChannels));

    for (int sample = 0; sample < context.sampleCount; ++sample)
    {
        const auto absoluteSample = samplesSinceAppStart + static_cast<uint64_t>(sample);
        auto& messages = midiBlockMessages[static_cast<size_t>(sample)];
        for (const auto& message : messages)
            applyMidiToState(message, wavInput->midiNoteOn, wavInput->midiCCValue,
                             wavInput->sampleWhenMidiNoteOn, wavInput->sampleWhenMidiNoteOff,
                             wavInput->sampleWhenCCValueChanged, sustainDown,
                             std::span<bool, midiStateCount>(wavInput->sustainDefer, midiStateCount), absoluteSample);

        wavInput->sampleInBlock = sample;
        wavInput->samplesSinceAppStart = absoluteSample;
        wavInput->sustain = sustainDown;

        for (int channel = 0; channel < context.outputChannels; ++channel)
        {
            wavInput->channel = static_cast<uint8_t>(channel);
            waviate::safety::ScopedArenaPass arenaPass(shaderArena);
            context.mainOut.getWritePointer(channel)[sample] = shader(wavInput.get(), nullptr);
            if (runtime.isFuelExhausted())
                return false;
        }
        messages.clear();
    }
    return true;
}

inline void WaviateScriptAudioProcessor::setupCommonFrequencyBlockData(AudioBlockContext& context) noexcept
{
    context.frequencySettings = getFrequencyDomainSettings();
    if (frequencyWorkspace == nullptr)
        return;
    auto& workspace = *frequencyWorkspace;
    workspace.input.sampleWidth = context.frequencySettings.fftSize;
    workspace.input.totalBinCount = context.frequencySettings.fftSize / 2 + 1;
    workspace.input.channelCount = context.outputChannels;
    workspace.input.sampleRate = static_cast<float>(currentSampleRate);
    workspace.input.samplesSinceAppStart = samplesSinceAppStart;
}
inline void WaviateScriptAudioProcessor::setupStandaloneFrequencyBlockData(AudioBlockContext&) noexcept {}
#ifdef WAV_SCRIPT_PREMIUM
inline void WaviateScriptAudioProcessor::setupDawFrequencyBlockData(AudioBlockContext&) noexcept {}
#endif
inline void WaviateScriptAudioProcessor::setupFrequencyStep(AudioBlockContext& context) noexcept
{
    setupCommonFrequencyBlockData(context);
    setupStandaloneFrequencyBlockData(context);
#ifdef WAV_SCRIPT_PREMIUM
    setupDawFrequencyBlockData(context);
#endif
}

inline bool WaviateScriptAudioProcessor::processFrequencyBins(AudioBlockContext& context) noexcept
{
    const auto shader = activeFrequencyShader.load(std::memory_order_acquire);
    if (shader == nullptr || frequencyWorkspace == nullptr || context.outputChannels <= 0)
        return true;

    auto& workspace = *frequencyWorkspace;
    const int fftSize = context.frequencySettings.fftSize;
    const int order = juce::roundToInt(std::log2(static_cast<double>(fftSize)));
    if (order < FrequencyWorkspace::minimumOrder || order > FrequencyWorkspace::maximumOrder)
        return true;
    auto& fft = *workspace.plans[static_cast<size_t>(order - FrequencyWorkspace::minimumOrder)];
    const int binCount = fftSize / 2 + 1;
    const int binsToProcess = context.frequencySettings.binLimit <= 0
        ? binCount : std::min(binCount, context.frequencySettings.binLimit);
    const auto runtime = loadRuntimeControls();
    if (runtime.hasFuelMetering())
        runtime.beginBlock(waviate::compile::calculateFuelBudget(getFuelLimitPreset(), binsToProcess, context.outputChannels));

    for (int frameStart = 0; frameStart < context.sampleCount; frameStart += fftSize)
    {
        const int frameSamples = std::min(fftSize, context.sampleCount - frameStart);
        for (int channel = 0; channel < context.outputChannels; ++channel)
        {
            auto& inputData = workspace.inputTransform[static_cast<size_t>(channel)];
            auto& outputData = workspace.outputTransform[static_cast<size_t>(channel)];
            std::fill(inputData.begin(), inputData.end(), 0.0f);
            std::fill(outputData.begin(), outputData.end(), 0.0f);
            for (int sample = 0; sample < frameSamples; ++sample)
            {
                const float window = context.frequencySettings.window == FftWindow::hann
                    ? 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * static_cast<float>(sample) / static_cast<float>(fftSize - 1))
                    : 1.0f;
                inputData[static_cast<size_t>(sample)] = (channel < context.inputChannels ? context.mainIn.getSample(channel, frameStart + sample) : 0.0f) * window;
                outputData[static_cast<size_t>(sample)] = context.mainOut.getSample(channel, frameStart + sample) * window;
            }
            fft.performRealOnlyForwardTransform(inputData.data());
            fft.performRealOnlyForwardTransform(outputData.data());
            for (int bin = 0; bin < binCount; ++bin)
            {
                workspace.inputSpectrum[static_cast<size_t>(channel)][static_cast<size_t>(bin)] = { inputData[static_cast<size_t>(2 * bin)], inputData[static_cast<size_t>(2 * bin + 1)] };
                workspace.outputSpectrum[static_cast<size_t>(channel)][static_cast<size_t>(bin)] = { outputData[static_cast<size_t>(2 * bin)], outputData[static_cast<size_t>(2 * bin + 1)] };
            }
            workspace.inputPointers[static_cast<size_t>(channel)] = workspace.inputSpectrum[static_cast<size_t>(channel)].data();
            workspace.outputPointers[static_cast<size_t>(channel)] = workspace.outputSpectrum[static_cast<size_t>(channel)].data();
        }

        workspace.input.inputDeviceData = workspace.inputPointers.data();
        workspace.input.currentFrequencyData = workspace.outputPointers.data();
        workspace.input.inputSideChainFrequencyData = nullptr;
        workspace.input.samplesSinceAppStart = samplesSinceAppStart + static_cast<uint64_t>(frameStart);

        for (int channel = 0; channel < context.outputChannels; ++channel)
            for (int bin = 0; bin < binsToProcess; ++bin)
            {
                workspace.input.channel = static_cast<uint8_t>(channel);
                workspace.input.bin = bin;
                waviate::safety::ScopedArenaPass arenaPass(shaderArena);
                workspace.outputSpectrum[static_cast<size_t>(channel)][static_cast<size_t>(bin)] = shader(&workspace.input, nullptr);
                if (runtime.isFuelExhausted())
                    return false;
            }

        for (int channel = 0; channel < context.outputChannels; ++channel)
        {
            auto& outputData = workspace.outputTransform[static_cast<size_t>(channel)];
            for (int bin = 0; bin < binCount; ++bin)
            {
                const auto value = workspace.outputSpectrum[static_cast<size_t>(channel)][static_cast<size_t>(bin)];
                outputData[static_cast<size_t>(2 * bin)] = value.real;
                outputData[static_cast<size_t>(2 * bin + 1)] = value.imag;
            }
            fft.performRealOnlyInverseTransform(outputData.data());
            for (int sample = 0; sample < frameSamples; ++sample)
                context.mainOut.setSample(channel, frameStart + sample, outputData[static_cast<size_t>(sample)]);
        }
    }
    return true;
}

inline void WaviateScriptAudioProcessor::pushVisualizerSamples(const AudioBlockContext& context)
{
    if (frequencyWorkspace == nullptr)
        return;
    auto& frame = frequencyWorkspace->visualizerFrame;
    for (int sample = 0; sample < context.sampleCount && context.outputChannels > 0; ++sample)
    {
        for (int channel = 0; channel < context.outputChannels; ++channel)
            frame[static_cast<size_t>(channel)] = context.mainOut.getSample(channel, sample);
        visualizer.pushSample(frame.data(), context.outputChannels);
    }
}
