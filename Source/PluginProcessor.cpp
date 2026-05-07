/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Compilers.h"
//#include "fftw3.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <span>

const size_t maxBlockSize = 8192;
const size_t startingMidiPerSampleCount = 64;
const size_t midiInitialCapacity = 128;
constexpr size_t midiStateCount = 128;

//==============================================================================
WaviateScriptAudioProcessor::WaviateScriptAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : 
    AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
    , activeSampleShader(nullptr)
    , activeFrequencyShader(nullptr)
    , visualizer(2)
#ifdef WAV_SCRIPT_PREMIUM
    , gameControllerInterface(gamepadEventsQueue)
	, oscInterface(oscEventsQueue)
#endif 
#endif
{
    wavInput = std::make_unique<WaviateSampleInput>();
    *wavInput = {};

    wavInput->midiNoteOn = midiNoteOnState.data();
    wavInput->midiCCValue = midiCCValueState.data();
    wavInput->sampleWhenMidiNoteOn = sampleWhenMidiNoteOnState.data();
    wavInput->sampleWhenMidiNoteOff = sampleWhenMidiNoteOffState.data();
    wavInput->sampleWhenCCValueChanged = sampleWhenCCValueChangedState.data();
    wavInput->sustainDefer = sustainDeferredNoteOff.data();
    wavInput->sampleRate = static_cast<float>(currentSampleRate);

    compilers.insert({ ".wlsl", std::make_unique<ClangCompiler<true>>() });
    InitializeMidiMessageLookup(maxBlockSize);
}

void WaviateScriptAudioProcessor::InitializeMidiMessageLookup(size_t blockSize) {
    midiBlockMessages.clear();
    midiBlockMessages.resize(std::max(blockSize, startingMidiPerSampleCount));
    for (auto& sampleMessages : midiBlockMessages)
        sampleMessages.reserve(midiInitialCapacity);

#ifdef WAV_SCRIPT_PREMIUM
    wavInput->oscColors = oscInterface.getOscColors();
	wavInput->oscFloats = oscInterface.getOscFloats();
	wavInput->oscInts = oscInterface.getOscInts();
	wavInput->oscStrings = oscInterface.getOscStrings();
#endif
}

WaviateScriptAudioProcessor::~WaviateScriptAudioProcessor()
{
}

//==============================================================================
const juce::String WaviateScriptAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WaviateScriptAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WaviateScriptAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WaviateScriptAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WaviateScriptAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

void WaviateScriptAudioProcessor::loadProgram(const juce::File& file)
{
    auto ext = file.getFileExtension().toStdString();
    auto compiler = compilers.find(ext);
    if (compiler == compilers.end())
        return;

    beginUserScriptCompile();

    juce::StringArray content;
    file.readLines(content);
    auto source = content.joinIntoString("\n");
    SampleShader samp = nullptr;
    FrequencyShader freq = nullptr;

    try
    {
        compiler->second->compileSource(source.toStdString(), samp, freq);
    }
    catch (const std::exception&)
    {
        return;
    }

    if (samp != nullptr || freq != nullptr)
    {
        installCompiledShaders(samp, freq);
    }
}

void WaviateScriptAudioProcessor::setProcessingEnabled(bool shouldBeEnabled)
{
    processingEnabled.store(shouldBeEnabled, std::memory_order_release);
}

bool WaviateScriptAudioProcessor::isProcessingEnabled() const
{
    return processingEnabled.load(std::memory_order_acquire);
}

void WaviateScriptAudioProcessor::beginUserScriptCompile() noexcept
{
    unloadUserScript();
    scriptOverBudget.store(false, std::memory_order_release);
}

void WaviateScriptAudioProcessor::installCompiledShaders(SampleShader sampleShader,
                                                         FrequencyShader frequencyShader) noexcept
{
    activeSampleShader.store(sampleShader, std::memory_order_release);
    activeFrequencyShader.store(frequencyShader, std::memory_order_release);
    scriptOverBudget.store(false, std::memory_order_release);
}

void WaviateScriptAudioProcessor::unloadUserScript() noexcept
{
    activeSampleShader.store(nullptr, std::memory_order_release);
    activeFrequencyShader.store(nullptr, std::memory_order_release);
}

bool WaviateScriptAudioProcessor::isScriptOverBudget() const noexcept
{
    return scriptOverBudget.load(std::memory_order_acquire);
}

