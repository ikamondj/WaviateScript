#pragma once

#include <cmath>
#include "Waviate.h"

#ifndef WAVIATE_SCRIPT_CPP_API_DEFINED
#define WAVIATE_SCRIPT_CPP_API_DEFINED

class WaviateCore {
public:
    float getSeconds() const { return samplesToSeconds(coreSamplesSinceAppStart); }
    float samplesToSeconds(uint64_t samples) const {
        return coreSampleRate > 0.0f ? static_cast<float>(samples) / coreSampleRate : 0.0f;
    }
    uint64_t secondsToSamples(float seconds) const {
        return seconds > 0.0f && coreSampleRate > 0.0f
            ? static_cast<uint64_t>(seconds * coreSampleRate + 0.5f)
            : 0ULL;
    }
    float sampleRateHz() const { return coreSampleRate; }
    float sampleRateKHz() const { return coreSampleRate * 0.001f; }

    float phase(float x) const { return fract(x); }
    float sine(float x) const { return wavSin(twoPi * phase(x)); }
    float saw(float x) const { return 2.0f * phase(x) - 1.0f; }
    float square(float x) const { return phase(x) < 0.5f ? 1.0f : -1.0f; }
    float pulse(float x, float width = 0.5f) const { return phase(x) < clamp01(width) ? 1.0f : -1.0f; }
    float triangle(float x) const { return 1.0f - 4.0f * wavAbs(phase(x) - 0.5f); }
    float semicircle(float x) const {
        const float centered = 2.0f * phase(x) - 1.0f;
        return 2.0f * wavSqrt(maxValue(0.0f, 1.0f - centered * centered)) - 1.0f;
    }
    float sawTan(float x) const { return wrapSigned(wavTan(pi * (phase(x) - 0.5f))); }
    float triangleTan(float x) const { return foldSigned(wavTan(pi * (phase(x) - 0.5f))); }
    float strongSine(float x) const { return 0.75f * (sine(x) + oneThird * sine(3.0f * x)); }
    float fractalSquare(float x) const {
        const float p = phase(x);
        if (p < 0.5f)
            return 1.0f;

        const float remaining = maxValue(0.00000011920928955f, 1.0f - p);
        const int band = static_cast<int>(wavFloor(-wavLog2(remaining)));
        return (band & 1) == 0 ? 1.0f : -1.0f;
    }

    float perlin(float x) const {
        const int cell = fastFloor(x);
        const float t = x - static_cast<float>(cell);
        const float u = fade(t);
        const float a = gradient(cell) * t;
        const float b = gradient(cell + 1) * (t - 1.0f);
        return clamp01(0.5f + lerp(a, b, u));
    }

    float simplex(float x) const {
        const int cell = fastFloor(x);
        const float x0 = x - static_cast<float>(cell);
        const float x1 = x0 - 1.0f;

        float t0 = 1.0f - x0 * x0;
        t0 *= t0;
        const float n0 = t0 * t0 * gradient(cell) * x0;

        float t1 = 1.0f - x1 * x1;
        t1 *= t1;
        const float n1 = t1 * t1 * gradient(cell + 1) * x1;

        return clamp01(0.5f + 4.0f * (n0 + n1));
    }

    float voronoi(float x) const {
        const int cell = fastFloor(x);
        float nearest = 2.0f;

        for (int offset = -1; offset <= 1; ++offset) {
            const int neighbour = cell + offset;
            const float feature = static_cast<float>(neighbour) + hash01(neighbour);
            nearest = minValue(nearest, wavAbs(x - feature));
        }

        return clamp01(1.0f - nearest);
    }

    float turbulence(float x, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f) const {
        float sum = 0.0f;
        float amplitude = 0.5f;
        float frequency = 1.0f;
        float normalizer = 0.0f;
        const int count = clampInt(octaves, 1, 8);

        for (int i = 0; i < count; ++i) {
            sum += amplitude * wavAbs(2.0f * perlin(x * frequency) - 1.0f);
            normalizer += amplitude;
            frequency *= maxValue(0.0001f, lacunarity);
            amplitude *= clamp01(gain);
        }

        return normalizer > 0.0f ? clamp01(sum / normalizer) : 0.0f;
    }

    float ridgedMulti(float x, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f) const {
        float sum = 0.0f;
        float amplitude = 0.5f;
        float frequency = 1.0f;
        float normalizer = 0.0f;
        const int count = clampInt(octaves, 1, 8);

        for (int i = 0; i < count; ++i) {
            const float ridge = 1.0f - wavAbs(2.0f * perlin(x * frequency) - 1.0f);
            sum += amplitude * ridge * ridge;
            normalizer += amplitude;
            frequency *= maxValue(0.0001f, lacunarity);
            amplitude *= clamp01(gain);
        }

        return normalizer > 0.0f ? clamp01(sum / normalizer) : 0.0f;
    }

