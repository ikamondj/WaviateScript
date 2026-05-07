#pragma once

#include "Waviate.h"

#ifndef WAVIATE_SCRIPT_CPP_API_DEFINED
#define WAVIATE_SCRIPT_CPP_API_DEFINED

extern "C" void* __waviate_internal_arena_allocate(uint64_t sizeBytes, uint64_t alignmentBytes) noexcept;
extern "C" uint64_t __waviate_internal_arena_generation() noexcept;
extern "C" void waviate_fuel_trap() noexcept;

template <typename T>
T* waviateArenaAllocateArray(uint64_t count) {
    static_assert(__is_trivially_copyable(T) && __is_trivially_destructible(T),
        "Waviate arena containers only support trivially copyable/destructible element types");

    if (count == 0)
        return nullptr;

    constexpr uint64_t elementSize = static_cast<uint64_t>(sizeof(T));
    if (elementSize != 0 && count > (~0ULL / elementSize)) {
        waviate_fuel_trap();
        return nullptr;
    }

    const uint64_t byteCount = count * elementSize;
    auto* memory = static_cast<T*>(__waviate_internal_arena_allocate(byteCount, static_cast<uint64_t>(alignof(T))));
    if (memory != nullptr)
        __builtin_memset(memory, 0, byteCount);

    return memory;
}

static uint64_t waviateNextCapacity(uint64_t current, uint64_t required) {
    uint64_t capacity = current > 0 ? current : 4;
    while (capacity < required) {
        if (capacity > (~0ULL / 2ULL)) {
            waviate_fuel_trap();
            return required;
        }
        capacity *= 2;
    }
    return capacity;
}

template <typename T>
class WaviateArray {
public:
    static WaviateArray create(uint64_t count) {
        WaviateArray array;
        array.items = waviateArenaAllocateArray<T>(count);
        array.itemCount = array.items != nullptr ? count : 0;
        array.generation = __waviate_internal_arena_generation();
        return array;
    }

    bool isValid() const {
        return items != nullptr && generation == __waviate_internal_arena_generation();
    }

    uint64_t size() const { return isValid() ? itemCount : 0; }
    bool empty() const { return size() == 0; }

    T get(uint64_t index, T fallback = T{}) const {
        if (!isValid() || index >= itemCount) {
            waviate_fuel_trap();
            return fallback;
        }
        return items[index];
    }

    bool set(uint64_t index, T value) const {
        if (!isValid() || index >= itemCount) {
            waviate_fuel_trap();
            return false;
        }
        items[index] = value;
        return true;
    }

private:
    T* items = nullptr;
    uint64_t itemCount = 0;
    uint64_t generation = 0;
};

template <typename T>
class WaviateVector {
public:
    static WaviateVector create(uint64_t initialCapacity = 0) {
        WaviateVector vector;
        vector.generation = __waviate_internal_arena_generation();
        if (initialCapacity > 0)
            vector.reserve(initialCapacity);
        return vector;
    }

    bool isValid() const {
        return generation == __waviate_internal_arena_generation();
    }

    uint64_t size() const { return isValid() ? itemCount : 0; }
    uint64_t capacity() const { return isValid() ? itemCapacity : 0; }
    bool empty() const { return size() == 0; }

    bool reserve(uint64_t requestedCapacity) {
        if (!isValid())
            return false;
        if (requestedCapacity <= itemCapacity)
            return true;

        auto* replacement = waviateArenaAllocateArray<T>(requestedCapacity);
        if (replacement == nullptr)
            return false;

        if (items != nullptr && itemCount > 0)
            __builtin_memcpy(replacement, items, itemCount * static_cast<uint64_t>(sizeof(T)));

        items = replacement;
        itemCapacity = requestedCapacity;
        return true;
    }

    bool pushBack(T value) {
        if (!isValid())
            return false;
        if (itemCount >= itemCapacity && !reserve(waviateNextCapacity(itemCapacity, itemCount + 1)))
            return false;
        items[itemCount++] = value;
        return true;
    }

    T get(uint64_t index, T fallback = T{}) const {
        if (!isValid() || index >= itemCount) {
            waviate_fuel_trap();
            return fallback;
        }
        return items[index];
    }

    bool set(uint64_t index, T value) const {
        if (!isValid() || index >= itemCount) {
            waviate_fuel_trap();
            return false;
        }
        items[index] = value;
        return true;
    }

