#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cctype>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AbstractCompiler.h"

#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/LangStandard.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/PreprocessorOptions.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/TargetSelect.h>

template <bool cppMode>
class ClangCompiler final : public AbstractCompiler {
public:
    ClangCompiler();
    ~ClangCompiler() = default;

    void compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) override;

    const void* getDispatchPtr() const noexcept {
        return dispatch_.load(std::memory_order_acquire);
    }

private:
    struct Dispatch final {
        SampleShader sample = nullptr;
        FrequencyShader freq = nullptr;
    };

    struct CompiledUnit final {
        std::unique_ptr<llvm::LLVMContext> ctx;
        std::unique_ptr<llvm::ExecutionEngine> ee;
        Dispatch dispatch{};
    };

    static constexpr size_t kKeepOldUnits = 4;

    std::unique_ptr<CompiledUnit> active_;
    std::deque<std::unique_ptr<CompiledUnit>> retired_;
    std::atomic<const Dispatch*> dispatch_{ nullptr };

    static bool isIdentifierChar(char c) noexcept;
    static std::string stripCommentsAndStrings(const std::string& source);
    static bool containsFunctionLikeIdentifier(const std::string& source, const char* name);
    static std::string buildEmbeddedCppApi();
    static std::string buildCppAbiShim(const std::string& userSource);
    static std::string buildTranslationUnit(const std::string& userSource);
    static void configureInvocation(std::shared_ptr <clang::CompilerInvocation>& inv, const char* virtualFilename);
    static std::unique_ptr<llvm::Module> emitLLVMModule(
        llvm::LLVMContext& ctx,
        std::unique_ptr<llvm::MemoryBuffer> buffer,
        std::string& diagnostics
    );

    static std::unique_ptr<llvm::ExecutionEngine> buildJIT(std::unique_ptr<llvm::Module> m);
    void retireOldActive();
};

