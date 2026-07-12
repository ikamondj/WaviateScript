# C++ Shader API

C++ is the only first-class shader authoring interface currently supported. A shader may define either or both entry points:

```cpp
float SampleProcess(WaviateSample& wav);
WaviateComplex FrequencyProcess(WaviateFrequency& wav);
```

The `wav` object is the user-facing context. Its data is private: context values are exposed through read-only `const` member functions. Functions beginning with `set` mutate frame-local shader state and are non-const. Mutation occurs synchronously on the audio thread while the shader is running.

## Shared context

Both context types inherit these `const` functions from `WaviateCore`:

```cpp
float secondsSinceAppStart() const;
float samplesToSeconds(uint64_t samples) const;
uint64_t secondsToSamples(float seconds) const;
float sampleRateHz() const;
float sampleRateKHz() const;
```

MIDI is also shared by sample and frequency contexts in public and premium builds:

```cpp
bool isMidiNoteOn(int note) const;
uint8_t midiCCValue(int controller) const;

uint64_t sampleWhenMidiNotePressed(int note) const;
uint64_t sampleWhenMidiNoteReleased(int note) const;
uint64_t samplesSinceMidiNotePressed(int note) const;
uint64_t samplesSinceMidiNoteReleased(int note) const;

int midiNotePressCount() const;
int midiNoteReleaseCount() const;
int midiVoiceCount() const;
int midiNotePressOrder(int index) const;
int midiNoteReleaseOrder(int index) const;
int midiVoiceNote(int index) const;

float midiNoteFrequency(int note) const;
float midiNotePhase(int note) const;
float midiNoteAdsr(int note, float attackSeconds, float decaySeconds,
                   float sustainLevel, float releaseSeconds) const;
MidiVoices midiVoices(int maximumVoices = 128) const;
```

All event times are stored and returned as unsigned 64-bit sample positions or sample durations. Floating-point conversion only happens when explicitly requesting frequency, phase, seconds, or an envelope amplitude.

Press, release, and voice orderings are newest-first. Each note appears at most once in each ordering. The voice ordering changes on note-on and retains released notes so release tails can continue; limit it with `midiVoices(n)` to implement fixed polyphony efficiently.

`midiNotePhase(note)` uses 12-tone equal temperament with A4 = 440 Hz and returns `[0, 1)`. A custom compile-time tuning object may map a MIDI note to frequency in hertz:

```cpp
struct JustIntonation {
    float operator()(int note) const {
        // Replace with the application's desired note-to-hertz mapping.
        return note == 69 ? 440.0f : 440.0f * powf(2.0f, (note - 69) / 12.0f);
    }
};

const float phase = wav.midiNotePhase(69, JustIntonation{});
```

Use a tuning functor rather than a raw function pointer; indirect calls are prohibited by the shader safety model.

ADSR attack, decay, and release are measured in seconds. Sustain is a `[0, 1]` amplitude ratio and is clamped. The returned amplitude is also clamped to `[0, 1]` and can be multiplied directly with an oscillator.

```cpp
float output = 0.0f;
for (const auto voice : wav.midiVoices(8)) {
    output += wav.sine(voice.phase())
        * voice.adsr(0.01f, 0.15f, 0.7f, 0.25f);
}
```

`WaviateCore` also exposes pure waveform, envelope, and noise helpers including `adsr`, `sine`, `saw`, `square`, `pulse`, `triangle`, `semicircle`, `perlin`, `simplex`, `voronoi`, `turbulence`, and `ridgedMulti`. These functions are all `const`.

## Sample context

```cpp
int channel() const;
int sampleInBlock() const;
int blockSize() const;
int inputChannelCount() const;
int sideChainChannelCount() const;
int channelCount() const;
float sampleRate() const;
uint64_t samplesSinceAppStart() const;
bool isSustainDown() const;

float incomingSample(int channel = -1, int sample = -1) const;
float sideChainSample(int channel = 0, int sample = -1) const;
float currentSample(int channel = -1, int sample = -1) const;

void setCurrentSample(float value, int channel = -1, int sample = -1);
```

For parameters defaulting to `-1`, WaviateScript uses the channel/sample currently being processed. `setCurrentSample` is non-const because it changes current-frame output state. That state may be observed by later sequential work in the same frame, but must not be retained as general-purpose storage beyond the engine-defined frame lifetime.

Example:

```cpp
float SampleProcess(WaviateSample& wav) {
    if (wav.isMidiNoteOn(60))
        return wav.incomingSample() * 0.5f;

    return wav.currentSample();
}
```

## Frequency context

```cpp
int channel() const;
int bin() const;
int totalBinCount() const;
int sampleWidth() const;
int channelCount() const;
float sampleRate() const;
uint64_t samplesSinceAppStart() const;

WaviateComplex incomingSample(int channel = -1, int bin = -1) const;
WaviateComplex currentSample(int channel = -1, int bin = -1) const;
WaviateComplex sideChainSample(int channel = 0, int bin = -1) const;
```

Frequency values retain phase and magnitude information. `WaviateComplex` (also available as `fcomplex`; `dcomplex` is the double-precision form) supports arithmetic operators and accessor functions:

```cpp
float real() const;
float imaginary() const;
float norm() const;
float magnitude() const;
float phase() const;
WaviateComplex conjugate() const;
```

Pure complex helpers are available under `waviate_complex` in shader code, including `abs`, `arg`, `conj`, `polar`, `exp`, `log`, `pow`, `sin`, `cos`, and `tan`.

```cpp
WaviateComplex FrequencyProcess(WaviateFrequency& wav) {
    const auto value = wav.incomingSample();
    return value * 0.75f;
}
```

## Language status

The runtime retains a C ABI internally so additional language frontends can interoperate in the future. That ABI is an implementation boundary, not a first-class user API. Rust shader support is neither currently supported nor under active development; it remains a stretch goal.
