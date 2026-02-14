
```cpp
float sample_process(const WaviateSampleInput*, WaviateSampleStateWriter*);

struct WaviateSampleInput {
    uint64_t samplesSinceAppStart;
    int32_t positionInBlock;
    int32_t blockSize;
    int32_t inputChannelCount;
    int32_t sideChainChannelCount;
    int32_t sampleMemoryCount;
    int32_t outputChannelCount;
    uint8_t outputChannel; //0..outputChannelCount-1
    uint8_t* midiNoteOn;   // 0 or 1. length = 128
    uint8_t* midiCCValue;  // 0..127. length = 128
    uint64_t* sampleWhenMidiNoteOn; // length = 128
    uint64_t* sampleWhenMidiNoteOff; // length = 128
    uint64_t* sampleWhenCCValueChanged; // length = 128
    float sampleRate; //in hz
    float** previousSamples; //length = outputChannelCount X sampleMemoryCount
    float** inputDeviceSamples; //length = inputChannelCount X blockSize
    float** inputSideChainSamples; //length = sideChainChannelCount X blockSize
    float** currentSampleData; //length = outputChannelCount X blockSize
};
```

## `WaviateSampleInput` field reference (alphabetical)

> **Lifetime & ownership**
>
> All pointer fields reference **engine-owned memory**. Plugins must not free, reallocate, or retain these pointers beyond the current `sample_process` invocation (or beyond the current block, if block-lifetime is guaranteed by the engine).
>
> This struct is a lightweight header; its size is fixed by the ABI and does not scale with `blockSize` or channel counts.

---

### `blockSize` (`int32_t`)
Number of samples in the current processing block. 

---

### `currentSampleData` (`float**`)
Audio buffer containing sample output data for each channel at the current shader state of execution. If the sample shader is executing first, it will initialize with all zeroes. If the sample shader executes after a frequency shader, this will be populated with the sample output of the frequency shader step. Indexed as `[outputChannel][sampleIndex]` with dimensions `outputChannelCount × blockSize`.

---

### `inputChannelCount` (`int32_t`)
Number of input channels.

---

### `inputDeviceSamples` (`float**`)
Read-only block of input samples from the primary input device. Indexed as `[inputChannel][sampleIndex]` with dimensions `inputChannelCount × blockSize`.

---

### `inputSideChainSamples` (`float**`)
Read-only block of sidechain input samples. Indexed as `[sideChainChannel][sampleIndex]` with dimensions `sideChainChannelCount × blockSize`.

---

### `midiCCValue` (`uint8_t*`)
Array of 128 MIDI Control Change values indexed by CC number. Values are `0–127`.

---

### `midiNoteOn` (`uint8_t*`)
Array of 128 MIDI note states indexed by MIDI note number. Values are `0` (off) or `1` (on).

---

### `outputChannel` (`uint8_t`)
Index of the output channel currently being processed. Values range from `0` to `outputChannelCount - 1`. Meaningful when processing is invoked per-channel.

---

### `outputChannelCount` (`int32_t`)
Total number of output channels available to the processor.

---

### `positionInBlock` (`int32_t`)
Index of the current sample within the block when `sample_process` is invoked per-sample. Values range from `0` to `blockSize - 1`.

---

### `previousSamples` (`float**`)
Read-only history buffer providing access to past output samples. Indexed as `[outputChannel][samplesAgo]` with dimensions `outputChannelCount × sampleMemoryCount`, where index `0` refers to the most recent past sample.

---

### `sampleMemoryCount` (`int32_t`)
Number of historical samples available per output channel in `previousSamples`.

---

### `sampleRate` (`float`)
Audio sample rate in Hertz (e.g. `44100.0`, `48000.0`).

---

### `sampleWhenCCValueChanged` (`uint64_t*`)
Array of 128 sample counters indexed by MIDI Control Change number. Each entry represents the global app sample number when that control last changed value. Useful in calculating the time since a midi control event.

---

### `sampleWhenMidiNoteOff` (`uint64_t*`)
Array of 128 sample counters indexed by MIDI note number. Each entry represents the global app sample number when that midi note last turned off. Useful when calculating the time since a midi note shut off (often for in release envelopes).

---

### `sampleWhenMidiNoteOn` (`uint64_t*`)
Array of 128 sample counters indexed by MIDI note number. Each entry represents the global app sample number when that midi note last turned off. Useful when calculating the time since a midi note shut off (often for in release envelopes).

---

### `samplesSinceAppStart` (`uint64_t`)
Monotonically increasing absolute sample counter since the engine or application started. Useful as a stable timebase for deterministic modulation and scheduling. Can be compared to sampleWhenMidiNoteOn for example to get duration since a note turned on.

---

### `sideChainChannelCount` (`int32_t`)
Number of sidechain channels available in `inputSideChainSamples`.