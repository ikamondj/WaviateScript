const n=`\`\`\`cpp\r
float sample_process(const WaviateSampleIn&);\r
\r
class WaviateSampleIn {\r
    private:\r
        ...\r
    public:\r
        bool note(int noteId);\r
        uint8_t cc(int noteId);\r
        double cc01(int noteId);\r
        bool ccOn(int noteId);\r
        uint64_t currSample();\r
        uint64_t noteOnDuration(int noteId);\r
        uint64_t noteOffDuration(int noteId);\r
        uint64_t ccSamples(int noteId);\r
        double samplesToSeconds(uint64_t samples);\r
        uint64_t secondsToSamples(double seconds);\r
        float sampleRateHz();\r
        float sampleRateKHz();\r
};  \r
\r
\`\`\``;export{n as default};
