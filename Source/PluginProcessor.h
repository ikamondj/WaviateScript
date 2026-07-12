/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "WaviateInput.h"
#include "AbstractCompiler.h"
#include "CompilePipeline.h"
#include "WaviateAudio.h"
#include "WaviateSafety.h"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

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
    enum class FftWindow : int { rectangular, hann };

    struct FrequencyDomainSettings
    {
        int fftSize = 1024;
        int binLimit = 0; // Zero processes every positive-frequency bin.
        FftWindow window = FftWindow::rectangular;
    };

    struct CompilationActivationResult
    {
        bool succeeded = false;
        bool hasSampleShader = false;
        bool hasFrequencyShader = false;
        juce::String errorMessage;

        explicit operator bool() const noexcept { return succeeded; }
    };

    struct ManualClipInfo
    {
        juce::String path;
        juce::String name;
    };

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
    CompilationActivationResult compileAndActivateSource(const juce::String& extension, const juce::String& source);
    void setProcessingEnabled(bool shouldBeEnabled);
    bool isProcessingEnabled() const;
    void setFrequencyDomainSettings(FrequencyDomainSettings settings) noexcept;
    [[nodiscard]] FrequencyDomainSettings getFrequencyDomainSettings() const noexcept;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void setFuelLimitPreset(waviate::compile::FuelLimitPreset preset) noexcept;
    [[nodiscard]] waviate::compile::FuelLimitPreset getFuelLimitPreset() const noexcept;
    [[nodiscard]] bool isScriptOverBudget() const noexcept;
    [[nodiscard]] waviate::audio::WaviateAudioCache& getAudioCache() noexcept;
    [[nodiscard]] const waviate::audio::WaviateAudioCache& getAudioCache() const noexcept;
    [[nodiscard]] const std::vector<ManualClipInfo>& getManualClips() const noexcept;
    [[nodiscard]] size_t getManualClipCount() const noexcept;
    void addManualClip(juce::String path, juce::String name);
    void setManualClipName(size_t index, juce::String name);
    void setManualClipPath(size_t index, juce::String path);
    void removeManualClipAt(size_t index);
    void clearManualClips();
    void clearAllAudioClips();
    std::unique_ptr<WaviateSampleInput> wavInput;
    std::vector<std::vector<juce::MidiMessage>> midiBlockMessages;
    juce::AudioVisualiserComponent visualizer;
    std::function<void()> onAudioCacheChanged;
#ifdef WAV_SCRIPT_PREMIUM
    GameControllerInterface gameControllerInterface;
    SpscEventQueue<GameControllerEvent, cueCap, true> gamepadEventsQueue;
	OSCInterface oscInterface;
	SpscEventQueue<OSCInputEvent, oscCap, true> oscEventsQueue;
#endif
private:
    struct AudioBlockContext;
    struct FrequencyWorkspace;

    inline void setupBlockData(AudioBlockContext&) noexcept;
    inline void setupCommonBlockData(AudioBlockContext&) noexcept;
    inline void setupNonPremiumCommonBlockData(AudioBlockContext&) noexcept;
    inline void setupStandaloneInputData(AudioBlockContext&) noexcept;
    inline void setupNonPremiumStandaloneInputData(AudioBlockContext&) noexcept;
#ifdef WAV_SCRIPT_PREMIUM
    inline void setupPremiumCommonBlockData(AudioBlockContext&) noexcept;
    inline void setupPremiumStandaloneInputData(AudioBlockContext&) noexcept;
    inline void setupDawInputData(AudioBlockContext&) noexcept;
#endif
    inline bool processSamples(AudioBlockContext&, juce::MidiBuffer&) noexcept;
    inline void setupFrequencyStep(AudioBlockContext&) noexcept;
    inline void setupCommonFrequencyBlockData(AudioBlockContext&) noexcept;
    inline void setupStandaloneFrequencyBlockData(AudioBlockContext&) noexcept;
#ifdef WAV_SCRIPT_PREMIUM
    inline void setupDawFrequencyBlockData(AudioBlockContext&) noexcept;
#endif
    inline bool processFrequencyBins(AudioBlockContext&) noexcept;
    inline void pushVisualizerSamples(const AudioBlockContext&);
    void prepareFrequencyWorkspace(int channelCount);

    void InitializeMidiMessageLookup(size_t blockSize);
    void storeRuntimeControls(const ShaderRuntimeControls& runtime) noexcept;
    [[nodiscard]] ShaderRuntimeControls loadRuntimeControls() const noexcept;
    void deactivateActiveScript(bool markOverBudget) noexcept;

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
    std::atomic<bool> scriptOverBudget { false };
    std::atomic<int> fuelLimitPresetIndex { static_cast<int>(waviate::compile::FuelLimitPreset::Medium) };
    std::atomic<int> requestedFftSize { 1024 };
    std::atomic<int> requestedBinLimit { 0 };
    std::atomic<int> requestedFftWindow { static_cast<int>(FftWindow::rectangular) };
    std::atomic<ShaderSetFuelBudgetFn> activeSetFuelBudget { nullptr };
    std::atomic<ShaderGetFuelRemainingFn> activeGetFuelRemaining { nullptr };
    std::atomic<ShaderGetFuelExhaustedFn> activeGetFuelExhausted { nullptr };
    waviate::audio::WaviateAudioCache audioCache;
    waviate::compile::Pipeline compilePipeline;
    waviate::safety::EphemeralArena shaderArena;
    std::vector<ManualClipInfo> manualClips;
    std::unique_ptr<FrequencyWorkspace> frequencyWorkspace;

    class AudioLoaderThread;
    std::unique_ptr<AudioLoaderThread> audioLoaderThread;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaviateScriptAudioProcessor)
};