    void clear() {
        if (isValid())
            itemCount = 0;
    }

private:
    T* items = nullptr;
    uint64_t itemCount = 0;
    uint64_t itemCapacity = 0;
    uint64_t generation = 0;
};

class WaviateString {
public:
    static WaviateString create(uint64_t initialCapacity = 0) {
        WaviateString string;
        if (initialCapacity == ~0ULL) {
            waviate_fuel_trap();
            return string;
        }
        string.characters = WaviateVector<char>::create(initialCapacity + 1);
        string.characters.pushBack('\0');
        return string;
    }

    uint64_t length() const {
        const auto size = characters.size();
        return size > 0 ? size - 1 : 0;
    }

    bool empty() const { return length() == 0; }

    char charAt(uint64_t index, char fallback = '\0') const {
        if (index >= length()) {
            waviate_fuel_trap();
            return fallback;
        }
        return characters.get(index, fallback);
    }

    bool appendChar(char c) {
        const auto len = length();
        if (!characters.set(len, c))
            return false;
        return characters.pushBack('\0');
    }

    bool append(const char* text) {
        if (text == nullptr)
            return false;
        for (uint64_t i = 0; text[i] != '\0'; ++i)
            if (!appendChar(text[i]))
                return false;
        return true;
    }

private:
    WaviateVector<char> characters;
};

template <typename K, typename V>
class WaviateMap {
public:
    struct Entry {
        K key;
        V value;
        bool occupied;
    };

    static WaviateMap create(uint64_t initialCapacity = 0) {
        WaviateMap map;
        map.entries = WaviateVector<Entry>::create(initialCapacity > 0 ? initialCapacity : 4);
        return map;
    }

    bool isValid() const { return entries.isValid(); }
    uint64_t size() const { return isValid() ? entryCount : 0; }

    bool put(K key, V value) {
        if (!isValid()) {
            waviate_fuel_trap();
            return false;
        }

        for (uint64_t i = 0; i < entries.size(); ++i) {
            auto entry = entries.get(i);
            if (entry.occupied && entry.key == key) {
                entry.value = value;
                return entries.set(i, entry);
            }
        }

        if (entryCount >= entries.capacity() && !entries.reserve(waviateNextCapacity(entries.capacity(), entryCount + 1)))
            return false;

        Entry entry { key, value, true };
        if (!entries.pushBack(entry))
            return false;

        ++entryCount;
        return true;
    }

    bool tryGet(K key, V& outValue) const {
        if (!isValid()) {
            waviate_fuel_trap();
            return false;
        }

        for (uint64_t i = 0; i < entries.size(); ++i) {
            const auto entry = entries.get(i);
            if (entry.occupied && entry.key == key) {
                outValue = entry.value;
                return true;
            }
        }
        return false;
    }

