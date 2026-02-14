/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "AlignedBlob.h"

//==============================================================================
WaviateScriptAudioProcessor::WaviateScriptAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    wavInput = std::make_unique<WavInput>();
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

void WaviateScriptAudioProcessor::loadProgram()
{

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

static inline void clearAllNotes(bool midiNote[128], std::array<bool,128>& deferred)
{
    std::memset(midiNote, 0, 128 * sizeof(bool));
    deferred.fill(false);
}

static inline void applyMidiToState(const juce::MidiMessage& m,
                                   bool midiNote[128],
                                   float midiCC[128],
                                   bool& sustainDown,
                                   std::array<bool,128>& sustainDeferredNoteOff)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        if (note >= 0 && note < 128)
        {
            midiNote[note] = true;
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

        if (cc >= 0 && cc < 128)
            midiCC[cc] = static_cast<float>(v) / 127.0f;

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

// Helper: process one contiguous [start, start+count) segment.
// Replace the guts with your sample-wise / frequency-wise work.
void WaviateScriptAudioProcessor::processSegment(juce::AudioBuffer<float>& mainOut,
                                                 const juce::AudioBuffer<float>& mainIn,
                                                 const juce::AudioBuffer<float>* sideIn,
                                                 int startSample,
                                                 int numSamplesInSegment,
                                                 int numInputCh,
                                                 int numOutputCh)
{
    // Point WavInput at the *whole-block* arrays (pointers stable); DSP should use startSample offset.
    wavInput->inputSamples = mainIn.getArrayOfReadPointers();
    wavInput->outputSamples = mainOut.getArrayOfReadPointers();
    wavInput->numSamples   = numSamplesInSegment; // segment size, not whole block
    wavInput->numInputChannels  = numInputCh;
    wavInput->numOutputChannels = numOutputCh;

    if (sideIn != nullptr && sideIn->getNumChannels() > 0)
    {
        wavInput->sideChainSamples    = sideIn->getReadPointer(0, startSample); // mono sidechain view
        wavInput->numSideChainSamples = numSamplesInSegment;
    }
    else
    {
        wavInput->sideChainSamples    = nullptr;
        wavInput->numSideChainSamples = 0;
    }

    // Ensure outputs beyond inputs are cleared for this segment
    for (int ch = numInputCh; ch < numOutputCh; ++ch)
        mainOut.clear(ch, startSample, numSamplesInSegment);

    // ----- SampleWise Processing (segment) -----
    for (int ch = 0; ch < numInputCh; ++ch)
    {
        wavInput->currentOutputChannel = ch;

        const float* in  = mainIn.getReadPointer(ch, startSample);
        float* out       = mainOut.getWritePointer(ch, startSample);

        // Example pass-through (replace with your time-domain callback)
        std::memcpy(out, in, sizeof(float) * (size_t)numSamplesInSegment);

        // If you have a DLL function pointer like:
        //   void (*processTime)(WavInput* in, float* out, int numSamples);
        // call it here, per-channel or per-block as you designed.
    }

    // ----- FrequencyWise Processing (segment) -----
    // If you do FFT, do NOT assume segment size == FFT size.
    // Use an internal fixed size / overlap-add, or only run FFT path when requested.
    // Placeholder: no-op here.
}

void WaviateScriptAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int blockNumSamples        = buffer.getNumSamples();

    // Correct JUCE multi-bus handling:
    auto& mainIn  = getBusBuffer(buffer, true,  0);
    auto& mainOut = getBusBuffer(buffer, false, 0);

    const bool sidechainEnabled = (getBusCount(true) > 1) && (getBuses(true)[1] != nullptr);
    juce::AudioBuffer<float>* sideInPtr = nullptr;
    juce::AudioBuffer<float> sideInDummy;

    if (sidechainEnabled)
    {
        auto& sideIn = getBusBuffer(buffer, true, 1);
        // If the host provides it, sideIn.getNumSamples() should match.
        sideInPtr = &sideIn;
    }

    // Determine effective input channels on the MAIN input bus
    const int mainInputCh = mainIn.getNumChannels();
    const int mainOutputCh = mainOut.getNumChannels();

    // Initialize midi arrays if needed (do this in constructor ideally)
    // std::memset(wavInput->midiNote, 0, sizeof(wavInput->midiNote));
    // std::memset(wavInput->midiControllersCC, 0, sizeof(wavInput->midiControllersCC));

    // Build segment boundaries: MIDI events are sorted by samplePosition in Juce::MidiBuffer iteration.
    int segmentStart = 0;

    auto it = midiMessages.begin();
    while (it != midiMessages.end())
    {
        const auto meta = *it;
        const int eventSamplePos = juce::jlimit(0, blockNumSamples, meta.samplePosition);

        // Process audio up to this event
        const int segLen = eventSamplePos - segmentStart;
        if (segLen > 0)
        {
            processSegment(mainOut, mainIn, sideInPtr, segmentStart, segLen,
                           mainInputCh, mainOutputCh);
            segmentStart = eventSamplePos;
        }

        // Apply all events at this same sample position
        const int currentPos = eventSamplePos;
        while (it != midiMessages.end() && (*it).samplePosition == currentPos)
        {
            const juce::MidiMessage msg((*it).getMessage());
            applyMidiToState(msg,
                             wavInput->midiNote,
                             wavInput->midiControllersCC,
                             sustainDown,
                             sustainDeferredNoteOff);
            ++it;
        }
    }

    // Process remaining tail
    if (segmentStart < blockNumSamples)
    {
        processSegment(mainOut, mainIn, sideInPtr, segmentStart, blockNumSamples - segmentStart,
                       mainInputCh, mainOutputCh);
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
