/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
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

class WaviateScriptAudioProcessor::AudioLoaderThread : public juce::Thread
{
public:
    AudioLoaderThread(WaviateScriptAudioProcessor& processor, waviate::audio::WaviateAudioCache& cache)
        : juce::Thread("AudioLoaderThread"), processor_(processor), cache_(cache)
    {
    }

    ~AudioLoaderThread() override
    {
        stopThread(4000);
    }

    void trigger()
    {
        triggerEvent.signal();
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            if (! triggerEvent.wait(500))
                continue;

            if (threadShouldExit())
                break;

            waviate::audio::WaviateAudioLoadRequest req;
            while (cache_.popPendingRequest(req))
            {
                if (threadShouldExit())
                    break;

                juce::String locationStr = juce::String::fromUTF8(req.location.data(), static_cast<int>(req.location.size()));
                juce::Logger::writeToLog("Loading audio from: " + locationStr);

                auto result = loadAudioLocation(locationStr);
                if (result.succeeded)
                {
                    cache_.storeLoadedAudio(req.location, std::move(result.interleavedSamples), result.channelCount, result.sampleRate);
                    triggerUIUpdate();
                }
                else
                {
                    cache_.markFailed(req.location, result.errorMessage);
                    triggerUIUpdate();
                }
            }
        }
    }

    void triggerUIUpdate()
    {
        juce::MessageManager::callAsync([this]() {
            if (processor_.onAudioCacheChanged)
                processor_.onAudioCacheChanged();
        });
    }

    struct LoadResult
    {
        std::vector<float> interleavedSamples;
        int32_t channelCount = 0;
        float sampleRate = 0.0f;
        bool succeeded = false;
        std::string errorMessage;
    };

    static LoadResult loadAudioLocation(const juce::String& location)
    {
        LoadResult result;
        
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::InputStream> stream;

        if (location.startsWithIgnoreCase("http://") || location.startsWithIgnoreCase("https://"))
        {
#ifdef WAV_SCRIPT_PREMIUM
            juce::URL url(location);
            if (! url.isWellFormed())
            {
                result.errorMessage = "Malformed URL: " + location.toStdString();
                return result;
            }

            juce::MemoryBlock mb;
            if (! url.readEntireBinaryStream(mb))
            {
                result.errorMessage = "Failed to download from URL: " + location.toStdString();
                return result;
            }

            stream = std::make_unique<juce::MemoryInputStream>(mb, false);
#else
            result.errorMessage = "Loading from URL requires Premium edition.";
            return result;
#endif
        }
        else
        {
            juce::File file = juce::File::createFileWithoutCheckingPath(location);
            if (! file.existsAsFile())
            {
                result.errorMessage = "File does not exist: " + file.getFullPathName().toStdString();
                return result;
            }
            stream = file.createInputStream();
            if (stream == nullptr)
            {
                result.errorMessage = "Failed to open file: " + file.getFullPathName().toStdString();
                return result;
            }
        }

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(std::move(stream)));
        if (reader == nullptr)
        {
            result.errorMessage = "Unsupported or corrupt format: " + location.toStdString();
            return result;
        }

        if (reader->numChannels == 0 || reader->lengthInSamples == 0)
        {
            result.errorMessage = "Invalid channels or length";
            return result;
        }

        const auto numSamples = reader->lengthInSamples;
        const auto numChannels = static_cast<int>(reader->numChannels);
        juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(numSamples));
        
        if (! reader->read(&buffer, 0, static_cast<int>(numSamples), 0, true, true))
        {
            result.errorMessage = "Failed to read audio samples";
            return result;
        }

        result.channelCount = numChannels;
        result.sampleRate = static_cast<float>(reader->sampleRate);
        result.interleavedSamples.resize(static_cast<size_t>(numSamples) * static_cast<size_t>(numChannels));

        for (int c = 0; c < numChannels; ++c)
        {
            const float* src = buffer.getReadPointer(c);
            for (size_t i = 0; i < static_cast<size_t>(numSamples); ++i)
            {
                result.interleavedSamples[i * static_cast<size_t>(numChannels) + static_cast<size_t>(c)] = src[i];
            }
        }

        result.succeeded = true;
        return result;
    }

private:
    WaviateScriptAudioProcessor& processor_;
    waviate::audio::WaviateAudioCache& cache_;
    juce::WaitableEvent triggerEvent;
};

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

    InitializeMidiMessageLookup(maxBlockSize);

    audioLoaderThread = std::make_unique<AudioLoaderThread>(*this, audioCache);
    audioCache.onRequestAdded = [this]() {
        if (audioLoaderThread)
            audioLoaderThread->trigger();
    };
    audioLoaderThread->startThread();
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
    audioLoaderThread.reset();
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
    if (! file.existsAsFile())
        return;

    juce::StringArray content;
    file.readLines(content);
    static_cast<void>(compileAndActivateSource(file.getFileExtension(), content.joinIntoString("\n")));
}

