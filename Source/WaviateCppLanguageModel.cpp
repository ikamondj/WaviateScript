/*
  ==============================================================================

    WaviateCppLanguageModel.cpp - Shared C++ shader API metadata.

  ==============================================================================
*/

#include "WaviateCppLanguageModel.h"

#include <algorithm>
#include <utility>

namespace waviate::language
{
namespace
{
ParameterSymbol param(std::string type, std::string name, std::string defaultValue = {})
{
    return { std::move(type), std::move(name), std::move(defaultValue) };
}

FieldSymbol field(std::string name, std::string type, std::string documentation = {})
{
    return { std::move(name), std::move(type), SymbolKind::Field, std::move(documentation) };
}

FieldSymbol typeSymbol(std::string name, std::string documentation = {})
{
    return { std::move(name), {}, SymbolKind::Type, std::move(documentation) };
}

FieldSymbol classSymbol(std::string name, std::string documentation = {})
{
    return { std::move(name), {}, SymbolKind::Class, std::move(documentation) };
}

FieldSymbol keyword(std::string name, std::string documentation = {})
{
    return { std::move(name), {}, SymbolKind::Keyword, std::move(documentation) };
}

FunctionSymbol function(std::string name,
                        std::string returnType,
                        std::vector<ParameterSymbol> parameters,
                        SymbolKind kind,
                        std::string documentation = {})
{
    return { std::move(name), std::move(returnType), std::move(parameters), kind, std::move(documentation) };
}
} // namespace

const std::vector<FunctionSymbol>& waviateCoreMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("getSeconds", "float", {}, SymbolKind::Method, "Seconds since app start."),
        function("samplesToSeconds", "float", { param("uint64_t", "samples") }, SymbolKind::Method, "Convert samples to seconds."),
        function("secondsToSamples", "uint64_t", { param("float", "seconds") }, SymbolKind::Method, "Convert seconds to samples."),
        function("sampleRateHz", "float", {}, SymbolKind::Method, "Sample rate in hertz."),
        function("sampleRateKHz", "float", {}, SymbolKind::Method, "Sample rate in kilohertz."),
        function("phase", "float", { param("float", "x") }, SymbolKind::Method, "Fractional phase in [0, 1)."),
        function("sine", "float", { param("float", "x") }, SymbolKind::Method, "Sine oscillator."),
        function("saw", "float", { param("float", "x") }, SymbolKind::Method, "Saw oscillator."),
        function("square", "float", { param("float", "x") }, SymbolKind::Method, "Square oscillator."),
        function("pulse", "float", { param("float", "x"), param("float", "width", "0.5f") }, SymbolKind::Method, "Pulse oscillator."),
        function("triangle", "float", { param("float", "x") }, SymbolKind::Method, "Triangle oscillator."),
        function("semicircle", "float", { param("float", "x") }, SymbolKind::Method, "Semicircle oscillator."),
        function("sawTan", "float", { param("float", "x") }, SymbolKind::Method, "Tangent-shaped saw oscillator."),
        function("triangleTan", "float", { param("float", "x") }, SymbolKind::Method, "Tangent-shaped triangle oscillator."),
        function("strongSine", "float", { param("float", "x") }, SymbolKind::Method, "Sine oscillator with harmonic weight."),
        function("fractalSquare", "float", { param("float", "x") }, SymbolKind::Method, "Fractal square oscillator."),
        function("perlin", "float", { param("float", "x") }, SymbolKind::Method, "Perlin noise."),
        function("simplex", "float", { param("float", "x") }, SymbolKind::Method, "Simplex noise."),
        function("voronoi", "float", { param("float", "x") }, SymbolKind::Method, "Voronoi noise."),
        function("turbulence", "float", { param("float", "x"), param("int", "octaves", "4"), param("float", "lacunarity", "2.0f"), param("float", "gain", "0.5f") }, SymbolKind::Method, "Turbulence noise."),
        function("ridgedMulti", "float", { param("float", "x"), param("int", "octaves", "4"), param("float", "lacunarity", "2.0f"), param("float", "gain", "0.5f") }, SymbolKind::Method, "Ridged multifractal noise."),
        function("adsr", "float", { param("float", "attack"), param("float", "decay"), param("float", "sustain"), param("float", "release"), param("float", "t") }, SymbolKind::Method, "ADSR envelope."),
        function("ADSR", "float", { param("float", "attack"), param("float", "decay"), param("float", "sustain"), param("float", "release"), param("float", "t") }, SymbolKind::Method, "ADSR envelope."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateSampleMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("getChannel", "int", {}, SymbolKind::Method, "Current channel."),
        function("getSampleInBlock", "int", {}, SymbolKind::Method, "Sample index in the current block."),
        function("getBlockSize", "int", {}, SymbolKind::Method, "Current block size."),
        function("getInputChannelCount", "int", {}, SymbolKind::Method, "Input channel count."),
        function("getSideChainChannelCount", "int", {}, SymbolKind::Method, "Sidechain channel count."),
        function("getChannelCount", "int", {}, SymbolKind::Method, "Output channel count."),
        function("getSampleRate", "float", {}, SymbolKind::Method, "Sample rate in hertz."),
        function("getSamplesSinceAppStart", "uint64_t", {}, SymbolKind::Method, "Samples since app start."),
        function("isSustainDown", "bool", {}, SymbolKind::Method, "Sustain pedal state."),
        function("getIncomingSample", "float", { param("int", "channel", "-1"), param("int", "sample", "-1") }, SymbolKind::Method, "Input sample."),
        function("getSideChainSample", "float", { param("int", "channel", "0"), param("int", "sample", "-1") }, SymbolKind::Method, "Sidechain sample."),
        function("getCurrentSample", "float", { param("int", "channel", "-1"), param("int", "sample", "-1") }, SymbolKind::Method, "Current output sample."),
        function("setCurrentSample", "void", { param("float", "value"), param("int", "channel", "-1"), param("int", "sample", "-1") }, SymbolKind::Method, "Write the current output sample."),
        function("isMidiNoteOn", "bool", { param("int", "note") }, SymbolKind::Method, "True when a MIDI note is held."),
        function("getMidiCCValue", "uint8_t", { param("int", "controller") }, SymbolKind::Method, "MIDI controller value."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateFrequencyMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("getChannel", "int", {}, SymbolKind::Method, "Current channel."),
        function("getBin", "int", {}, SymbolKind::Method, "Current frequency bin."),
        function("getTotalBinCount", "int", {}, SymbolKind::Method, "Total frequency bin count."),
        function("getSampleWidth", "int", {}, SymbolKind::Method, "FFT window size."),
        function("getChannelCount", "int", {}, SymbolKind::Method, "Channel count."),
        function("getSampleRate", "float", {}, SymbolKind::Method, "Sample rate in hertz."),
        function("getSamplesSinceAppStart", "uint64_t", {}, SymbolKind::Method, "Samples since app start."),
        function("getIncomingSample", "WaviateComplex", { param("int", "channel", "-1"), param("int", "bin", "-1") }, SymbolKind::Method, "Input frequency sample."),
        function("getCurrentSample", "WaviateComplex", { param("int", "channel", "-1"), param("int", "bin", "-1") }, SymbolKind::Method, "Current frequency output sample."),
        function("getSideChainSample", "WaviateComplex", { param("int", "channel", "0"), param("int", "bin", "-1") }, SymbolKind::Method, "Sidechain frequency sample."),
    };

    return symbols;
}

const std::vector<FieldSymbol>& waviateSampleInputFields()
{
    static const std::vector<FieldSymbol> symbols {
        field("samplesSinceAppStart", "uint64_t", "Samples since app start."),
        field("sampleInBlock", "int32_t", "Sample index in the current block."),
        field("blockSize", "int32_t", "Current block size."),
        field("inputChannelCount", "int32_t", "Input channel count."),
        field("sideChainChannelCount", "int32_t", "Sidechain channel count."),
        field("sampleMemoryCount", "int32_t", "Previous sample history count."),
        field("channelCount", "int32_t", "Output channel count."),
        field("channel", "uint8_t", "Current channel."),
        field("midiNoteOn", "uint8_t*", "MIDI note-on state array."),
        field("midiCCValue", "uint8_t*", "MIDI controller value array."),
        field("sampleWhenMidiNoteOn", "uint64_t*", "Sample positions for MIDI note-on events."),
        field("sampleWhenMidiNoteOff", "uint64_t*", "Sample positions for MIDI note-off events."),
        field("sampleWhenCCValueChanged", "uint64_t*", "Sample positions for MIDI controller changes."),
        field("sustain", "bool", "Sustain pedal state."),
        field("sustainDefer", "bool*", "Deferred sustain note-off state."),
        field("controllerCount", "int32_t", "Connected controller count."),
        field("controllerButtonMask", "uint64_t*", "Controller button states."),
        field("sampleWhenControllerButtonChanged", "uint64_t*", "Sample positions for controller button changes."),
        field("controllerButtonCount", "int32_t", "Controller button count."),
        field("controllerAxisValue", "float*", "Controller axis values."),
        field("sampleWhenControllerAxisChanged", "uint64_t*", "Sample positions for controller axis changes."),
        field("controllerAxisCount", "int32_t", "Controller axis count."),
        field("sampleRate", "float", "Sample rate in hertz."),
        field("previousSamples", "float**", "Previous sample history."),
        field("inputDeviceSamples", "const float* const*", "Input sample buffers."),
        field("inputSideChainSamples", "const float* const*", "Sidechain sample buffers."),
        field("currentSampleData", "float* const*", "Output sample buffers."),
    };

    return symbols;
}

const std::vector<FieldSymbol>& waviateFrequencyInputFields()
{
    static const std::vector<FieldSymbol> symbols {
        field("sampleWidth", "int32_t", "FFT window size."),
        field("bin", "int32_t", "Current frequency bin."),
        field("totalBinCount", "int32_t", "Total frequency bin count."),
        field("channelCount", "int32_t", "Channel count."),
        field("channel", "uint8_t", "Current channel."),
        field("currentFrequencyData", "const WaviateComplex**", "Current frequency output data."),
        field("inputDeviceData", "const WaviateComplex**", "Input frequency data."),
        field("inputSideChainFrequencyData", "const WaviateComplex**", "Sidechain frequency data."),
        field("sampleRate", "float", "Sample rate in hertz."),
        field("samplesSinceAppStart", "uint64_t", "Samples since app start."),
    };

    return symbols;
}

const std::vector<FieldSymbol>& waviateComplexFields()
{
    static const std::vector<FieldSymbol> symbols {
        field("real", "float", "Real component."),
        field("imag", "float", "Imaginary component."),
    };

    return symbols;
}

const std::vector<FieldSymbol>& cppBuiltinTypes()
{
    static const std::vector<FieldSymbol> symbols {
        classSymbol("WaviateSample", "Sample shader context."),
        classSymbol("WaviateFrequency", "Frequency shader context."),
        classSymbol("WaviateCore", "Shared oscillator and utility API."),
        classSymbol("WaviateSampleInput", "Sample input data passed to the C ABI."),
        classSymbol("WaviateSampleStateWriter", "Sample state writer."),
        classSymbol("WaviateFrequencyInput", "Frequency input data passed to the C ABI."),
        classSymbol("WaviateFrequencyStateWriter", "Frequency state writer."),
        classSymbol("WaviateComplex", "Complex number with real and imaginary fields."),
        typeSymbol("void"),
        typeSymbol("auto"),
        typeSymbol("bool"),
        typeSymbol("char"),
        typeSymbol("float"),
        typeSymbol("double"),
        typeSymbol("int"),
        typeSymbol("int32_t"),
        typeSymbol("uint8_t"),
        typeSymbol("uint32_t"),
        typeSymbol("uint64_t"),
    };

    return symbols;
}

const std::vector<FieldSymbol>& cppKeywords()
{
    static const std::vector<FieldSymbol> symbols {
        keyword("if"),
        keyword("else"),
        keyword("for"),
        keyword("while"),
        keyword("do"),
        keyword("switch"),
        keyword("case"),
        keyword("default"),
        keyword("break"),
        keyword("continue"),
        keyword("return"),
        keyword("const"),
        keyword("constexpr"),
        keyword("static"),
        keyword("struct"),
        keyword("class"),
        keyword("inline"),
        keyword("true", "Boolean true."),
        keyword("false", "Boolean false."),
        keyword("nullptr", "Null pointer literal."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateEntryPoints()
{
    static const std::vector<FunctionSymbol> symbols {
        function("SampleProcess", "float", { param("const WaviateSample&", "wav") }, SymbolKind::Function, "C++ sample shader entry point."),
        function("FrequencyProcess", "WaviateComplex", { param("const WaviateFrequency&", "wav") }, SymbolKind::Function, "C++ frequency shader entry point."),
        function("sample_process", "float", { param("const WaviateSampleInput*", "input"), param("WaviateSampleStateWriter*", "writer") }, SymbolKind::Function, "C ABI sample shader entry point."),
        function("frequency_process", "WaviateComplex", { param("const WaviateFrequencyInput*", "input"), param("WaviateFrequencyStateWriter*", "writer") }, SymbolKind::Function, "C ABI frequency shader entry point."),
    };

    return symbols;
}

bool isKnownWaviateType(const std::string& typeName)
{
    const auto& types = cppBuiltinTypes();
    return std::any_of(types.begin(), types.end(), [&typeName](const FieldSymbol& symbol) {
        return symbol.name == typeName
            && (symbol.kind == SymbolKind::Class || symbol.name.rfind("Waviate", 0) == 0);
    });
}

std::string buildEmbeddedCppApi()
{
    return R"waviate_cpp_api(
#ifndef WAVIATE_SCRIPT_INPUT_API_DEFINED
#define WAVIATE_SCRIPT_INPUT_API_DEFINED
using uint8_t = unsigned char;
using int32_t = int;
using uint32_t = unsigned int;
using uint64_t = unsigned long long;

struct WaviateSampleInput {
    uint64_t samplesSinceAppStart;
    int32_t sampleInBlock;
    int32_t blockSize;
    int32_t inputChannelCount;
    int32_t sideChainChannelCount;
    int32_t sampleMemoryCount;
    int32_t channelCount;
    uint8_t channel;

    uint8_t* midiNoteOn;
    uint8_t* midiCCValue;
    uint64_t* sampleWhenMidiNoteOn;
    uint64_t* sampleWhenMidiNoteOff;
    uint64_t* sampleWhenCCValueChanged;
    bool sustain;
    bool* sustainDefer;

    int32_t controllerCount;
    uint64_t* controllerButtonMask;
    uint64_t* sampleWhenControllerButtonChanged;
    int32_t controllerButtonCount;
    float* controllerAxisValue;
    uint64_t* sampleWhenControllerAxisChanged;
    int32_t controllerAxisCount;

    float sampleRate;
    float** previousSamples;
    const float* const* inputDeviceSamples;
    const float* const* inputSideChainSamples;
    float* const* currentSampleData;
};

struct WaviateSampleStateWriter {};

struct WaviateComplex {
    float real;
    float imag;
};

struct WaviateFrequencyInput {
    int32_t sampleWidth;
    int32_t bin;
    int32_t totalBinCount;
    int32_t channelCount;
    uint8_t channel;

    const WaviateComplex** currentFrequencyData;
    const WaviateComplex** inputDeviceData;
    const WaviateComplex** inputSideChainFrequencyData;

    float sampleRate;
    uint64_t samplesSinceAppStart;
};

struct WaviateFrequencyStateWriter {};

#endif

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

    static float wavSin(float x) { return __builtin_sinf(x); }
    static float wavTan(float x) { return __builtin_tanf(x); }
    static float wavFloor(float x) { return __builtin_floorf(x); }
    static float wavAbs(float x) { return __builtin_fabsf(x); }
    static float wavSqrt(float x) { return __builtin_sqrtf(x); }
    static float wavLog2(float x) { return __builtin_log2f(x); }

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

)waviate_cpp_api";
}
} // namespace waviate::language