    float adsr(float attack, float decay, float sustain, float release, float t) const {
        const float a = maxValue(0.0f, attack);
        const float d = maxValue(0.0f, decay);
        const float s = clamp01(sustain);
        const float r = maxValue(0.0f, release);

        if (t < 0.0f)
            return r > 0.0f ? s * (1.0f - clamp01(-t / r)) : 0.0f;
        if (a > 0.0f && t < a)
            return clamp01(t / a);
        if (d > 0.0f && t < a + d)
            return lerp(1.0f, s, (t - a) / d);

        return s;
    }
    float ADSR(float attack, float decay, float sustain, float release, float t) const {
        return adsr(attack, decay, sustain, release, t);
    }

protected:
    WaviateCore(float sampleRateIn, uint64_t samplesSinceAppStartIn)
        : coreSampleRate(sampleRateIn), coreSamplesSinceAppStart(samplesSinceAppStartIn) {}

    static bool isValidIndex(int index, int count) {
        return index >= 0 && index < count;
    }

private:
    static constexpr float pi = 3.14159265358979323846f;
    static constexpr float twoPi = 6.28318530717958647692f;
    static constexpr float oneThird = 0.33333333333333333333f;

    static float wavSin(float x) { return static_cast<float>(std::sin(x)); }
    static float wavTan(float x) { return static_cast<float>(std::tan(x)); }
    static float wavFloor(float x) { return static_cast<float>(std::floor(x)); }
    static float wavAbs(float x) { return static_cast<float>(std::fabs(x)); }
    static float wavSqrt(float x) { return static_cast<float>(std::sqrt(x)); }
    static float wavLog2(float x) { return static_cast<float>(std::log2(x)); }

    static float minValue(float a, float b) { return a < b ? a : b; }
    static float maxValue(float a, float b) { return a > b ? a : b; }
    static int clampInt(int value, int low, int high) {
        return value < low ? low : (value > high ? high : value);
    }
    static float clamp01(float x) {
        return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
    }
    static float fract(float x) { return x - wavFloor(x); }
    static int fastFloor(float x) {
        const int i = static_cast<int>(x);
        return static_cast<float>(i) > x ? i - 1 : i;
    }
    static float lerp(float a, float b, float t) { return a + (b - a) * t; }
    static float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    static float wrapSigned(float x) { return 2.0f * fract(0.5f * (x + 1.0f)) - 1.0f; }
    static float foldSigned(float x) {
        const float wrapped = fract(0.25f * (x + 1.0f));
        const float folded = wrapped < 0.5f ? wrapped : 1.0f - wrapped;
        return 4.0f * folded - 1.0f;
    }
    static uint32_t hashBits(int cell) {
        uint32_t h = static_cast<uint32_t>(cell);
        h ^= h >> 16;
        h *= 0x7feb352dU;
        h ^= h >> 15;
        h *= 0x846ca68bU;
        h ^= h >> 16;
        return h;
    }
    static float hash01(int cell) {
        return static_cast<float>(hashBits(cell) & 0x00ffffffU) * (1.0f / 16777215.0f);
    }
    static float gradient(int cell) {
        return (hashBits(cell) & 1U) != 0U ? 1.0f : -1.0f;
    }

    float coreSampleRate;
    uint64_t coreSamplesSinceAppStart;
};

class WaviateSample final : public WaviateCore {
public:
    WaviateSample(const WaviateSampleInput* inputIn, WaviateSampleStateWriter* writerIn)
        : WaviateCore(inputIn != nullptr ? inputIn->sampleRate : 0.0f,
              inputIn != nullptr ? inputIn->samplesSinceAppStart : 0ULL),
          input(inputIn), writer(writerIn) {}

    int getChannel() const { return input != nullptr ? static_cast<int>(input->channel) : 0; }
    int getSampleInBlock() const { return input != nullptr ? input->sampleInBlock : 0; }
    int getBlockSize() const { return input != nullptr ? input->blockSize : 0; }
    int getInputChannelCount() const { return input != nullptr ? input->inputChannelCount : 0; }
    int getSideChainChannelCount() const { return input != nullptr ? input->sideChainChannelCount : 0; }
    int getChannelCount() const { return input != nullptr ? input->channelCount : 0; }
    float getSampleRate() const { return input != nullptr ? input->sampleRate : 0.0f; }
    uint64_t getSamplesSinceAppStart() const { return input != nullptr ? input->samplesSinceAppStart : 0ULL; }
    bool isSustainDown() const { return input != nullptr && input->sustain; }

    float getIncomingSample(int channel = -1, int sample = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : getChannel();
        const int resolvedSample = sample >= 0 ? sample : getSampleInBlock();
        if (input == nullptr
            || input->inputDeviceSamples == nullptr
            || !isValidIndex(resolvedChannel, input->inputChannelCount)
            || !isValidIndex(resolvedSample, input->blockSize)
            || input->inputDeviceSamples[resolvedChannel] == nullptr) {
            return 0.0f;
        }

        return input->inputDeviceSamples[resolvedChannel][resolvedSample];
    }