WaviateScriptAudioProcessor::CompilationActivationResult
WaviateScriptAudioProcessor::compileAndActivateSource(const juce::String& extension, const juce::String& source)
{
    CompilationActivationResult result;

    try
    {
        const auto compiled = compilePipeline.compile(extension.toStdString(), source.toStdString());
        result.succeeded = true;
        result.hasSampleShader = compiled.sampleShader != nullptr;
        result.hasFrequencyShader = compiled.frequencyShader != nullptr;

        if (compiled.hasEntryPoints())
        {
            storeRuntimeControls(compiled.runtime);
            scriptOverBudget.store(false, std::memory_order_release);
            activeSampleShader.store(compiled.sampleShader, std::memory_order_release);
            activeFrequencyShader.store(compiled.frequencyShader, std::memory_order_release);
        }
        else
        {
            deactivateActiveScript(false);
        }
    }
    catch (const std::exception& e)
    {
        result.errorMessage = e.what();
    }

    return result;
}

void WaviateScriptAudioProcessor::setProcessingEnabled(bool shouldBeEnabled)
{
    processingEnabled.store(shouldBeEnabled, std::memory_order_release);
}

bool WaviateScriptAudioProcessor::isProcessingEnabled() const
{
    return processingEnabled.load(std::memory_order_acquire);
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

    prepareFrequencyWorkspace(std::max(getTotalNumInputChannels(), getTotalNumOutputChannels()));
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



#include "AudioProcessingFlow.inl"

void WaviateScriptAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumSamples() <= 0)
        return;

    const auto sampleCount = static_cast<size_t>(buffer.getNumSamples());
    if (sampleCount > midiBlockMessages.size())
        InitializeMidiMessageLookup(sampleCount);
    for (size_t sample = 0; sample < sampleCount; ++sample)
        midiBlockMessages[sample].clear();

    AudioBlockContext context { buffer };
    setupBlockData(context);

    if (! processingEnabled.load(std::memory_order_acquire))
    {
        context.mainOut.clear();
    }
    else if (! processSamples(context, midiMessages))
    {
        context.mainOut.clear();
        deactivateActiveScript(true);
    }
    else
    {
        setupFrequencyStep(context);
        if (! processFrequencyBins(context))
        {
            context.mainOut.clear();
            deactivateActiveScript(true);
        }
    }

    samplesSinceAppStart += static_cast<uint64_t>(context.sampleCount);
    pushVisualizerSamples(context);
}

void WaviateScriptAudioProcessor::prepareFrequencyWorkspace(int channelCount)
{
    auto workspace = std::make_unique<FrequencyWorkspace>();
    for (int order = FrequencyWorkspace::minimumOrder; order <= FrequencyWorkspace::maximumOrder; ++order)
        workspace->plans[static_cast<size_t>(order - FrequencyWorkspace::minimumOrder)] = std::make_unique<juce::dsp::FFT>(order);

    const auto channels = static_cast<size_t>(std::max(1, channelCount));
    const auto transformSize = static_cast<size_t>(FrequencyWorkspace::maximumSize * 2);
    const auto spectrumSize = static_cast<size_t>(FrequencyWorkspace::maximumSize / 2 + 1);
    workspace->inputTransform.assign(channels, std::vector<float>(transformSize));
    workspace->outputTransform.assign(channels, std::vector<float>(transformSize));
    workspace->sidechainTransform.assign(channels, std::vector<float>(transformSize));
    workspace->inputSpectrum.assign(channels, std::vector<WaviateComplex>(spectrumSize));
    workspace->outputSpectrum.assign(channels, std::vector<WaviateComplex>(spectrumSize));
    workspace->sidechainSpectrum.assign(channels, std::vector<WaviateComplex>(spectrumSize));
    workspace->inputPointers.resize(channels);
    workspace->outputPointers.resize(channels);
    workspace->sidechainPointers.resize(channels);
    workspace->visualizerFrame.resize(channels);
    frequencyWorkspace = std::move(workspace);
}

void WaviateScriptAudioProcessor::setFrequencyDomainSettings(FrequencyDomainSettings settings) noexcept
{
    const int clampedSize = juce::jlimit(1 << FrequencyWorkspace::minimumOrder,
                                         1 << FrequencyWorkspace::maximumOrder,
                                         settings.fftSize);
    const int order = juce::roundToInt(std::log2(static_cast<double>(clampedSize)));
    requestedFftSize.store(1 << order, std::memory_order_release);
    requestedBinLimit.store(std::max(0, settings.binLimit), std::memory_order_release);
    requestedFftWindow.store(static_cast<int>(settings.window), std::memory_order_release);
}

