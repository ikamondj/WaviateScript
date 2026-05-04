/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "WaviateInput.h"
#include "AbstractCompiler.h"
#include <array>
#include <atomic>

#ifdef WAV_SCRIPT_PREMIUM
#include "GameControllerInterface.h"
#include "GameControllerInputEvent.h"
#include "OSCInterface.h"
#include "OSCInput.h"
#endif

typedef float (*SampleWiseProcessor)(const WaviateSampleInput*, void* state);

//==============================================================================
/**
*/
class WaviateScriptAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    WaviateScriptAudioProcessor();
    
    ~WaviateScriptAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    std::atomic<SampleShader> activeSampleShader;
    std::atomic<FrequencyShader> activeFrequencyShader;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    void loadProgram(const juce::File& file);
    void setProcessingEnabled(bool shouldBeEnabled);
    bool isProcessingEnabled() const;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    std::unique_ptr<WaviateSampleInput> wavInput;
    std::unordered_map<std::string, std::unique_ptr<AbstractCompiler>> compilers;
    std::vector<std::vector<juce::MidiMessage>> midiBlockMessages;
    juce::AudioVisualiserComponent visualizer;
#ifdef WAV_SCRIPT_PREMIUM
    GameControllerInterface gameControllerInterface;
    SpscEventQueue<GameControllerEvent, cueCap, true> gamepadEventsQueue;
	OSCInterface oscInterface;
	SpscEventQueue<OSCInputEvent, oscCap, true> oscEventsQueue;
#endif
private:
    void InitializeMidiMessageLookup(size_t blockSize);

    std::array<uint8_t, 128> midiNoteOnState{};
    std::array<uint8_t, 128> midiCCValueState{};
    std::array<uint64_t, 128> sampleWhenMidiNoteOnState{};
    std::array<uint64_t, 128> sampleWhenMidiNoteOffState{};
    std::array<uint64_t, 128> sampleWhenCCValueChangedState{};
    std::array<bool, 128> sustainDeferredNoteOff{};
    bool sustainDown = false;
    double currentSampleRate = 44100.0;
    uint64_t samplesSinceAppStart = 0;
    std::atomic<bool> processingEnabled { true };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaviateScriptAudioProcessor)
};
