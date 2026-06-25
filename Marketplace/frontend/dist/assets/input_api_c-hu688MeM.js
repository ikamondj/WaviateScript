const n=`\r
\`\`\`cpp\r
[include](../src/waviate/WaviateInput.h)\r
\`\`\`\r
\`\`\`cpp\r
float sample_process(const WaviateSampleInput*, WaviateSampleStateWriter*);\r
\r
struct WaviateSampleInput {\r
    uint64_t samplesSinceAppStart;\r
    int32_t positionInBlock;\r
    int32_t blockSize;\r
    int32_t inputChannelCount;\r
    int32_t sideChainChannelCount;\r
    int32_t sampleMemoryCount;\r
    int32_t outputChannelCount;\r
    uint8_t outputChannel;\r
\r
    uint8_t* midiNoteOn;\r
    uint8_t* midiCCValue;\r
    uint64_t* sampleWhenMidiNoteOn;\r
    uint64_t* sampleWhenMidiNoteOff;\r
    uint64_t* sampleWhenCCValueChanged;\r
\r
    int32_t controllerCount;\r
    uint64_t* controllerButtonMask;\r
    uint64_t* sampleWhenControllerButtonChanged;\r
    int32_t controllerButtonCount;\r
    float* controllerAxisValue;\r
    uint64_t* sampleWhenControllerAxisChanged;\r
    int32_t controllerAxisCount;\r
\r
    int32_t* oscInts;\r
    uint32_t* oscColors\r
    float* oscFloats;\r
    char** oscStrings;\r
\r
    float sampleRate;\r
    float** previousSamples;\r
    float** inputDeviceSamples;\r
    float** inputSideChainSamples;\r
    float** currentSampleData;\r
};\r
\`\`\`\r
\r
## \`WaviateSampleInput\` field reference (alphabetical)\r
\r
> **Lifetime & ownership**\r
>\r
> All pointer fields reference **engine-owned memory**. Plugins must not free, reallocate, or retain these pointers beyond the current \`sample_process\` invocation (or beyond the current block, if block-lifetime is guaranteed by the engine).\r
>\r
> This struct is a lightweight header; its size is fixed by the ABI and does not scale with \`blockSize\` or channel counts.\r
\r
---\r
\r
### \`blockSize\` (\`int32_t\`)\r
Number of samples in the current processing block. \r
\r
---\r
\r
### \`currentSampleData\` (\`float**\`)\r
Audio buffer containing sample output data for each channel at the current sample-stage state of execution. Because the sample shader always runs before the frequency shader within a plugin instance, this buffer initializes with all zeroes at the start of the sample stage. It does not contain output from a frequency shader in the same plugin instance. Indexed as \`[outputChannel][sampleIndex]\` with dimensions \`outputChannelCount × blockSize\`.
\r
---\r
\r
### \`inputChannelCount\` (\`int32_t\`)\r
Number of input channels.\r
\r
---\r
\r
### \`inputDeviceSamples\` (\`float**\`)\r
Read-only block of input samples from the primary input device. Indexed as \`[inputChannel][sampleIndex]\` with dimensions \`inputChannelCount × blockSize\`.\r
\r
---\r
\r
### \`inputSideChainSamples\` (\`float**\`)\r
Read-only block of sidechain input samples. Indexed as \`[sideChainChannel][sampleIndex]\` with dimensions \`sideChainChannelCount × blockSize\`.\r
\r
---\r
\r
### \`midiCCValue\` (\`uint8_t*\`)\r
Array of 128 MIDI Control Change values indexed by CC number. Values are \`0–127\`.\r
\r
---\r
\r
### \`midiNoteOn\` (\`uint8_t*\`)\r
Array of 128 MIDI note states indexed by MIDI note number. Values are \`0\` (off) or \`1\` (on).\r
\r
---\r
\r
### \`outputChannel\` (\`uint8_t\`)\r
Index of the output channel currently being processed. Values range from \`0\` to \`outputChannelCount - 1\`. Meaningful when processing is invoked per-channel.\r
\r
---\r
\r
### \`outputChannelCount\` (\`int32_t\`)\r
Total number of output channels available to the processor.\r
\r
---\r
\r
### \`positionInBlock\` (\`int32_t\`)\r
Index of the current sample within the block when \`sample_process\` is invoked per-sample. Values range from \`0\` to \`blockSize - 1\`.\r
\r
---\r
\r
### \`previousSamples\` (\`float**\`)\r
Read-only history buffer providing access to past output samples. Indexed as \`[outputChannel][samplesAgo]\` with dimensions \`outputChannelCount × sampleMemoryCount\`, where index \`0\` refers to the most recent past sample.\r
\r
---\r
\r
### \`sampleMemoryCount\` (\`int32_t\`)\r
Number of historical samples available per output channel in \`previousSamples\`.\r
\r
---\r
\r
### \`sampleRate\` (\`float\`)\r
Audio sample rate in Hertz (e.g. \`44100.0\`, \`48000.0\`).\r
\r
---\r
\r
### \`sampleWhenCCValueChanged\` (\`uint64_t*\`)\r
Array of 128 sample counters indexed by MIDI Control Change number. Each entry represents the global app sample number when that control last changed value. Useful in calculating the time since a midi control event.\r
\r
---\r
\r
### \`sampleWhenMidiNoteOff\` (\`uint64_t*\`)\r
Array of 128 sample counters indexed by MIDI note number. Each entry represents the global app sample number when that midi note last turned off. Useful when calculating the time since a midi note shut off (often for in release envelopes).\r
\r
---\r
\r
### \`sampleWhenMidiNoteOn\` (\`uint64_t*\`)\r
Array of 128 sample counters indexed by MIDI note number. Each entry represents the global app sample number when that midi note last turned off. Useful when calculating the time since a midi note shut off (often for in release envelopes).\r
\r
---\r
\r
### \`samplesSinceAppStart\` (\`uint64_t\`)\r
Monotonically increasing absolute sample counter since the engine or application started. Useful as a stable timebase for deterministic modulation and scheduling. Can be compared to sampleWhenMidiNoteOn for example to get duration since a note turned on.\r
\r
---\r
\r
### \`sideChainChannelCount\` (\`int32_t\`)\r
Number of sidechain channels available in \`inputSideChainSamples\`.
`;export{n as default};
