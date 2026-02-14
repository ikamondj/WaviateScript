```cpp
float sample_process(const WaviateSampleIn&);

class WaviateSampleIn {
    private:
        ...
    public:
        bool note(int noteId);
        uint8_t cc(int noteId);
        double cc01(int noteId);
        bool ccOn(int noteId);
        uint64_t currSample();
        uint64_t noteOnDuration(int noteId);
        uint64_t noteOffDuration(int noteId);
        uint64_t ccSamples(int noteId);
        double samplesToSeconds(uint64_t samples);
        uint64_t secondsToSamples(double seconds);
        float sampleRateHz();
        float sampleRateKHz();
};  

```