void WaviateScriptAudioProcessor::setFuelLimitPreset(waviate::safety::FuelLimitPreset preset) noexcept
{
    fuelLimitPreset.store(static_cast<int>(preset), std::memory_order_release);
}

waviate::safety::FuelLimitPreset WaviateScriptAudioProcessor::getFuelLimitPreset() const noexcept
{
    const auto stored = fuelLimitPreset.load(std::memory_order_acquire);
    const auto presets = waviate::safety::getFuelLimitPresets();

    for (const auto preset : presets)
        if (stored == static_cast<int>(preset))
            return preset;

    return waviate::safety::FuelLimitPreset::medium;
}

void WaviateScriptAudioProcessor::handleScriptOverBudgetFromAudioThread() noexcept
{
    unloadUserScript();
    scriptOverBudget.store(true, std::memory_order_release);
}

uint64_t WaviateScriptAudioProcessor::calculateCurrentSampleFuelBudget(int shaderCallsForSample) const noexcept
{
    return waviate::safety::calculateFuelBudget(getFuelLimitPreset(), shaderCallsForSample);
}

int WaviateScriptAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int WaviateScriptAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WaviateScriptAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WaviateScriptAudioProcessor::getProgramName (int index)
{
    return {};
}

void WaviateScriptAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void WaviateScriptAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : currentSampleRate;
    samplesSinceAppStart = 0;
    sustainDown = false;

    std::fill(midiNoteOnState.begin(), midiNoteOnState.end(), uint8_t { 0 });
    std::fill(midiCCValueState.begin(), midiCCValueState.end(), uint8_t { 0 });
    std::fill(sampleWhenMidiNoteOnState.begin(), sampleWhenMidiNoteOnState.end(), uint64_t { 0 });
    std::fill(sampleWhenMidiNoteOffState.begin(), sampleWhenMidiNoteOffState.end(), uint64_t { 0 });
    std::fill(sampleWhenCCValueChangedState.begin(), sampleWhenCCValueChangedState.end(), uint64_t { 0 });
    std::fill(sustainDeferredNoteOff.begin(), sustainDeferredNoteOff.end(), false);

    wavInput->sampleRate = static_cast<float>(currentSampleRate);

    if (samplesPerBlock > 0 && static_cast<size_t>(samplesPerBlock) > midiBlockMessages.size())
        InitializeMidiMessageLookup(static_cast<size_t>(samplesPerBlock));
}



void WaviateScriptAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WaviateScriptAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

static inline void clearAllNotes(uint8_t* midiNote, std::span<bool, midiStateCount> deferred)
{
    if (midiNote != nullptr)
        std::fill_n(midiNote, midiStateCount, uint8_t { 0 });

    std::fill(deferred.begin(), deferred.end(), false);
}

static inline void applyMidiToState(const juce::MidiMessage& m,
                                   uint8_t* midiNote,
                                   uint8_t* midiCC,
                                   uint64_t* sampleWhenMidiNoteOn,
                                   uint64_t* sampleWhenMidiNoteOff,
                                   uint64_t* sampleWhenCCValueChanged,
                                   bool& sustainDown,
                                   std::span<bool, midiStateCount> sustainDeferredNoteOff,
                                   uint64_t sampleTime)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        if (note >= 0 && note < static_cast<int>(midiStateCount))
        {
            if (midiNote != nullptr)
                midiNote[note] = 1;
            if (sampleWhenMidiNoteOn != nullptr)
                sampleWhenMidiNoteOn[note] = sampleTime;
            sustainDeferredNoteOff[note] = false;
        }
        return;
    }

    if (m.isNoteOff())
    {
        const int note = m.getNoteNumber();
        if (note >= 0 && note < static_cast<int>(midiStateCount))
        {
            if (sampleWhenMidiNoteOff != nullptr)
                sampleWhenMidiNoteOff[note] = sampleTime;

            if (sustainDown)
            {
                // Defer turning it off until sustain releases
                sustainDeferredNoteOff[note] = true;
            }
            else
            {
                if (midiNote != nullptr)
                    midiNote[note] = 0;
                sustainDeferredNoteOff[note] = false;
            }
        }
        return;
    }

    // JUCE treats NoteOn with velocity 0 as NoteOff in some paths,
    // but isNoteOff()/isNoteOn() already cover typical cases.

    if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        clearAllNotes(midiNote, sustainDeferredNoteOff);
        return;
    }

    if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const int v  = m.getControllerValue();

        if (cc < 0 || cc >= static_cast<int>(midiStateCount))
            return;

        if (midiCC != nullptr)
            midiCC[cc] = static_cast<uint8_t>(v);
        if (sampleWhenCCValueChanged != nullptr)
            sampleWhenCCValueChanged[cc] = sampleTime;

        // Sustain pedal (CC 64): standard threshold is >= 64 = down
        if (cc == 64)
        {
            const bool newSustainDown = (v >= 64);

            // sustain released: apply deferred note-offs
            if (sustainDown && !newSustainDown)
            {
                for (size_t n = 0; n < midiStateCount; ++n)
                {
                    if (sustainDeferredNoteOff[n])
                    {
                        if (midiNote != nullptr)
                            midiNote[n] = 0;
                        sustainDeferredNoteOff[n] = false;
                    }
                }
            }

            sustainDown = newSustainDown;
        }

        // Common "All Notes Off" is also sometimes sent as CC 123
        if (cc == 123 || cc == 120)
        {
            clearAllNotes(midiNote, sustainDeferredNoteOff);
        }

        return;
    }
}



void WaviateScriptAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int blockNumSamples        = buffer.getNumSamples();

    if (blockNumSamples <= 0)
        return;

    if (static_cast<size_t>(blockNumSamples) > midiBlockMessages.size())
        InitializeMidiMessageLookup(static_cast<size_t>(blockNumSamples));

    for (int samp = 0; samp < blockNumSamples; ++samp)
        midiBlockMessages[static_cast<size_t>(samp)].clear();

    // Correct JUCE multi-bus handling:
    auto mainIn  = getBusBuffer(buffer, true,  0);
    auto mainOut = getBusBuffer(buffer, false, 0);

    bool sidechainEnabled = false;

#ifdef WAV_SCRIPT_PREMIUM
    {
		OSCInputEvent oscEvent;
        while (oscEventsQueue.popOne(oscEvent)) {
            oscInterface.receiveEventOnAudioThread(oscEvent);
        }
		GameControllerEvent gamepadEvent;
        while (gamepadEventsQueue.popOne(gamepadEvent)) {
            gameControllerInterface.receiveEventOnAudioThread(gamepadEvent);
        }
    }
    
    
#endif

    if (getBusCount(true) > 1)
    {
        if (auto* bus = getBus(true, 1))
            sidechainEnabled = bus->isEnabled();
    }
    juce::AudioBuffer<float>* sideInPtr = nullptr;
    juce::AudioBuffer<float> sideInBuffer;

    if (sidechainEnabled)
    {
        sideInBuffer = getBusBuffer(buffer, true, 1);
        // If the host provides it, sideIn.getNumSamples() should match.
        sideInPtr = &sideInBuffer;
    }

    wavInput->blockSize = buffer.getNumSamples();
    wavInput->sampleRate = static_cast<float>(currentSampleRate);
    wavInput->samplesSinceAppStart = samplesSinceAppStart;
    wavInput->sampleInBlock = 0;
    wavInput->sustain = sustainDown;
    wavInput->previousSamples = nullptr;
    wavInput->sampleMemoryCount = 0;

    // Determine effective input channels on the MAIN input bus
    const int mainInputCh = mainIn.getNumChannels();
    const int mainOutputCh = mainOut.getNumChannels();

    auto pushVisualizerSamples = [&]()
    {
        std::vector<float> samplesPush;
        samplesPush.reserve(mainOutputCh);
        for (int i = 0; i < blockNumSamples && mainOutputCh > 0; i += 1)
        {
            samplesPush.clear();
            for (int j = 0; j < mainOutputCh; j += 1)
                samplesPush.push_back(mainOut.getSample(j, i));

            visualizer.pushSample(samplesPush.data(), mainOutputCh);
        }
    };

    if (! processingEnabled.load(std::memory_order_acquire))
    {
        mainOut.clear();
        samplesSinceAppStart += static_cast<uint64_t>(blockNumSamples);
        pushVisualizerSamples();
        return;
    }

    // Initialize midi arrays if needed (do this in constructor ideally)
    // std::memset(wavInput->midiNote, 0, sizeof(wavInput->midiNote));
    // std::memset(wavInput->midiControllersCC, 0, sizeof(wavInput->midiControllersCC));

    // Point WavInput at the *whole-block* arrays (pointers stable); DSP should use startSample offset.
    wavInput->inputDeviceSamples = mainIn.getArrayOfReadPointers();
    wavInput->currentSampleData = mainOut.getArrayOfWritePointers();
    wavInput->inputChannelCount = mainInputCh;
    wavInput->channelCount = mainOutputCh;

    if (sideInPtr != nullptr && sideInPtr->getNumChannels() > 0)
    {
        for (int c = 0; c < sideInPtr->getNumChannels(); c += 1) {
            wavInput->inputSideChainSamples = sideInPtr->getArrayOfReadPointers();
        }

        wavInput->sideChainChannelCount = sideInPtr->getNumChannels();
    }
    else
    {
        wavInput->inputSideChainSamples = nullptr;
        wavInput->sideChainChannelCount = 0;
    }

    // Ensure outputs beyond inputs are cleared for this segment
    for (int ch = mainInputCh; ch < mainOutputCh; ++ch)
        mainOut.clear(ch, 0, blockNumSamples);

    for (const auto& midiMessage : midiMessages) {
        const int samp = midiMessage.samplePosition;
        if (samp >= 0 && samp < blockNumSamples)
            midiBlockMessages[static_cast<size_t>(samp)].push_back(midiMessage.getMessage());
    }

    SampleShader sampleShader = activeSampleShader.load(std::memory_order_acquire);
    FrequencyShader freqShader = activeFrequencyShader.load(std::memory_order_acquire);
    const int shaderCallsPerSample = (sampleShader != nullptr ? mainOutputCh : 0)
                                   + (freqShader != nullptr ? 1 : 0);

    auto scriptExceededFuel = false;

    // ----- SampleWise Processing (segment) -----
    if (sampleShader) {
        for (int samp = 0; samp < blockNumSamples; ++samp)
        {
            waviate::safety::FuelState fuelState {
                calculateCurrentSampleFuelBudget(shaderCallsPerSample),
                false
            };

            {
                waviate::safety::ScopedFuelBudget fuelScope(fuelState);
                const uint64_t absoluteSample = samplesSinceAppStart + static_cast<uint64_t>(samp);
                auto& sampleMidiMessages = midiBlockMessages[static_cast<size_t>(samp)];

                for (const auto& midiMessage : sampleMidiMessages) {
                    applyMidiToState(
                        midiMessage,
                        wavInput->midiNoteOn,
                        wavInput->midiCCValue,
                        wavInput->sampleWhenMidiNoteOn,
                        wavInput->sampleWhenMidiNoteOff,
                        wavInput->sampleWhenCCValueChanged,
                        sustainDown,
                        std::span<bool, midiStateCount>(wavInput->sustainDefer, midiStateCount),
                        absoluteSample);
                }

                wavInput->sampleInBlock = samp;
                wavInput->samplesSinceAppStart = absoluteSample;
                wavInput->sustain = sustainDown;

                for (int ch = 0; ch < mainOutputCh; ++ch)
                {
                    waviate::safety::ScopedArenaPass arenaPass(ephemeralArena);
                    wavInput->channel = static_cast<uint8_t>(ch);
                    mainOut.getWritePointer(ch)[samp] = sampleShader(wavInput.get(), nullptr);

                    if (fuelState.exhausted || ephemeralArena.wasExhausted())
                        break;
                }
            }

            midiBlockMessages[static_cast<size_t>(samp)].clear();

            if (fuelState.exhausted || ephemeralArena.wasExhausted())
            {
                scriptExceededFuel = true;
                break;
            }
        }
    }

    if (freqShader) {

    }

    if (scriptExceededFuel)
    {
        handleScriptOverBudgetFromAudioThread();
        mainOut.clear();
        samplesSinceAppStart += static_cast<uint64_t>(blockNumSamples);
        pushVisualizerSamples();
        return;
    }

    samplesSinceAppStart += static_cast<uint64_t>(blockNumSamples);

    pushVisualizerSamples();

}


//==============================================================================
bool WaviateScriptAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* WaviateScriptAudioProcessor::createEditor()
{
    return new WaviateScriptAudioProcessorEditor (*this);
}

//==============================================================================
void WaviateScriptAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void WaviateScriptAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaviateScriptAudioProcessor();
}