    float getSideChainSample(int channel = 0, int sample = -1) const {
        const int resolvedSample = sample >= 0 ? sample : getSampleInBlock();
        if (input == nullptr
            || input->inputSideChainSamples == nullptr
            || !isValidIndex(channel, input->sideChainChannelCount)
            || !isValidIndex(resolvedSample, input->blockSize)
            || input->inputSideChainSamples[channel] == nullptr) {
            return 0.0f;
        }

        return input->inputSideChainSamples[channel][resolvedSample];
    }

    float getCurrentSample(int channel = -1, int sample = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : getChannel();
        const int resolvedSample = sample >= 0 ? sample : getSampleInBlock();
        if (input == nullptr
            || input->currentSampleData == nullptr
            || !isValidIndex(resolvedChannel, input->channelCount)
            || !isValidIndex(resolvedSample, input->blockSize)
            || input->currentSampleData[resolvedChannel] == nullptr) {
            return 0.0f;
        }

        return input->currentSampleData[resolvedChannel][resolvedSample];
    }

    void setCurrentSample(float value, int channel = -1, int sample = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : getChannel();
        const int resolvedSample = sample >= 0 ? sample : getSampleInBlock();
        if (input == nullptr
            || input->currentSampleData == nullptr
            || !isValidIndex(resolvedChannel, input->channelCount)
            || !isValidIndex(resolvedSample, input->blockSize)
            || input->currentSampleData[resolvedChannel] == nullptr) {
            return;
        }

        input->currentSampleData[resolvedChannel][resolvedSample] = value;
    }

    bool isMidiNoteOn(int note) const {
        return input != nullptr
            && input->midiNoteOn != nullptr
            && isValidIndex(note, 128)
            && input->midiNoteOn[note] != 0;
    }

    uint8_t getMidiCCValue(int controller) const {
        if (input == nullptr || input->midiCCValue == nullptr || !isValidIndex(controller, 128))
            return 0;

        return input->midiCCValue[controller];
    }

private:
    const WaviateSampleInput* input;
    WaviateSampleStateWriter* writer;
};

class WaviateFrequency final : public WaviateCore {
public:
    WaviateFrequency(const WaviateFrequencyInput* inputIn, WaviateFrequencyStateWriter* writerIn)
        : WaviateCore(inputIn != nullptr ? inputIn->sampleRate : 0.0f,
              inputIn != nullptr ? inputIn->samplesSinceAppStart : 0ULL),
          input(inputIn), writer(writerIn) {}

    int getChannel() const { return input != nullptr ? static_cast<int>(input->channel) : 0; }
    int getBin() const { return input != nullptr ? input->bin : 0; }
    int getTotalBinCount() const { return input != nullptr ? input->totalBinCount : 0; }
    int getSampleWidth() const { return input != nullptr ? input->sampleWidth : 0; }
    int getChannelCount() const { return input != nullptr ? input->channelCount : 0; }
    float getSampleRate() const { return input != nullptr ? input->sampleRate : 0.0f; }
    uint64_t getSamplesSinceAppStart() const { return input != nullptr ? input->samplesSinceAppStart : 0ULL; }

    WaviateComplex getIncomingSample(int channel = -1, int bin = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : getChannel();
        const int resolvedBin = bin >= 0 ? bin : getBin();
        if (input == nullptr
            || input->inputDeviceData == nullptr
            || !isValidIndex(resolvedChannel, input->channelCount)
            || !isValidIndex(resolvedBin, input->totalBinCount)
            || input->inputDeviceData[resolvedChannel] == nullptr) {
            return { 0.0f, 0.0f };
        }

        return input->inputDeviceData[resolvedChannel][resolvedBin];
    }

    WaviateComplex getCurrentSample(int channel = -1, int bin = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : getChannel();
        const int resolvedBin = bin >= 0 ? bin : getBin();
        if (input == nullptr
            || input->currentFrequencyData == nullptr
            || !isValidIndex(resolvedChannel, input->channelCount)
            || !isValidIndex(resolvedBin, input->totalBinCount)
            || input->currentFrequencyData[resolvedChannel] == nullptr) {
            return { 0.0f, 0.0f };
        }

        return input->currentFrequencyData[resolvedChannel][resolvedBin];
    }

    WaviateComplex getSideChainSample(int channel = 0, int bin = -1) const {
        const int resolvedBin = bin >= 0 ? bin : getBin();
        if (input == nullptr
            || input->inputSideChainFrequencyData == nullptr
            || !isValidIndex(channel, input->channelCount)
            || !isValidIndex(resolvedBin, input->totalBinCount)
            || input->inputSideChainFrequencyData[channel] == nullptr) {
            return { 0.0f, 0.0f };
        }

        return input->inputSideChainFrequencyData[channel][resolvedBin];
    }

private:
    const WaviateFrequencyInput* input;
    WaviateFrequencyStateWriter* writer;
};

#endif