template <bool cppMode>
ClangCompiler<cppMode>::ClangCompiler() {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

template <bool cppMode>
bool ClangCompiler<cppMode>::isIdentifierChar(char c) noexcept {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::stripCommentsAndStrings(const std::string& source) {
    enum class ScanState {
        normal,
        lineComment,
        blockComment,
        stringLiteral,
        charLiteral
    };

    std::string stripped = source;
    ScanState state = ScanState::normal;

    for (size_t i = 0; i < source.size(); ++i) {
        const char c = source[i];
        const char next = (i + 1 < source.size()) ? source[i + 1] : '\0';

        switch (state) {
        case ScanState::normal:
            if (c == '/' && next == '/') {
                stripped[i] = ' ';
                stripped[i + 1] = ' ';
                ++i;
                state = ScanState::lineComment;
            }
            else if (c == '/' && next == '*') {
                stripped[i] = ' ';
                stripped[i + 1] = ' ';
                ++i;
                state = ScanState::blockComment;
            }
            else if (c == '"') {
                stripped[i] = ' ';
                state = ScanState::stringLiteral;
            }
            else if (c == '\'') {
                stripped[i] = ' ';
                state = ScanState::charLiteral;
            }
            break;

        case ScanState::lineComment:
            if (c == '\n' || c == '\r') {
                state = ScanState::normal;
            }
            else {
                stripped[i] = ' ';
            }
            break;

        case ScanState::blockComment:
            stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
            if (c == '*' && next == '/') {
                stripped[i + 1] = ' ';
                ++i;
                state = ScanState::normal;
            }
            break;

        case ScanState::stringLiteral:
            stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
            if (c == '\\' && next != '\0') {
                stripped[i + 1] = (next == '\n' || next == '\r') ? next : ' ';
                ++i;
            }
            else if (c == '"') {
                state = ScanState::normal;
            }
            break;

        case ScanState::charLiteral:
            stripped[i] = (c == '\n' || c == '\r') ? c : ' ';
            if (c == '\\' && next != '\0') {
                stripped[i + 1] = (next == '\n' || next == '\r') ? next : ' ';
                ++i;
            }
            else if (c == '\'') {
                state = ScanState::normal;
            }
            break;
        }
    }

    return stripped;
}

template <bool cppMode>
bool ClangCompiler<cppMode>::containsFunctionLikeIdentifier(const std::string& source, const char* name) {
    const std::string identifier(name);
    size_t pos = 0;

    while ((pos = source.find(identifier, pos)) != std::string::npos) {
        const size_t end = pos + identifier.size();
        const bool leftBoundary = pos == 0 || !isIdentifierChar(source[pos - 1]);
        const bool rightBoundary = end >= source.size() || !isIdentifierChar(source[end]);

        if (leftBoundary && rightBoundary) {
            size_t cursor = end;
            while (cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor])) != 0)
                ++cursor;

            if (cursor < source.size() && source[cursor] == '(')
                return true;
        }

        pos = end;
    }

    return false;
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::buildEmbeddedCppApi() {
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

template <bool cppMode>
std::string ClangCompiler<cppMode>::buildCppAbiShim(const std::string& userSource) {
    const std::string strippedSource = stripCommentsAndStrings(userSource);
    const bool hasCAbiSample = containsFunctionLikeIdentifier(strippedSource, "sample_process");
    const bool hasCAbiFrequency = containsFunctionLikeIdentifier(strippedSource, "frequency_process");
    const bool hasCppSample = containsFunctionLikeIdentifier(strippedSource, "SampleProcess");
    const bool hasCppFrequency = containsFunctionLikeIdentifier(strippedSource, "FrequencyProcess");

    std::string shim;
    if (!hasCAbiSample && hasCppSample) {
        shim.append(R"wslshim(
extern "C" float sample_process(const WaviateSampleInput* input, WaviateSampleStateWriter* writer) {
    WaviateSample wav(input, writer);
    return SampleProcess(wav);
}
)wslshim");
    }

    if (!hasCAbiFrequency && hasCppFrequency) {
        shim.append(R"wslshim(
extern "C" WaviateComplex frequency_process(const WaviateFrequencyInput* input, WaviateFrequencyStateWriter* writer) {
    WaviateFrequency wav(input, writer);
    return FrequencyProcess(wav);
}
)wslshim");
    }

    return shim;
}

template <bool cppMode>
std::string ClangCompiler<cppMode>::buildTranslationUnit(const std::string& userSource) {
    std::string tu;
    tu.reserve(userSource.size() + 4096);

    if constexpr (cppMode) {
        tu.append("#line 1 \"WaviateSdk.hpp\"\n");
        tu.append(buildEmbeddedCppApi());
        tu.append("\n#line 1 \"shader.wsl\"\n");
        tu.append(userSource);
        tu.append("\n\n#line 1 \"WaviateCppAbiShim.hpp\"\n");
        tu.append(buildCppAbiShim(userSource));
        tu.append("\n");
    }
    else {
        tu.append(R"(#include "C:\Program Files\Waviate\Script\Include\Waviate.h")");
        tu.append("\n\n");
        tu.append(userSource);
        tu.append("\n");
    }

    return tu;
}

template <bool cppMode>
void ClangCompiler<cppMode>::configureInvocation(std::shared_ptr<clang::CompilerInvocation>& inv, const char* virtualFilename) {
    inv = std::make_shared<clang::CompilerInvocation>();
    inv->getTargetOpts().Triple = llvm::sys::getDefaultTargetTriple();

    auto& fe = inv->getFrontendOpts();
    fe.Inputs.clear();

    clang::InputKind kind = cppMode
        ? clang::InputKind(clang::Language::CXX)
        : clang::InputKind(clang::Language::C);

    fe.Inputs.emplace_back(virtualFilename, kind);
    fe.DisableFree = false;

    auto& lang = inv->getLangOpts();
    std::vector<std::string> implicitIncludes;
    const llvm::Triple targetTriple(inv->getTargetOpts().Triple);
    clang::LangOptions::setLangDefaults(
        lang,
        cppMode ? clang::Language::CXX : clang::Language::C,
        targetTriple,
        implicitIncludes,
        cppMode ? clang::LangStandard::lang_cxx23 : clang::LangStandard::lang_c17);

    auto& cg = inv->getCodeGenOpts();
    cg.OptimizationLevel = 2;

    auto& hs = inv->getHeaderSearchOpts();
    hs.UseBuiltinIncludes = true;
    hs.UseStandardSystemIncludes = false;
    hs.UseStandardCXXIncludes = false;

    hs.AddPath(R"(C:\Program Files\Waviate\Script\Include)", clang::frontend::System, false, false);
}

template <bool cppMode> 
std::unique_ptr<llvm::Module> ClangCompiler<cppMode>::emitLLVMModule(
    llvm::LLVMContext& ctx,
    std::unique_ptr<llvm::MemoryBuffer> buffer,
    std::string& diagnostics
) {
    auto diagOpts = llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions>(new clang::DiagnosticOptions());
    llvm::raw_string_ostream diagStream(diagnostics);
    clang::TextDiagnosticPrinter diagClient(diagStream, diagOpts.get());

    clang::CompilerInstance ci;
    std::shared_ptr<clang::CompilerInvocation> invNew;
    const char* virtualFilename = cppMode ? "shader.cpp" : "shader.c";
    configureInvocation(invNew, virtualFilename);
    ci.setInvocation(invNew);
    ci.createDiagnostics(&diagClient, false);
    if (!ci.hasDiagnostics()) return nullptr;

    ci.setTarget(clang::TargetInfo::CreateTargetInfo(ci.getDiagnostics(), ci.getInvocation().TargetOpts));
    if (!ci.hasTarget()) return nullptr;

    const auto& inputs = invNew->getFrontendOpts().Inputs;
    if (inputs.empty()) return nullptr;

    const std::string vFilename = inputs.front().getFile().str();

    ci.createFileManager();
    ci.createSourceManager(ci.getFileManager());

    ci.getPreprocessorOpts().RetainRemappedFileBuffers = true;
    ci.getPreprocessorOpts().addRemappedFile(vFilename, buffer.release());

    clang::EmitLLVMOnlyAction action(&ctx);
    if (!ci.ExecuteAction(action)) {
        diagStream.flush();
        return nullptr;
    }

    diagStream.flush();
    return action.takeModule();
}

template <bool cppMode>
std::unique_ptr<llvm::ExecutionEngine> ClangCompiler<cppMode>::buildJIT(std::unique_ptr<llvm::Module> m) {
    if (!m) return nullptr;

    std::string err;
    llvm::ExecutionEngine* raw = llvm::EngineBuilder(std::move(m))
        .setEngineKind(llvm::EngineKind::JIT)
        .setErrorStr(&err)
        .create();

    if (!raw) return nullptr;

    return std::unique_ptr<llvm::ExecutionEngine>(raw);
}

template <bool cppMode>
void ClangCompiler<cppMode>::retireOldActive() {
    if (active_) {
        retired_.push_back(std::move(active_));
        while (retired_.size() > kKeepOldUnits) {
            retired_.pop_front();
        }
    }
}

template <bool cppMode>
void ClangCompiler<cppMode>::compileSource(std::string source, SampleShader& outSample, FrequencyShader& outFrequency) {
    outSample = nullptr;
    outFrequency = nullptr;

    dispatch_.store(nullptr, std::memory_order_release);

    const char* virtualFilename = cppMode ? "shader.cpp" : "shader.c";
    const std::string tu = buildTranslationUnit(source);

    auto unit = std::make_unique<CompiledUnit>();
    unit->ctx = std::make_unique<llvm::LLVMContext>();

    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(tu, virtualFilename);
    if (!buffer) return;

    std::string diagnostics;
    auto module = emitLLVMModule(*unit->ctx, std::move(buffer), diagnostics);
    if (!module) {
        if (!diagnostics.empty())
            throw std::runtime_error(diagnostics);

        throw std::runtime_error("Clang did not emit an LLVM module");
    }

    unit->ee = buildJIT(std::move(module));
    if (!unit->ee)
        throw std::runtime_error("Failed to create the JIT execution engine");

    unit->ee->finalizeObject();

    const uint64_t sampleAddr = unit->ee->getFunctionAddress("sample_process");
    const uint64_t freqAddr = unit->ee->getFunctionAddress("frequency_process");

    unit->dispatch.sample = sampleAddr ? reinterpret_cast<SampleShader>(sampleAddr) : nullptr;
    unit->dispatch.freq = freqAddr ? reinterpret_cast<FrequencyShader>(freqAddr) : nullptr;

    outSample = unit->dispatch.sample;
    outFrequency = unit->dispatch.freq;

    retireOldActive();
    active_ = std::move(unit);

    dispatch_.store(&active_->dispatch, std::memory_order_release);
}