WaviateScriptAudioProcessor::FrequencyDomainSettings
WaviateScriptAudioProcessor::getFrequencyDomainSettings() const noexcept
{
    FrequencyDomainSettings settings;
    settings.fftSize = requestedFftSize.load(std::memory_order_acquire);
    settings.binLimit = requestedBinLimit.load(std::memory_order_acquire);
    settings.window = static_cast<FftWindow>(requestedFftWindow.load(std::memory_order_acquire));
    return settings;
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

waviate::audio::WaviateAudioCache& WaviateScriptAudioProcessor::getAudioCache() noexcept
{
    return audioCache;
}

const waviate::audio::WaviateAudioCache& WaviateScriptAudioProcessor::getAudioCache() const noexcept
{
    return audioCache;
}

const std::vector<WaviateScriptAudioProcessor::ManualClipInfo>& WaviateScriptAudioProcessor::getManualClips() const noexcept
{
    return manualClips;
}

size_t WaviateScriptAudioProcessor::getManualClipCount() const noexcept
{
    return manualClips.size();
}

void WaviateScriptAudioProcessor::addManualClip(juce::String path, juce::String name)
{
    manualClips.push_back({path, name});
    if (path.isNotEmpty())
        audioCache.registerManualClip(path.toStdString(), name.toStdString());
}

void WaviateScriptAudioProcessor::setManualClipName(size_t index, juce::String name)
{
    if (index < manualClips.size())
    {
        manualClips[index].name = name;
        if (manualClips[index].path.isNotEmpty())
            audioCache.setClipName(manualClips[index].path.toStdString(), name.toStdString());
    }
}

void WaviateScriptAudioProcessor::setManualClipPath(size_t index, juce::String path)
{
    if (index < manualClips.size())
    {
        if (manualClips[index].path.isNotEmpty())
            audioCache.removeManualClip(manualClips[index].path.toStdString());

        manualClips[index].path = path;
        
        if (path.isNotEmpty())
            audioCache.registerManualClip(path.toStdString(), manualClips[index].name.toStdString());
    }
}

void WaviateScriptAudioProcessor::removeManualClipAt(size_t index)
{
    if (index < manualClips.size())
    {
        if (manualClips[index].path.isNotEmpty())
            audioCache.removeManualClip(manualClips[index].path.toStdString());
        
        manualClips.erase(manualClips.begin() + static_cast<std::ptrdiff_t>(index));
    }
}

void WaviateScriptAudioProcessor::clearManualClips()
{
    for (const auto& clip : manualClips)
    {
        if (clip.path.isNotEmpty())
            audioCache.removeManualClip(clip.path.toStdString());
    }
    manualClips.clear();
}

void WaviateScriptAudioProcessor::clearAllAudioClips()
{
    audioCache.clear();
    manualClips.clear();
}

void WaviateScriptAudioProcessor::setFuelLimitPreset(waviate::compile::FuelLimitPreset preset) noexcept
{
    fuelLimitPresetIndex.store(static_cast<int>(preset), std::memory_order_relaxed);
}

waviate::compile::FuelLimitPreset WaviateScriptAudioProcessor::getFuelLimitPreset() const noexcept
{
    return static_cast<waviate::compile::FuelLimitPreset>(fuelLimitPresetIndex.load(std::memory_order_relaxed));
}

bool WaviateScriptAudioProcessor::isScriptOverBudget() const noexcept
{
    return scriptOverBudget.load(std::memory_order_relaxed);
}

void WaviateScriptAudioProcessor::storeRuntimeControls(const ShaderRuntimeControls& runtime) noexcept
{
    activeSetFuelBudget.store(runtime.setFuelBudget, std::memory_order_release);
    activeGetFuelRemaining.store(runtime.getFuelRemaining, std::memory_order_release);
    activeGetFuelExhausted.store(runtime.getFuelExhausted, std::memory_order_release);
}

ShaderRuntimeControls WaviateScriptAudioProcessor::loadRuntimeControls() const noexcept
{
    ShaderRuntimeControls runtime;
    runtime.setFuelBudget = activeSetFuelBudget.load(std::memory_order_acquire);
    runtime.getFuelRemaining = activeGetFuelRemaining.load(std::memory_order_acquire);
    runtime.getFuelExhausted = activeGetFuelExhausted.load(std::memory_order_acquire);
    return runtime;
}

void WaviateScriptAudioProcessor::deactivateActiveScript(bool markOverBudget) noexcept
{
    activeSampleShader.store(nullptr, std::memory_order_release);
    activeFrequencyShader.store(nullptr, std::memory_order_release);
    storeRuntimeControls({});
    scriptOverBudget.store(markOverBudget, std::memory_order_release);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaviateScriptAudioProcessor();
}