    V getOrDefault(K key, V fallback = V{}) const {
        V value = fallback;
        return tryGet(key, value) ? value : fallback;
    }

private:
    WaviateVector<Entry> entries;
    uint64_t entryCount = 0;
};

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
        float threshold = 0.5f;
        int band = 0;
        while (remaining <= threshold && band < 24) {
            threshold *= 0.5f;
            ++band;
        }
        return (band & 1) == 0 ? 1.0f : -1.0f;
    }

    float perlin(float x, float min = 0.0f, float max = 1.0f) const {
        const int cell = fastFloor(x);
        const float t = x - static_cast<float>(cell);
        const float u = fade(t);
        const float a = gradient(cell) * t;
        const float b = gradient(cell + 1) * (t - 1.0f);
        const float value = clamp01(0.5f + lerp(a, b, u));
        return min + value * (max - min);
    }

    float simplex(float x, float min = 0.0f, float max = 1.0f) const {
        const int cell = fastFloor(x);
        const float x0 = x - static_cast<float>(cell);
        const float x1 = x0 - 1.0f;

        float t0 = 1.0f - x0 * x0;
        t0 *= t0;
        const float n0 = t0 * t0 * gradient(cell) * x0;

        float t1 = 1.0f - x1 * x1;
        t1 *= t1;
        const float n1 = t1 * t1 * gradient(cell + 1) * x1;

        const float value = clamp01(0.5f + 4.0f * (n0 + n1));
        return min + value * (max - min);
    }

    float voronoi(float x, float min = 0.0f, float max = 1.0f) const {
        const int cell = fastFloor(x);
        float nearest = 2.0f;

        for (int offset = -1; offset <= 1; ++offset) {
            const int neighbour = cell + offset;
            const float feature = static_cast<float>(neighbour) + hash01(neighbour);
            nearest = minValue(nearest, wavAbs(x - feature));
        }

        const float value = clamp01(1.0f - nearest);
        return min + value * (max - min);
    }

    float turbulence(float x, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f, float min = 0.0f, float max = 1.0f) const {
        float sum = 0.0f;
        float amplitude = 0.5f;
        float frequency = 1.0f;
        float normalizer = 0.0f;
        const int count = clampInt(octaves, 1, 8);

        for (int i = 0; i < count; ++i) {
            sum += amplitude * wavAbs(2.0f * perlin(x * frequency, 0.0f, 1.0f) - 1.0f);
            normalizer += amplitude;
            frequency *= maxValue(0.0001f, lacunarity);
            amplitude *= clamp01(gain);
        }

        const float value = normalizer > 0.0f ? clamp01(sum / normalizer) : 0.0f;
        return min + value * (max - min);
    }

    float ridgedMulti(float x, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f, float min = 0.0f, float max = 1.0f) const {
        float sum = 0.0f;
        float amplitude = 0.5f;
        float frequency = 1.0f;
        float normalizer = 0.0f;
        const int count = clampInt(octaves, 1, 8);

        for (int i = 0; i < count; ++i) {
            const float ridge = 1.0f - wavAbs(2.0f * perlin(x * frequency, 0.0f, 1.0f) - 1.0f);
            sum += amplitude * ridge * ridge;
            normalizer += amplitude;
            frequency *= maxValue(0.0001f, lacunarity);
            amplitude *= clamp01(gain);
        }

        const float value = normalizer > 0.0f ? clamp01(sum / normalizer) : 0.0f;
        return min + value * (max - min);
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

    template <typename T>
    WaviateArray<T> newArray(uint64_t size) const { return WaviateArray<T>::create(size); }

    template <typename T>
    WaviateVector<T> newVector(uint64_t capacity = 0) const { return WaviateVector<T>::create(capacity); }

    WaviateString newString(uint64_t capacity = 0) const { return WaviateString::create(capacity); }

    template <typename K, typename V>
    WaviateMap<K, V> newMap(uint64_t capacity = 0) const { return WaviateMap<K, V>::create(capacity); }

protected:
    WaviateCore(float sampleRateIn, uint64_t samplesSinceAppStartIn)
        : coreSampleRate(sampleRateIn), coreSamplesSinceAppStart(samplesSinceAppStartIn) {}

    static bool isValidIndex(int index, int count) {
        return index >= 0 && index < count;
    }

private:
    static constexpr float pi = 3.14159265358979323846f;
    static constexpr float twoPi = 6.28318530717958647692f;
    static constexpr float halfPi = 1.57079632679489661923f;
    static constexpr float invTwoPi = 0.15915494309189533577f;
    static constexpr float oneThird = 0.33333333333333333333f;

    static float wavAbs(float x) { return x < 0.0f ? -x : x; }
    static float reduceRadians(float x) {
        if (x > 2147483520.0f || x < -2147483520.0f)
            return 0.0f;

        const int turns = static_cast<int>(x * invTwoPi + (x >= 0.0f ? 0.5f : -0.5f));
        x -= static_cast<float>(turns) * twoPi;
        if (x > pi)
            x -= twoPi;
        else if (x < -pi)
            x += twoPi;
        return x;
    }
    static float wavSin(float x) {
        x = reduceRadians(x);
        const float x2 = x * x;
        return x * (1.0f + x2 * (-0.1666666716f + x2 * (0.0083333310f + x2 * -0.0001984090f)));
    }
    static float wavCos(float x) {
        return wavSin(x + halfPi);
    }
    static float wavTan(float x) {
        const float c = wavCos(x);
        if (wavAbs(c) < 0.0001f)
            return wavSin(x) >= 0.0f ? 10000.0f : -10000.0f;
        return wavSin(x) / c;
    }
    static float wavFloor(float x) {
        if (x >= 2147483520.0f || x <= -2147483520.0f)
            return x;

        const int i = static_cast<int>(x);
        const float f = static_cast<float>(i);
        return f > x ? f - 1.0f : f;
    }
    static float wavSqrt(float x) {
        if (x <= 0.0f)
            return 0.0f;

        float guess = x > 1.0f ? x : 1.0f;
        for (int i = 0; i < 6; ++i)
            guess = 0.5f * (guess + x / guess);
        return guess;
    }

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
