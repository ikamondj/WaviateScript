/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "WaviateInput.h"
#include "AbstractCompiler.h"
#include <atomic>

#ifdef WAV_SCRIPT_PREMIUM
#include "WSPremium/GameControllerInterface.h"
#include "WSPremium/GameControllerInputEvent.h"
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

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void processSegment(juce::AudioBuffer<float>& mainOut,
        const juce::AudioBuffer<float>& mainIn,
        const juce::AudioBuffer<float>* sideIn,
        int startSample,
        int numSamplesInSegment,
        int numInputCh,
        int numOutputCh);
    std::unique_ptr<WaviateSampleInput> wavInput;
    std::unordered_map<std::string, std::unique_ptr<AbstractCompiler>> compilers;
#ifdef WAV_SCRIPT_PREMIUM
    GameControllerInterface gameControllerInterface;
    SpscEventQueue<GameControllerEvent, cueCap, true> gamepadEventsQueue;
#endif
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaviateScriptAudioProcessor)
};
