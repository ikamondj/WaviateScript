/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Compilers.h"
#include "fftw3.h"
#include <span>

const size_t maxBlockSize = 8192;
const size_t startingMidiPerSampleCount = 64;
const size_t midiInitialCapacity = 128;

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
    , visualizer(2)
#ifdef WAV_SCRIPT_PREMIUM
    , gameControllerInterface(gamepadEventsQueue)
#endif 
#endif
{
    wavInput = std::make_unique<WaviateSampleInput>();

    compilers.insert({ ".wc", std::make_unique<ClangCompiler<false>>()});
    compilers.insert({ ".wcpp", std::make_unique<ClangCompiler<true>>()});
#ifdef WAV_SCRIPT_PREMIUM
    compilers.insert({ ".wrs", std::make_unique<RustCompiler>() });
#endif
    InitializeMidiMessageLookup(maxBlockSize);
}

void WaviateScriptAudioProcessor::InitializeMidiMessageLookup(size_t blockSize) {
    midiBlockMessages.resize(maxBlockSize);
    for (int i = 0; i < maxBlockSize; i += 1) {
        midiBlockMessages[i].reserve(midiInitialCapacity);
    }
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
    juce::StringArray content;
    file.readLines(content);
    auto source = content.joinIntoString("\n");
    SampleShader samp;
    FrequencyShader freq;
    compilers[ext]->compileSource(source.toStdString(), samp, freq);
    activeSampleShader.store(samp, std::memory_order_release);
    activeFrequencyShader.store(freq, std::memory_order_release);
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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
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

#include <array>
#include <algorithm>
#include <cstring>

// Put this somewhere persistent (member variables), not inside processBlock:
std::array<bool, 128> sustainDeferredNoteOff{}; // notes released while sustain is down
bool sustainDown = false;

static inline void clearAllNotes(uint8_t* midiNote, std::span<bool,128> deferred)
{
    std::memset(midiNote, 0, 128 * sizeof(bool));
    for (int i = 0; i < deferred.size(); i += 1) {
        deferred[i] = false;
    }
}

static inline void applyMidiToState(const juce::MidiMessage& m,
                                   uint8_t* midiNote,
                                   uint8_t* midiCC,
                                   bool& sustainDown,
                                   std::span<bool, 128> sustainDeferredNoteOff)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        if (note >= 0 && note < 128)
        {
            midiNote[note] = 1;
            sustainDeferredNoteOff[note] = false;
        }
        return;
    }

    if (m.isNoteOff())
    {
        const int note = m.getNoteNumber();
        if (note >= 0 && note < 128)
        {
            if (sustainDown)
            {
                // Defer turning it off until sustain releases
                sustainDeferredNoteOff[note] = true;
            }
            else
            {
                midiNote[note] = false;
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

        midiCC[cc] = static_cast<uint8_t>(v);

        // Sustain pedal (CC 64): standard threshold is >= 64 = down
        if (cc == 64)
        {
            const bool newSustainDown = (v >= 64);

            // sustain released: apply deferred note-offs
            if (sustainDown && !newSustainDown)
            {
                for (int n = 0; n < 128; ++n)
                {
                    if (sustainDeferredNoteOff[(size_t)n])
                    {
                        midiNote[n] = false;
                        sustainDeferredNoteOff[(size_t)n] = false;
                    }
                }
            }

            sustainDown = newSustainDown;
        }

        // Common “All Notes Off” is also sometimes sent as CC 123
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

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int blockNumSamples        = buffer.getNumSamples();

    if (blockNumSamples > midiBlockMessages.size()) {
        InitializeMidiMessageLookup(blockNumSamples);
    }

    // Correct JUCE multi-bus handling:
    auto mainIn  = getBusBuffer(buffer, true,  0);
    auto mainOut = getBusBuffer(buffer, false, 0);

    bool sidechainEnabled = false;

#ifdef WAV_SCRIPT_PREMIUM

#endif

    if (getBusCount(true) > 1)
    {
        if (auto* bus = getBus(true, 1))
            sidechainEnabled = bus->isEnabled();
    }
    juce::AudioBuffer<float>* sideInPtr = nullptr;
    juce::AudioBuffer<float> sideInDummy;

    if (sidechainEnabled)
    {
        auto sideIn = getBusBuffer(buffer, true, 1);
        // If the host provides it, sideIn.getNumSamples() should match.
        sideInPtr = &sideIn;
    }

    wavInput->blockSize = buffer.getNumSamples();

    // Determine effective input channels on the MAIN input bus
    const int mainInputCh = mainIn.getNumChannels();
    const int mainOutputCh = mainOut.getNumChannels();

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
        int samp = midiMessage.samplePosition;
        midiBlockMessages[samp].push_back(midiMessage.getMessage());
    }

    // ----- SampleWise Processing (segment) -----
    SampleShader sampleShader = activeSampleShader.load(std::memory_order_acquire);
    if (sampleShader) {
        for (int ch = 0; ch < mainInputCh; ++ch)
        {
            wavInput->channel = ch;

            const float* in = mainIn.getReadPointer(ch, 0);
            float* out = mainOut.getWritePointer(ch, 0);

            for (int samp = 0; samp < blockNumSamples; samp += 1) {
                for (int m = 0; m < midiBlockMessages.size(); m += 1) {
                    applyMidiToState(midiBlockMessages[samp][m], wavInput->midiNoteOn, wavInput->midiCCValue, wavInput->sustain, std::span<bool, 128>(wavInput->sustainDefer, 128));
                    out[samp] = sampleShader(wavInput.get(), nullptr);
                }
                midiBlockMessages[samp].clear();
            }
        }
    }

    FrequencyShader freqShader = activeFrequencyShader.load(std::memory_order_acquire);
    if (freqShader) {

    }

    std::vector<float> samplesPush;
    samplesPush.reserve(totalNumOutputChannels);
    for (int i = 0; i < blockNumSamples; i += 1) {
        samplesPush.clear();
        for (int j = 0; j < totalNumOutputChannels; j += 1) {
            samplesPush.push_back(mainOut.getSample(j, i));
        }
        visualizer.pushSample(samplesPush.data(), totalNumOutputChannels);
    }

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
