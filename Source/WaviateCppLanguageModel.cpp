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

FieldSymbol constantSymbol(std::string name, std::string type, std::string documentation = {})
{
    return { std::move(name), std::move(type), SymbolKind::Constant, std::move(documentation) };
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
        function("secondsSinceAppStart", "float", {}, SymbolKind::Method, "Seconds since app start."),
        function("samplesToSeconds", "float", { param("uint64_t", "samples") }, SymbolKind::Method, "Convert samples to seconds."),
        function("secondsToSamples", "uint64_t", { param("float", "seconds") }, SymbolKind::Method, "Convert seconds to samples."),
        function("sampleRateHz", "float", {}, SymbolKind::Method, "Sample rate in hertz."),
        function("sampleRateKHz", "float", {}, SymbolKind::Method, "Sample rate in kilohertz."),
        function("isMidiNoteOn", "bool", { param("int", "note") }, SymbolKind::Method, "Whether a MIDI note is held or sustained."),
        function("midiCCValue", "uint8_t", { param("int", "controller") }, SymbolKind::Method, "MIDI controller value."),
        function("midiNotePressCount", "int", {}, SymbolKind::Method, "Number of notes in the press ordering."),
        function("midiNoteReleaseCount", "int", {}, SymbolKind::Method, "Number of notes in the release ordering."),
        function("midiVoiceCount", "int", {}, SymbolKind::Method, "Number of allocated note voices."),
        function("midiNotePressOrder", "int", { param("int", "index") }, SymbolKind::Method, "Note number at a newest-first press index."),
        function("midiNoteReleaseOrder", "int", { param("int", "index") }, SymbolKind::Method, "Note number at a newest-first release index."),
        function("midiVoiceNote", "int", { param("int", "index") }, SymbolKind::Method, "Note number at a newest-first voice index."),
        function("sampleWhenMidiNotePressed", "uint64_t", { param("int", "note") }, SymbolKind::Method, "Absolute sample of the latest note press."),
        function("sampleWhenMidiNoteReleased", "uint64_t", { param("int", "note") }, SymbolKind::Method, "Absolute sample of the latest note release."),
        function("samplesSinceMidiNotePressed", "uint64_t", { param("int", "note") }, SymbolKind::Method, "Samples since the latest note press."),
        function("samplesSinceMidiNoteReleased", "uint64_t", { param("int", "note") }, SymbolKind::Method, "Samples since the latest note release."),
        function("midiNoteFrequency", "float", { param("int", "note") }, SymbolKind::Method, "12-TET note frequency in hertz."),
        function("midiNotePhase", "float", { param("int", "note") }, SymbolKind::Method, "12-TET phase in [0, 1) since note press."),
        function("adsr", "float", { param("int", "note"), param("float", "attackSeconds"), param("float", "decaySeconds"), param("float", "sustainLevel"), param("float", "releaseSeconds") }, SymbolKind::Method, "Note-aware ADSR amplitude in [0, 1]."),
        function("midiVoices", "WaviateCore::MidiVoices", { param("int", "maximumVoices", "128") }, SymbolKind::Method, "Newest-first bounded iterable voice view."),
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
        function("perlin", "float", { param("float", "x"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Method, "Perlin noise."),
        function("simplex", "float", { param("float", "x"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Method, "Simplex noise."),
        function("voronoi", "float", { param("float", "x"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Method, "Voronoi noise."),
        function("turbulence", "float", { param("float", "x"), param("int", "octaves", "4"), param("float", "lacunarity", "2.0f"), param("float", "gain", "0.5f"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Method, "Turbulence noise."),
        function("ridgedMulti", "float", { param("float", "x"), param("int", "octaves", "4"), param("float", "lacunarity", "2.0f"), param("float", "gain", "0.5f"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Method, "Ridged multifractal noise."),
        function("newArray", "WaviateArray<T>", { param("size_t", "size") }, SymbolKind::Method, "Allocate a temporary arena-backed array for the current shader pass."),
        function("newVector", "WaviateVector<T>", { param("size_t", "initialCapacity", "0") }, SymbolKind::Method, "Allocate a temporary arena-backed vector for the current shader pass."),
        function("newString", "WaviateString", {}, SymbolKind::Method, "Allocate a temporary arena-backed string for the current shader pass."),
        function("newMap", "WaviateMap<K,V>", { param("size_t", "initialCapacity", "0") }, SymbolKind::Method, "Allocate a temporary arena-backed linear map for the current shader pass."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateAudioMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("isLoaded", "bool", {}, SymbolKind::Method, "Whether this audio clip is ready to read."),
        function("length", "uint64_t", {}, SymbolKind::Method, "Length of the loaded audio clip in sample frames."),
        function("channelCount", "int", {}, SymbolKind::Method, "Number of channels in the loaded audio clip."),
        function("sampleRateHz", "float", {}, SymbolKind::Method, "Sample rate of the loaded audio clip in hertz."),
        function("readSample", "float", { param("uint64_t", "sampleIndex"), param("int", "channel", "0") }, SymbolKind::Method, "Read a sample frame directly."),
        function("read", "float", {
            param("float", "x"),
            param("WaviateAudioAddressMode", "addressMode", "WaviateAudioAddressMode::Clamp"),
            param("WaviateAudioInterpolation", "interpolation", "WaviateAudioInterpolation::Linear"),
            param("int", "channel", "0")
        }, SymbolKind::Method, "Read the clip at a normalized position."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateGlobalFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("phase", "float", { param("float", "x") }, SymbolKind::Function, "Fractional phase in [0, 1)."),
        function("sine", "float", { param("float", "x") }, SymbolKind::Function, "Sine oscillator."),
        function("saw", "float", { param("float", "x") }, SymbolKind::Function, "Saw oscillator."),
        function("square", "float", { param("float", "x") }, SymbolKind::Function, "Square oscillator."),
        function("pulse", "float", { param("float", "x"), param("float", "width", "0.5f") }, SymbolKind::Function, "Pulse oscillator."),
        function("triangle", "float", { param("float", "x") }, SymbolKind::Function, "Triangle oscillator."),
        function("semicircle", "float", { param("float", "x") }, SymbolKind::Function, "Semicircle oscillator."),
        function("sawTan", "float", { param("float", "x") }, SymbolKind::Function, "Tangent-shaped saw oscillator."),
        function("triangleTan", "float", { param("float", "x") }, SymbolKind::Function, "Tangent-shaped triangle oscillator."),
        function("strongSine", "float", { param("float", "x") }, SymbolKind::Function, "Sine oscillator with harmonic weight."),
        function("fractalSquare", "float", { param("float", "x") }, SymbolKind::Function, "Fractal square oscillator."),
        function("perlin", "float", { param("float", "x"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Function, "Perlin noise."),
        function("simplex", "float", { param("float", "x"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Function, "Simplex noise."),
        function("voronoi", "float", { param("float", "x"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Function, "Voronoi noise."),
        function("turbulence", "float", { param("float", "x"), param("int", "octaves", "4"), param("float", "lacunarity", "2.0f"), param("float", "gain", "0.5f"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Function, "Turbulence noise."),
        function("ridgedMulti", "float", { param("float", "x"), param("int", "octaves", "4"), param("float", "lacunarity", "2.0f"), param("float", "gain", "0.5f"), param("float", "min", "0.0f"), param("float", "max", "1.0f") }, SymbolKind::Function, "Ridged multifractal noise."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& cMathFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("acos", "float", { param("float", "x") }, SymbolKind::Function, "Arc cosine."),
        function("acosf", "float", { param("float", "x") }, SymbolKind::Function, "Arc cosine."),
        function("asin", "float", { param("float", "x") }, SymbolKind::Function, "Arc sine."),
        function("asinf", "float", { param("float", "x") }, SymbolKind::Function, "Arc sine."),
        function("atan", "float", { param("float", "x") }, SymbolKind::Function, "Arc tangent."),
        function("atanf", "float", { param("float", "x") }, SymbolKind::Function, "Arc tangent."),
        function("atan2", "float", { param("float", "y"), param("float", "x") }, SymbolKind::Function, "Arc tangent of y / x."),
        function("atan2f", "float", { param("float", "y"), param("float", "x") }, SymbolKind::Function, "Arc tangent of y / x."),
        function("cos", "float", { param("float", "x") }, SymbolKind::Function, "Cosine."),
        function("cosf", "float", { param("float", "x") }, SymbolKind::Function, "Cosine."),
        function("sin", "float", { param("float", "x") }, SymbolKind::Function, "Sine."),
        function("sinf", "float", { param("float", "x") }, SymbolKind::Function, "Sine."),
        function("tan", "float", { param("float", "x") }, SymbolKind::Function, "Tangent."),
        function("tanf", "float", { param("float", "x") }, SymbolKind::Function, "Tangent."),
        function("acosh", "float", { param("float", "x") }, SymbolKind::Function, "Inverse hyperbolic cosine."),
        function("acoshf", "float", { param("float", "x") }, SymbolKind::Function, "Inverse hyperbolic cosine."),
        function("asinh", "float", { param("float", "x") }, SymbolKind::Function, "Inverse hyperbolic sine."),
        function("asinhf", "float", { param("float", "x") }, SymbolKind::Function, "Inverse hyperbolic sine."),
        function("atanh", "float", { param("float", "x") }, SymbolKind::Function, "Inverse hyperbolic tangent."),
        function("atanhf", "float", { param("float", "x") }, SymbolKind::Function, "Inverse hyperbolic tangent."),
        function("cosh", "float", { param("float", "x") }, SymbolKind::Function, "Hyperbolic cosine."),
        function("coshf", "float", { param("float", "x") }, SymbolKind::Function, "Hyperbolic cosine."),
        function("sinh", "float", { param("float", "x") }, SymbolKind::Function, "Hyperbolic sine."),
        function("sinhf", "float", { param("float", "x") }, SymbolKind::Function, "Hyperbolic sine."),
        function("tanh", "float", { param("float", "x") }, SymbolKind::Function, "Hyperbolic tangent."),
        function("tanhf", "float", { param("float", "x") }, SymbolKind::Function, "Hyperbolic tangent."),
        function("exp", "float", { param("float", "x") }, SymbolKind::Function, "e raised to x."),
        function("expf", "float", { param("float", "x") }, SymbolKind::Function, "e raised to x."),
        function("exp2", "float", { param("float", "x") }, SymbolKind::Function, "2 raised to x."),
        function("exp2f", "float", { param("float", "x") }, SymbolKind::Function, "2 raised to x."),
        function("expm1", "float", { param("float", "x") }, SymbolKind::Function, "exp(x) - 1."),
        function("expm1f", "float", { param("float", "x") }, SymbolKind::Function, "exp(x) - 1."),
        function("log", "float", { param("float", "x") }, SymbolKind::Function, "Natural logarithm."),
        function("logf", "float", { param("float", "x") }, SymbolKind::Function, "Natural logarithm."),
        function("log10", "float", { param("float", "x") }, SymbolKind::Function, "Base-10 logarithm."),
        function("log10f", "float", { param("float", "x") }, SymbolKind::Function, "Base-10 logarithm."),
        function("log1p", "float", { param("float", "x") }, SymbolKind::Function, "log(1 + x)."),
        function("log1pf", "float", { param("float", "x") }, SymbolKind::Function, "log(1 + x)."),
        function("log2", "float", { param("float", "x") }, SymbolKind::Function, "Base-2 logarithm."),
        function("log2f", "float", { param("float", "x") }, SymbolKind::Function, "Base-2 logarithm."),
        function("frexp", "double", { param("double", "x"), param("int*", "exponent") }, SymbolKind::Function, "Split x into a normalized fraction and exponent."),
        function("frexpf", "float", { param("float", "x"), param("int*", "exponent") }, SymbolKind::Function, "Split x into a normalized fraction and exponent."),
        function("ilogb", "int", { param("double", "x") }, SymbolKind::Function, "Floating-point exponent as an integer."),
        function("ilogbf", "int", { param("float", "x") }, SymbolKind::Function, "Floating-point exponent as an integer."),
        function("ldexp", "double", { param("double", "x"), param("int", "exponent") }, SymbolKind::Function, "Multiply x by 2 raised to exponent."),
        function("ldexpf", "float", { param("float", "x"), param("int", "exponent") }, SymbolKind::Function, "Multiply x by 2 raised to exponent."),
        function("logb", "double", { param("double", "x") }, SymbolKind::Function, "Floating-point exponent."),
        function("logbf", "float", { param("float", "x") }, SymbolKind::Function, "Floating-point exponent."),
        function("modf", "double", { param("double", "x"), param("double*", "integerPart") }, SymbolKind::Function, "Split x into integer and fractional parts."),
        function("modff", "float", { param("float", "x"), param("float*", "integerPart") }, SymbolKind::Function, "Split x into integer and fractional parts."),
        function("scalbn", "double", { param("double", "x"), param("int", "exponent") }, SymbolKind::Function, "Multiply x by FLT_RADIX raised to exponent."),
        function("scalbnf", "float", { param("float", "x"), param("int", "exponent") }, SymbolKind::Function, "Multiply x by FLT_RADIX raised to exponent."),
        function("scalbln", "double", { param("double", "x"), param("long", "exponent") }, SymbolKind::Function, "Multiply x by FLT_RADIX raised to exponent."),
        function("scalblnf", "float", { param("float", "x"), param("long", "exponent") }, SymbolKind::Function, "Multiply x by FLT_RADIX raised to exponent."),
        function("cbrt", "float", { param("float", "x") }, SymbolKind::Function, "Cube root."),
        function("cbrtf", "float", { param("float", "x") }, SymbolKind::Function, "Cube root."),
        function("fabs", "float", { param("float", "x") }, SymbolKind::Function, "Absolute value."),
        function("fabsf", "float", { param("float", "x") }, SymbolKind::Function, "Absolute value."),
        function("hypot", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Hypotenuse."),
        function("hypotf", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Hypotenuse."),
        function("pow", "float", { param("float", "base"), param("float", "exponent") }, SymbolKind::Function, "Power."),
        function("powf", "float", { param("float", "base"), param("float", "exponent") }, SymbolKind::Function, "Power."),
        function("sqrt", "float", { param("float", "x") }, SymbolKind::Function, "Square root."),
        function("sqrtf", "float", { param("float", "x") }, SymbolKind::Function, "Square root."),
        function("erf", "double", { param("double", "x") }, SymbolKind::Function, "Error function."),
        function("erff", "float", { param("float", "x") }, SymbolKind::Function, "Error function."),
        function("erfc", "double", { param("double", "x") }, SymbolKind::Function, "Complementary error function."),
        function("erfcf", "float", { param("float", "x") }, SymbolKind::Function, "Complementary error function."),
        function("lgamma", "double", { param("double", "x") }, SymbolKind::Function, "Natural log of absolute gamma."),
        function("lgammaf", "float", { param("float", "x") }, SymbolKind::Function, "Natural log of absolute gamma."),
        function("tgamma", "double", { param("double", "x") }, SymbolKind::Function, "Gamma function."),
        function("tgammaf", "float", { param("float", "x") }, SymbolKind::Function, "Gamma function."),
        function("ceil", "float", { param("float", "x") }, SymbolKind::Function, "Ceiling."),
        function("ceilf", "float", { param("float", "x") }, SymbolKind::Function, "Ceiling."),
        function("floor", "float", { param("float", "x") }, SymbolKind::Function, "Floor."),
        function("floorf", "float", { param("float", "x") }, SymbolKind::Function, "Floor."),
        function("nearbyint", "double", { param("double", "x") }, SymbolKind::Function, "Round using the current rounding mode."),
        function("nearbyintf", "float", { param("float", "x") }, SymbolKind::Function, "Round using the current rounding mode."),
        function("rint", "double", { param("double", "x") }, SymbolKind::Function, "Round using the current rounding mode."),
        function("rintf", "float", { param("float", "x") }, SymbolKind::Function, "Round using the current rounding mode."),
        function("lrint", "long", { param("double", "x") }, SymbolKind::Function, "Round to long using the current rounding mode."),
        function("lrintf", "long", { param("float", "x") }, SymbolKind::Function, "Round to long using the current rounding mode."),
        function("llrint", "long long", { param("double", "x") }, SymbolKind::Function, "Round to long long using the current rounding mode."),
        function("llrintf", "long long", { param("float", "x") }, SymbolKind::Function, "Round to long long using the current rounding mode."),
        function("round", "float", { param("float", "x") }, SymbolKind::Function, "Round to nearest."),
        function("roundf", "float", { param("float", "x") }, SymbolKind::Function, "Round to nearest."),
        function("lround", "long", { param("double", "x") }, SymbolKind::Function, "Round to long."),
        function("lroundf", "long", { param("float", "x") }, SymbolKind::Function, "Round to long."),
        function("llround", "long long", { param("double", "x") }, SymbolKind::Function, "Round to long long."),
        function("llroundf", "long long", { param("float", "x") }, SymbolKind::Function, "Round to long long."),
        function("trunc", "float", { param("float", "x") }, SymbolKind::Function, "Truncate."),
        function("truncf", "float", { param("float", "x") }, SymbolKind::Function, "Truncate."),
        function("fmod", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Floating-point remainder."),
        function("fmodf", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Floating-point remainder."),
        function("remainder", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "IEEE remainder."),
        function("remainderf", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "IEEE remainder."),
        function("remquo", "double", { param("double", "x"), param("double", "y"), param("int*", "quotient") }, SymbolKind::Function, "IEEE remainder plus quotient bits."),
        function("remquof", "float", { param("float", "x"), param("float", "y"), param("int*", "quotient") }, SymbolKind::Function, "IEEE remainder plus quotient bits."),
        function("copysign", "float", { param("float", "magnitude"), param("float", "sign") }, SymbolKind::Function, "Copy sign."),
        function("copysignf", "float", { param("float", "magnitude"), param("float", "sign") }, SymbolKind::Function, "Copy sign."),
        function("nan", "double", { param("const char*", "tagp") }, SymbolKind::Function, "Quiet NaN."),
        function("nanf", "float", { param("const char*", "tagp") }, SymbolKind::Function, "Quiet NaN."),
        function("nextafter", "double", { param("double", "from"), param("double", "to") }, SymbolKind::Function, "Next representable value toward to."),
        function("nextafterf", "float", { param("float", "from"), param("float", "to") }, SymbolKind::Function, "Next representable value toward to."),
        function("nexttoward", "double", { param("double", "from"), param("long double", "to") }, SymbolKind::Function, "Next representable value toward to."),
        function("nexttowardf", "float", { param("float", "from"), param("long double", "to") }, SymbolKind::Function, "Next representable value toward to."),
        function("fmax", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Maximum numeric value."),
        function("fmaxf", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Maximum numeric value."),
        function("fmin", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Minimum numeric value."),
        function("fminf", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Minimum numeric value."),
        function("fdim", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Positive difference."),
        function("fdimf", "float", { param("float", "x"), param("float", "y") }, SymbolKind::Function, "Positive difference."),
        function("fma", "float", { param("float", "x"), param("float", "y"), param("float", "z") }, SymbolKind::Function, "Fused multiply-add."),
        function("fmaf", "float", { param("float", "x"), param("float", "y"), param("float", "z") }, SymbolKind::Function, "Fused multiply-add."),
        function("fpclassify", "int", { param("double", "x") }, SymbolKind::Function, "Floating-point classification."),
        function("isfinite", "bool", { param("float", "x") }, SymbolKind::Function, "True if x is finite."),
        function("isinf", "bool", { param("float", "x") }, SymbolKind::Function, "True if x is infinite."),
        function("isnan", "bool", { param("float", "x") }, SymbolKind::Function, "True if x is NaN."),
        function("isnormal", "bool", { param("float", "x") }, SymbolKind::Function, "True if x is normal."),
        function("signbit", "bool", { param("float", "x") }, SymbolKind::Function, "True if x has a negative sign bit."),
    };

    return symbols;
}

const std::vector<FieldSymbol>& cMathConstants()
{
    static const std::vector<FieldSymbol> symbols {
        constantSymbol("HUGE_VAL", "double", "Positive overflow value."),
        constantSymbol("HUGE_VALF", "float", "Positive overflow value."),
        constantSymbol("HUGE_VALL", "long double", "Positive overflow value."),
        constantSymbol("INFINITY", "float", "Positive infinity."),
        constantSymbol("NAN", "float", "Quiet NaN."),
        constantSymbol("FP_NAN", "int", "fpclassify result for NaN."),
        constantSymbol("FP_INFINITE", "int", "fpclassify result for infinity."),
        constantSymbol("FP_ZERO", "int", "fpclassify result for zero."),
        constantSymbol("FP_SUBNORMAL", "int", "fpclassify result for subnormal values."),
        constantSymbol("FP_NORMAL", "int", "fpclassify result for normal values."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateSampleMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("channel", "int", {}, SymbolKind::Method, "Current channel."),
        function("sampleInBlock", "int", {}, SymbolKind::Method, "Sample index in the current block."),
        function("blockSize", "int", {}, SymbolKind::Method, "Current block size."),
        function("inputChannelCount", "int", {}, SymbolKind::Method, "Input channel count."),
        function("sideChainChannelCount", "int", {}, SymbolKind::Method, "Sidechain channel count."),
        function("channelCount", "int", {}, SymbolKind::Method, "Output channel count."),
        function("sampleRate", "float", {}, SymbolKind::Method, "Sample rate in hertz."),
        function("samplesSinceAppStart", "uint64_t", {}, SymbolKind::Method, "Samples since app start."),
        function("isSustainDown", "bool", {}, SymbolKind::Method, "Sustain pedal state."),
        function("incomingSample", "float", { param("int", "channel", "-1"), param("int", "sample", "-1") }, SymbolKind::Method, "Input sample."),
        function("sideChainSample", "float", { param("int", "channel", "0"), param("int", "sample", "-1") }, SymbolKind::Method, "Sidechain sample."),
        function("currentSample", "float", { param("int", "channel", "-1"), param("int", "sample", "-1") }, SymbolKind::Method, "Current output sample."),
        function("setCurrentSample", "void", { param("float", "value"), param("int", "channel", "-1"), param("int", "sample", "-1") }, SymbolKind::Method, "Write the current output sample."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateFrequencyMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("channel", "int", {}, SymbolKind::Method, "Current channel."),
        function("bin", "int", {}, SymbolKind::Method, "Current frequency bin."),
        function("totalBinCount", "int", {}, SymbolKind::Method, "Total frequency bin count."),
        function("sampleWidth", "int", {}, SymbolKind::Method, "FFT window size."),
        function("channelCount", "int", {}, SymbolKind::Method, "Channel count."),
        function("sampleRate", "float", {}, SymbolKind::Method, "Sample rate in hertz."),
        function("samplesSinceAppStart", "uint64_t", {}, SymbolKind::Method, "Samples since app start."),
        function("incomingSample", "WaviateComplex", { param("int", "channel", "-1"), param("int", "bin", "-1") }, SymbolKind::Method, "Input frequency sample."),
        function("currentSample", "WaviateComplex", { param("int", "channel", "-1"), param("int", "bin", "-1") }, SymbolKind::Method, "Current frequency output sample."),
        function("sideChainSample", "WaviateComplex", { param("int", "channel", "0"), param("int", "bin", "-1") }, SymbolKind::Method, "Sidechain frequency sample."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateMidiVoiceMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("note", "int", {}, SymbolKind::Method, "MIDI note number."),
        function("isHeld", "bool", {}, SymbolKind::Method, "Whether the note is held or sustained."),
        function("samplesSincePressed", "uint64_t", {}, SymbolKind::Method, "Samples since the latest press."),
        function("samplesSinceReleased", "uint64_t", {}, SymbolKind::Method, "Samples since the latest release."),
        function("frequency", "float", {}, SymbolKind::Method, "12-TET frequency in hertz."),
        function("phase", "float", {}, SymbolKind::Method, "12-TET phase in [0, 1)."),
        function("adsr", "float", { param("float", "attackSeconds"), param("float", "decaySeconds"), param("float", "sustainLevel"), param("float", "releaseSeconds") }, SymbolKind::Method, "Voice ADSR amplitude in [0, 1]."),
    };
    return symbols;
}

const std::vector<FunctionSymbol>& waviateMidiVoicesMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("count", "int", {}, SymbolKind::Method, "Number of voices in this bounded view."),
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
        field("midiNotePressOrder", "const uint8_t*", "Newest-first unique MIDI note press order."),
        field("midiNoteReleaseOrder", "const uint8_t*", "Newest-first unique MIDI note release order."),
        field("midiVoiceOrder", "const uint8_t*", "Newest-first unique MIDI voice order."),
        field("midiNotePressCount", "int32_t", "Valid press-order entries."),
        field("midiNoteReleaseCount", "int32_t", "Valid release-order entries."),
        field("midiVoiceCount", "int32_t", "Valid voice-order entries."),
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
        field("midiNoteOn", "const uint8_t*", "MIDI note-on state array."),
        field("midiCCValue", "const uint8_t*", "MIDI controller value array."),
        field("sampleWhenMidiNoteOn", "const uint64_t*", "Sample positions for MIDI note presses."),
        field("sampleWhenMidiNoteOff", "const uint64_t*", "Sample positions for MIDI note releases."),
        field("sampleWhenCCValueChanged", "const uint64_t*", "Sample positions for MIDI controller changes."),
        field("midiNotePressOrder", "const uint8_t*", "Newest-first unique MIDI note press order."),
        field("midiNoteReleaseOrder", "const uint8_t*", "Newest-first unique MIDI note release order."),
        field("midiVoiceOrder", "const uint8_t*", "Newest-first unique MIDI voice order."),
        field("midiNotePressCount", "int32_t", "Valid press-order entries."),
        field("midiNoteReleaseCount", "int32_t", "Valid release-order entries."),
        field("midiVoiceCount", "int32_t", "Valid voice-order entries."),
        field("sustain", "bool", "Sustain pedal state."),
        field("currentFrequencyData", "const WaviateComplex**", "Current frequency output data."),
        field("inputDeviceData", "const WaviateComplex**", "Input frequency data."),
        field("inputSideChainFrequencyData", "const WaviateComplex**", "Sidechain frequency data."),
        field("sampleRate", "float", "Sample rate in hertz."),
        field("samplesSinceAppStart", "uint64_t", "Samples since app start."),
    };

    return symbols;
}

const std::vector<FunctionSymbol>& waviateComplexMemberFunctions()
{
    static const std::vector<FunctionSymbol> symbols {
        function("real", "float", {}, SymbolKind::Method, "Real component."),
        function("imaginary", "float", {}, SymbolKind::Method, "Imaginary component."),
        function("norm", "float", {}, SymbolKind::Method, "Squared magnitude."),
        function("magnitude", "float", {}, SymbolKind::Method, "Magnitude."),
        function("phase", "float", {}, SymbolKind::Method, "Phase in radians."),
        function("conjugate", "WaviateComplex", {}, SymbolKind::Method, "Complex conjugate."),
    };

    return symbols;
}

const std::vector<FieldSymbol>& cppBuiltinTypes()
{
    static const std::vector<FieldSymbol> symbols {
        classSymbol("WaviateSample", "Sample shader context."),
        classSymbol("WaviateFrequency", "Frequency shader context."),
        classSymbol("WaviateCore", "Shared oscillator and utility API."),
        classSymbol("WaviateCore::MidiVoice", "A relevant MIDI voice."),
        classSymbol("WaviateCore::MidiVoices", "A bounded iterable MIDI voice view."),
        classSymbol("WaviateComplex", "Complex number with accessor-based components."),
        classSymbol("WaviateArray", "Temporary arena-backed array wrapper."),
        classSymbol("WaviateVector", "Temporary arena-backed vector wrapper."),
        classSymbol("WaviateString", "Temporary arena-backed string wrapper."),
        classSymbol("WaviateMap", "Temporary arena-backed map wrapper."),
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
        typeSymbol("size_t"),
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
        function("SampleProcess", "float", { param("WaviateSample&", "wav") }, SymbolKind::Function, "C++ sample shader entry point."),
        function("FrequencyProcess", "WaviateComplex", { param("WaviateFrequency&", "wav") }, SymbolKind::Function, "C++ frequency shader entry point."),
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
    std::string api;
    api.reserve(32768);
    api.append(R"waviate_cpp_api(
#ifndef WAVIATE_SCRIPT_INPUT_API_DEFINED
#define WAVIATE_SCRIPT_INPUT_API_DEFINED

class WaviateSample;
class WaviateFrequency;
using uint8_t = unsigned char;
using int32_t = int;
using uint32_t = unsigned int;
using uint64_t = unsigned long long;
using size_t = __SIZE_TYPE__;

struct WaviateSampleInput {
private:
    friend class WaviateSample;
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
    const uint8_t* midiNotePressOrder;
    const uint8_t* midiNoteReleaseOrder;
    const uint8_t* midiVoiceOrder;
    int32_t midiNotePressCount;
    int32_t midiNoteReleaseCount;
    int32_t midiVoiceCount;
    bool sustain;
    bool* sustainDefer;

    int32_t controllerCount;
    uint64_t* controllerButtonMask;
    uint64_t* sampleWhenControllerButtonChanged;
    int32_t controllerButtonCount;
    float* controllerAxisValue;
    uint64_t* sampleWhenControllerAxisChanged;
    int32_t controllerAxisCount;

    const char* const* oscStrings;
    const int32_t* oscInts;
    const uint32_t* oscColors;
    const float* oscFloats;

    float sampleRate;
    float** previousSamples;
    const float* const* inputDeviceSamples;
    const float* const* inputSideChainSamples;
    float* const* currentSampleData;
};

struct WaviateSampleStateWriter {};

template <typename T>
struct WaviateBasicComplex {
    constexpr WaviateBasicComplex(T realIn = T{}, T imaginaryIn = T{}) : realValue(realIn), imaginaryValue(imaginaryIn) {}
    constexpr T real() const { return realValue; }
    constexpr T imaginary() const { return imaginaryValue; }
    constexpr WaviateBasicComplex operator+(WaviateBasicComplex rhs) const { return { realValue + rhs.realValue, imaginaryValue + rhs.imaginaryValue }; }
    constexpr WaviateBasicComplex operator-(WaviateBasicComplex rhs) const { return { realValue - rhs.realValue, imaginaryValue - rhs.imaginaryValue }; }
    constexpr WaviateBasicComplex operator*(WaviateBasicComplex rhs) const {
        return { realValue * rhs.realValue - imaginaryValue * rhs.imaginaryValue, realValue * rhs.imaginaryValue + imaginaryValue * rhs.realValue };
    }
    constexpr WaviateBasicComplex operator/(WaviateBasicComplex rhs) const {
        const T d = rhs.realValue * rhs.realValue + rhs.imaginaryValue * rhs.imaginaryValue;
        return { (realValue * rhs.realValue + imaginaryValue * rhs.imaginaryValue) / d, (imaginaryValue * rhs.realValue - realValue * rhs.imaginaryValue) / d };
    }
    constexpr WaviateBasicComplex operator*(T scalar) const { return { realValue * scalar, imaginaryValue * scalar }; }
    constexpr WaviateBasicComplex operator/(T scalar) const { return { realValue / scalar, imaginaryValue / scalar }; }
    constexpr WaviateBasicComplex conjugate() const { return { realValue, -imaginaryValue }; }
    constexpr T norm() const { return realValue * realValue + imaginaryValue * imaginaryValue; }
    T magnitude() const { return __builtin_sqrt(norm()); }
    T phase() const { return __builtin_atan2(imaginaryValue, realValue); }
private:
    T realValue;
    T imaginaryValue;
};
using fcomplex = WaviateBasicComplex<float>;
using dcomplex = WaviateBasicComplex<double>;
using WaviateComplex = fcomplex;

struct WaviateFrequencyInput {
private:
    friend class WaviateFrequency;
    int32_t sampleWidth;
    int32_t bin;
    int32_t totalBinCount;
    int32_t channelCount;
    uint8_t channel;

    const uint8_t* midiNoteOn;
    const uint8_t* midiCCValue;
    const uint64_t* sampleWhenMidiNoteOn;
    const uint64_t* sampleWhenMidiNoteOff;
    const uint64_t* sampleWhenCCValueChanged;
    const uint8_t* midiNotePressOrder;
    const uint8_t* midiNoteReleaseOrder;
    const uint8_t* midiVoiceOrder;
    int32_t midiNotePressCount;
    int32_t midiNoteReleaseCount;
    int32_t midiVoiceCount;
    bool sustain;

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

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
inline double acos(double x) { return __builtin_acos(x); }
inline float acosf(float x) { return __builtin_acosf(x); }
inline double asin(double x) { return __builtin_asin(x); }
inline float asinf(float x) { return __builtin_asinf(x); }
inline double atan(double x) { return __builtin_atan(x); }
inline float atanf(float x) { return __builtin_atanf(x); }
inline double atan2(double y, double x) { return __builtin_atan2(y, x); }
inline float atan2f(float y, float x) { return __builtin_atan2f(y, x); }
inline double cos(double x) { return __builtin_cos(x); }
inline float cosf(float x) { return __builtin_cosf(x); }
inline double sin(double x) { return __builtin_sin(x); }
inline float sinf(float x) { return __builtin_sinf(x); }
inline double tan(double x) { return __builtin_tan(x); }
inline float tanf(float x) { return __builtin_tanf(x); }
inline double acosh(double x) { return __builtin_acosh(x); }
inline float acoshf(float x) { return __builtin_acoshf(x); }
inline double asinh(double x) { return __builtin_asinh(x); }
inline float asinhf(float x) { return __builtin_asinhf(x); }
inline double atanh(double x) { return __builtin_atanh(x); }
inline float atanhf(float x) { return __builtin_atanhf(x); }
inline double cosh(double x) { return __builtin_cosh(x); }
inline float coshf(float x) { return __builtin_coshf(x); }
inline double sinh(double x) { return __builtin_sinh(x); }
inline float sinhf(float x) { return __builtin_sinhf(x); }
inline double tanh(double x) { return __builtin_tanh(x); }
inline float tanhf(float x) { return __builtin_tanhf(x); }
inline double exp(double x) { return __builtin_exp(x); }
inline float expf(float x) { return __builtin_expf(x); }
inline double exp2(double x) { return __builtin_exp2(x); }
inline float exp2f(float x) { return __builtin_exp2f(x); }
inline double expm1(double x) { return __builtin_expm1(x); }
inline float expm1f(float x) { return __builtin_expm1f(x); }
inline double frexp(double x, int* exponent) { return __builtin_frexp(x, exponent); }
inline float frexpf(float x, int* exponent) { return __builtin_frexpf(x, exponent); }
inline int ilogb(double x) { return __builtin_ilogb(x); }
inline int ilogbf(float x) { return __builtin_ilogbf(x); }
inline double ldexp(double x, int exponent) { return __builtin_ldexp(x, exponent); }
inline float ldexpf(float x, int exponent) { return __builtin_ldexpf(x, exponent); }
inline double log(double x) { return __builtin_log(x); }
inline float logf(float x) { return __builtin_logf(x); }
inline double log10(double x) { return __builtin_log10(x); }
inline float log10f(float x) { return __builtin_log10f(x); }
inline double log1p(double x) { return __builtin_log1p(x); }
inline float log1pf(float x) { return __builtin_log1pf(x); }
inline double log2(double x) { return __builtin_log2(x); }
inline float log2f(float x) { return __builtin_log2f(x); }
inline double logb(double x) { return __builtin_logb(x); }
inline float logbf(float x) { return __builtin_logbf(x); }
inline double modf(double x, double* integerPart) { return __builtin_modf(x, integerPart); }
inline float modff(float x, float* integerPart) { return __builtin_modff(x, integerPart); }
inline double scalbn(double x, int exponent) { return __builtin_scalbn(x, exponent); }
inline float scalbnf(float x, int exponent) { return __builtin_scalbnf(x, exponent); }
inline double scalbln(double x, long exponent) { return __builtin_scalbln(x, exponent); }
inline float scalblnf(float x, long exponent) { return __builtin_scalblnf(x, exponent); }
inline double cbrt(double x) { return __builtin_cbrt(x); }
inline float cbrtf(float x) { return __builtin_cbrtf(x); }
inline double fabs(double x) { return __builtin_fabs(x); }
inline float fabsf(float x) { return __builtin_fabsf(x); }
inline double hypot(double x, double y) { return __builtin_hypot(x, y); }
inline float hypotf(float x, float y) { return __builtin_hypotf(x, y); }
inline double pow(double base, double exponent) { return __builtin_pow(base, exponent); }
inline float powf(float base, float exponent) { return __builtin_powf(base, exponent); }
inline double sqrt(double x) { return __builtin_sqrt(x); }
inline float sqrtf(float x) { return __builtin_sqrtf(x); }
inline double erf(double x) { return __builtin_erf(x); }
inline float erff(float x) { return __builtin_erff(x); }
inline double erfc(double x) { return __builtin_erfc(x); }
inline float erfcf(float x) { return __builtin_erfcf(x); }
inline double lgamma(double x) { return __builtin_lgamma(x); }
inline float lgammaf(float x) { return __builtin_lgammaf(x); }
inline double tgamma(double x) { return __builtin_tgamma(x); }
inline float tgammaf(float x) { return __builtin_tgammaf(x); }
inline double ceil(double x) { return __builtin_ceil(x); }
inline float ceilf(float x) { return __builtin_ceilf(x); }
inline double floor(double x) { return __builtin_floor(x); }
inline float floorf(float x) { return __builtin_floorf(x); }
inline double nearbyint(double x) { return __builtin_nearbyint(x); }
inline float nearbyintf(float x) { return __builtin_nearbyintf(x); }
inline double rint(double x) { return __builtin_rint(x); }
inline float rintf(float x) { return __builtin_rintf(x); }
inline long lrint(double x) { return __builtin_lrint(x); }
inline long lrintf(float x) { return __builtin_lrintf(x); }
inline long long llrint(double x) { return __builtin_llrint(x); }
inline long long llrintf(float x) { return __builtin_llrintf(x); }
inline double round(double x) { return __builtin_round(x); }
inline float roundf(float x) { return __builtin_roundf(x); }
inline long lround(double x) { return __builtin_lround(x); }
inline long lroundf(float x) { return __builtin_lroundf(x); }
inline long long llround(double x) { return __builtin_llround(x); }
inline long long llroundf(float x) { return __builtin_llroundf(x); }
inline double trunc(double x) { return __builtin_trunc(x); }
inline float truncf(float x) { return __builtin_truncf(x); }
inline double fmod(double x, double y) { return __builtin_fmod(x, y); }
inline float fmodf(float x, float y) { return __builtin_fmodf(x, y); }
inline double remainder(double x, double y) { return __builtin_remainder(x, y); }
inline float remainderf(float x, float y) { return __builtin_remainderf(x, y); }
inline double remquo(double x, double y, int* quotient) { return __builtin_remquo(x, y, quotient); }
inline float remquof(float x, float y, int* quotient) { return __builtin_remquof(x, y, quotient); }
inline double copysign(double magnitude, double sign) { return __builtin_copysign(magnitude, sign); }
inline float copysignf(float magnitude, float sign) { return __builtin_copysignf(magnitude, sign); }
inline double nan(const char* tagp) { return __builtin_nan(tagp); }
inline float nanf(const char* tagp) { return __builtin_nanf(tagp); }
inline double nextafter(double from, double to) { return __builtin_nextafter(from, to); }
inline float nextafterf(float from, float to) { return __builtin_nextafterf(from, to); }
inline double nexttoward(double from, long double to) { return __builtin_nexttoward(from, to); }
inline float nexttowardf(float from, long double to) { return __builtin_nexttowardf(from, to); }
inline double fmax(double x, double y) { return __builtin_fmax(x, y); }
inline float fmaxf(float x, float y) { return __builtin_fmaxf(x, y); }
inline double fmin(double x, double y) { return __builtin_fmin(x, y); }
inline float fminf(float x, float y) { return __builtin_fminf(x, y); }
inline double fdim(double x, double y) { return __builtin_fdim(x, y); }
inline float fdimf(float x, float y) { return __builtin_fdimf(x, y); }
inline double fma(double x, double y, double z) { return __builtin_fma(x, y, z); }
inline float fmaf(float x, float y, float z) { return __builtin_fmaf(x, y, z); }
inline int fpclassify(double x) { return __builtin_fpclassify(0, 1, 2, 3, 4, x); }
inline bool isfinite(double x) { return __builtin_isfinite(x) != 0; }
inline bool isinf(double x) { return __builtin_isinf(x) != 0; }
inline bool isnan(double x) { return __builtin_isnan(x) != 0; }
inline bool isnormal(double x) { return __builtin_isnormal(x) != 0; }
inline bool signbit(double x) { return __builtin_signbit(x) != 0; }

namespace waviate_complex {
    template <typename T> inline T abs(WaviateBasicComplex<T> v) { return __builtin_sqrt(v.norm()); }
    template <typename T> inline T arg(WaviateBasicComplex<T> v) { return __builtin_atan2(v.imaginary(), v.real()); }
    template <typename T> inline WaviateBasicComplex<T> conj(WaviateBasicComplex<T> v) { return v.conjugate(); }
    template <typename T> inline WaviateBasicComplex<T> polar(T magnitude, T phase) {
        return { static_cast<T>(magnitude * __builtin_cos(phase)), static_cast<T>(magnitude * __builtin_sin(phase)) };
    }
    template <typename T> inline WaviateBasicComplex<T> exp(WaviateBasicComplex<T> v) {
        const T scale = __builtin_exp(v.real());
        return { static_cast<T>(scale * __builtin_cos(v.imaginary())), static_cast<T>(scale * __builtin_sin(v.imaginary())) };
    }
    template <typename T> inline WaviateBasicComplex<T> log(WaviateBasicComplex<T> v) {
        return { static_cast<T>(__builtin_log(abs(v))), arg(v) };
    }
    template <typename T> inline WaviateBasicComplex<T> pow(WaviateBasicComplex<T> base, T exponent) {
        return polar(__builtin_pow(abs(base), exponent), arg(base) * exponent);
    }
    template <typename T> inline WaviateBasicComplex<T> sin(WaviateBasicComplex<T> v) {
        return { static_cast<T>(__builtin_sin(v.real()) * __builtin_cosh(v.imaginary())), static_cast<T>(__builtin_cos(v.real()) * __builtin_sinh(v.imaginary())) };
    }
    template <typename T> inline WaviateBasicComplex<T> cos(WaviateBasicComplex<T> v) {
        return { static_cast<T>(__builtin_cos(v.real()) * __builtin_cosh(v.imaginary())), static_cast<T>(-__builtin_sin(v.real()) * __builtin_sinh(v.imaginary())) };
    }
    template <typename T> inline WaviateBasicComplex<T> tan(WaviateBasicComplex<T> v) { return sin(v) / cos(v); }
}

inline constexpr double HUGE_VAL = __builtin_huge_val();
inline constexpr float HUGE_VALF = __builtin_huge_valf();
inline constexpr long double HUGE_VALL = __builtin_huge_vall();
inline constexpr float INFINITY = __builtin_inff();
inline constexpr float NAN = __builtin_nanf("");
inline constexpr int FP_NAN = 0;
inline constexpr int FP_INFINITE = 1;
inline constexpr int FP_ZERO = 2;
inline constexpr int FP_SUBNORMAL = 3;
inline constexpr int FP_NORMAL = 4;

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
namespace waviate_detail {
    inline constexpr float pi = 3.14159265358979323846f;
    inline constexpr float twoPi = 6.28318530717958647692f;
    inline constexpr float oneThird = 0.33333333333333333333f;

    inline float wavSin(float x) { return __builtin_sinf(x); }
    inline float wavTan(float x) { return __builtin_tanf(x); }
    inline float wavFloor(float x) { return __builtin_floorf(x); }
    inline float wavAbs(float x) { return __builtin_fabsf(x); }
    inline float wavSqrt(float x) { return __builtin_sqrtf(x); }
    inline float wavLog2(float x) { return __builtin_log2f(x); }

    inline float minValue(float a, float b) { return a < b ? a : b; }
    inline float maxValue(float a, float b) { return a > b ? a : b; }
    inline int clampInt(int value, int low, int high) {
        return value < low ? low : (value > high ? high : value);
    }
    inline float clamp01(float x) {
        return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
    }
    inline float fract(float x) { return x - wavFloor(x); }
    inline int fastFloor(float x) {
        const int i = static_cast<int>(x);
        return static_cast<float>(i) > x ? i - 1 : i;
    }
    inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
    inline float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    inline float wrapSigned(float x) { return 2.0f * fract(0.5f * (x + 1.0f)) - 1.0f; }
    inline float foldSigned(float x) {
        const float wrapped = fract(0.25f * (x + 1.0f));
        const float folded = wrapped < 0.5f ? wrapped : 1.0f - wrapped;
        return 4.0f * folded - 1.0f;
    }
    inline uint32_t hashBits(int cell) {
        uint32_t h = static_cast<uint32_t>(cell);
        h ^= h >> 16;
        h *= 0x7feb352dU;
        h ^= h >> 15;
        h *= 0x846ca68bU;
        h ^= h >> 16;
        return h;
    }
    inline float hash01(int cell) {
        return static_cast<float>(hashBits(cell) & 0x00ffffffU) * (1.0f / 16777215.0f);
    }
    inline float gradient(int cell) {
        return (hashBits(cell) & 1U) != 0U ? 1.0f : -1.0f;
    }
}

inline float phase(float x) { return waviate_detail::fract(x); }
inline float sine(float x) { return waviate_detail::wavSin(waviate_detail::twoPi * phase(x)); }
inline float saw(float x) { return 2.0f * phase(x) - 1.0f; }
inline float square(float x) { return phase(x) < 0.5f ? 1.0f : -1.0f; }
inline float pulse(float x, float width = 0.5f) { return phase(x) < waviate_detail::clamp01(width) ? 1.0f : -1.0f; }
inline float triangle(float x) { return 1.0f - 4.0f * waviate_detail::wavAbs(phase(x) - 0.5f); }
inline float semicircle(float x) {
    const float centered = 2.0f * phase(x) - 1.0f;
    return 2.0f * waviate_detail::wavSqrt(waviate_detail::maxValue(0.0f, 1.0f - centered * centered)) - 1.0f;
}
inline float sawTan(float x) { return waviate_detail::wrapSigned(waviate_detail::wavTan(waviate_detail::pi * (phase(x) - 0.5f))); }
inline float triangleTan(float x) { return waviate_detail::foldSigned(waviate_detail::wavTan(waviate_detail::pi * (phase(x) - 0.5f))); }
inline float strongSine(float x) { return 0.75f * (sine(x) + waviate_detail::oneThird * sine(3.0f * x)); }
inline float fractalSquare(float x) {
    const float p = phase(x);
    if (p < 0.5f)
        return 1.0f;

    const float remaining = waviate_detail::maxValue(0.00000011920928955f, 1.0f - p);
    const int band = static_cast<int>(waviate_detail::wavFloor(-waviate_detail::wavLog2(remaining)));
    return (band & 1) == 0 ? 1.0f : -1.0f;
}

inline float perlin(float x, float min = 0.0f, float max = 1.0f) {
    const int cell = waviate_detail::fastFloor(x);
    const float t = x - static_cast<float>(cell);
    const float u = waviate_detail::fade(t);
    const float a = waviate_detail::gradient(cell) * t;
    const float b = waviate_detail::gradient(cell + 1) * (t - 1.0f);
    const float value = waviate_detail::clamp01(0.5f + waviate_detail::lerp(a, b, u));
    return min + value * (max - min);
}

inline float simplex(float x, float min = 0.0f, float max = 1.0f) {
    const int cell = waviate_detail::fastFloor(x);
    const float x0 = x - static_cast<float>(cell);
    const float x1 = x0 - 1.0f;

    float t0 = 1.0f - x0 * x0;
    t0 *= t0;
    const float n0 = t0 * t0 * waviate_detail::gradient(cell) * x0;

    float t1 = 1.0f - x1 * x1;
    t1 *= t1;
    const float n1 = t1 * t1 * waviate_detail::gradient(cell + 1) * x1;

    const float value = waviate_detail::clamp01(0.5f + 4.0f * (n0 + n1));
    return min + value * (max - min);
}

inline float voronoi(float x, float min = 0.0f, float max = 1.0f) {
    const int cell = waviate_detail::fastFloor(x);
    float nearest = 2.0f;

    for (int offset = -1; offset <= 1; ++offset) {
        const int neighbour = cell + offset;
        const float feature = static_cast<float>(neighbour) + waviate_detail::hash01(neighbour);
        nearest = waviate_detail::minValue(nearest, waviate_detail::wavAbs(x - feature));
    }

    const float value = waviate_detail::clamp01(1.0f - nearest);
    return min + value * (max - min);
}

inline float turbulence(float x, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f, float min = 0.0f, float max = 1.0f) {
    float sum = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float normalizer = 0.0f;
    const int count = waviate_detail::clampInt(octaves, 1, 8);

    for (int i = 0; i < count; ++i) {
        sum += amplitude * waviate_detail::wavAbs(2.0f * perlin(x * frequency, 0.0f, 1.0f) - 1.0f);
        normalizer += amplitude;
        frequency *= waviate_detail::maxValue(0.0001f, lacunarity);
        amplitude *= waviate_detail::clamp01(gain);
    }

    const float value = normalizer > 0.0f ? waviate_detail::clamp01(sum / normalizer) : 0.0f;
    return min + value * (max - min);
}

inline float ridgedMulti(float x, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f, float min = 0.0f, float max = 1.0f) {
    float sum = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float normalizer = 0.0f;
    const int count = waviate_detail::clampInt(octaves, 1, 8);

    for (int i = 0; i < count; ++i) {
        const float ridge = 1.0f - waviate_detail::wavAbs(2.0f * perlin(x * frequency, 0.0f, 1.0f) - 1.0f);
        sum += amplitude * ridge * ridge;
        normalizer += amplitude;
        frequency *= waviate_detail::maxValue(0.0001f, lacunarity);
        amplitude *= waviate_detail::clamp01(gain);
    }

    const float value = normalizer > 0.0f ? waviate_detail::clamp01(sum / normalizer) : 0.0f;
    return min + value * (max - min);
}

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
enum class WaviateAudioAddressMode : uint32_t {
    Clamp = 0,
    Wrap = 1,
    Reflect = 2
};

enum class WaviateAudioInterpolation : uint32_t {
    Nearest = 0,
    Linear = 1,
    Cubic = 2
};

struct WaviateAudioRuntimeClip {
    const float* samples;
    uint64_t frameCount;
    int32_t channelCount;
    float sampleRate;
};

extern "C" const WaviateAudioRuntimeClip* waviate_load_audio_from_location(const char* location);

class WaviateAudio {
public:
    WaviateAudio() = default;
    explicit WaviateAudio(const WaviateAudioRuntimeClip* clipIn) : clip(clipIn) {}

    bool isLoaded() const {
        return clip != nullptr
            && clip->samples != nullptr
            && clip->frameCount > 0
            && clip->channelCount > 0;
    }

    uint64_t length() const { return isLoaded() ? clip->frameCount : 1ULL; }
    int channelCount() const { return isLoaded() ? clip->channelCount : 1; }
    float sampleRateHz() const { return isLoaded() ? clip->sampleRate : 0.0f; }

    float readSample(uint64_t sampleIndex, int channel = 0) const {
        if (! isLoaded()
            || channel < 0
            || channel >= clip->channelCount
            || sampleIndex >= clip->frameCount) {
            return 0.0f;
        }

        const uint64_t offset = sampleIndex * static_cast<uint64_t>(clip->channelCount)
            + static_cast<uint64_t>(channel);
        return clip->samples[offset];
    }

    float read(float x,
               WaviateAudioAddressMode addressMode = WaviateAudioAddressMode::Clamp,
               WaviateAudioInterpolation interpolation = WaviateAudioInterpolation::Linear,
               int channel = 0) const {
        if (! isLoaded())
            return 0.0f;

        const float addressed = addressCoordinate(x, addressMode);
        const float position = addressed * static_cast<float>(clip->frameCount > 1 ? clip->frameCount - 1 : 0);

        if (interpolation == WaviateAudioInterpolation::Nearest)
            return sampleAt(static_cast<long long>(position + 0.5f), channel, addressMode);

        const long long baseIndex = static_cast<long long>(waviate_detail::wavFloor(position));
        const float t = position - static_cast<float>(baseIndex);

        if (interpolation == WaviateAudioInterpolation::Cubic)
        {
            const float p0 = sampleAt(baseIndex - 1, channel, addressMode);
            const float p1 = sampleAt(baseIndex, channel, addressMode);
            const float p2 = sampleAt(baseIndex + 1, channel, addressMode);
            const float p3 = sampleAt(baseIndex + 2, channel, addressMode);
            return cubic(p0, p1, p2, p3, t);
        }

        const float a = sampleAt(baseIndex, channel, addressMode);
        const float b = sampleAt(baseIndex + 1, channel, addressMode);
        return waviate_detail::lerp(a, b, t);
    }

private:
    const WaviateAudioRuntimeClip* clip = nullptr;

    static float addressCoordinate(float x, WaviateAudioAddressMode mode) {
        if (mode == WaviateAudioAddressMode::Wrap)
            return waviate_detail::fract(x);

        if (mode == WaviateAudioAddressMode::Reflect)
        {
            const float period = x - waviate_detail::wavFloor(x * 0.5f) * 2.0f;
            return period <= 1.0f ? period : 2.0f - period;
        }

        return waviate_detail::clamp01(x);
    }

    uint64_t addressIndex(long long index, WaviateAudioAddressMode mode) const {
        if (clip->frameCount <= 1)
            return 0ULL;

        if (mode == WaviateAudioAddressMode::Wrap)
        {
            const long long lengthIn = static_cast<long long>(clip->frameCount);
            long long wrapped = index % lengthIn;
            if (wrapped < 0)
                wrapped += lengthIn;
            return static_cast<uint64_t>(wrapped);
        }

        if (mode == WaviateAudioAddressMode::Reflect)
        {
            const long long lengthIn = static_cast<long long>(clip->frameCount);
            const long long period = lengthIn * 2 - 2;
            long long reflected = index % period;
            if (reflected < 0)
                reflected += period;
            if (reflected >= lengthIn)
                reflected = period - reflected;
            return static_cast<uint64_t>(reflected);
        }

        if (index <= 0)
            return 0ULL;

        const auto maxIndex = static_cast<long long>(clip->frameCount - 1);
        return static_cast<uint64_t>(index >= maxIndex ? maxIndex : index);
    }

    float sampleAt(long long index, int channel, WaviateAudioAddressMode mode) const {
        if (channel < 0 || channel >= clip->channelCount)
            return 0.0f;

        return readSample(addressIndex(index, mode), channel);
    }

    static float cubic(float p0, float p1, float p2, float p3, float t) {
        const float t2 = t * t;
        const float t3 = t2 * t;
        return 0.5f * ((2.0f * p1)
            + (-p0 + p2) * t
            + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
            + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }
};

inline WaviateAudio loadAudio(const char* fileLocation) {
    return WaviateAudio(waviate_load_audio_from_location(fileLocation));
}

inline WaviateAudio loadAudio(char* fileLocation) {
    return loadAudio(static_cast<const char*>(fileLocation));
}

template <typename StringLike>
inline auto loadAudio(const StringLike& fileLocation) -> decltype(fileLocation.c_str(), WaviateAudio()) {
    return loadAudio(fileLocation.c_str());
}

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
extern "C" void waviate_fuel_trap() noexcept;
extern "C" void* __waviate_internal_arena_allocate(uint64_t sizeBytes, uint64_t alignmentBytes) noexcept;
extern "C" uint64_t __waviate_internal_arena_generation() noexcept;

namespace waviate_detail {
    inline size_t maxSize() {
        return static_cast<size_t>(~static_cast<size_t>(0));
    }

    inline bool multiplyWouldOverflow(size_t a, size_t b) {
        return a != 0 && b > maxSize() / a;
    }

    inline void trapArenaFailure() {
        waviate_fuel_trap();
    }

    template <typename T>
    inline T* arenaAllocate(size_t count) {
        static_assert(__is_trivially_destructible(T),
                      "Waviate arena containers only support trivially destructible values");

        if (count == 0)
            return nullptr;

        if (multiplyWouldOverflow(count, sizeof(T))) {
            trapArenaFailure();
            return nullptr;
        }

        return static_cast<T*>(__waviate_internal_arena_allocate(
            static_cast<uint64_t>(count * sizeof(T)),
            static_cast<uint64_t>(alignof(T))));
    }

    inline uint64_t arenaGeneration() {
        return __waviate_internal_arena_generation();
    }
}

template <typename T>
class WaviateArray {
public:
    static WaviateArray create(size_t sizeIn) {
        WaviateArray result;
        result.items = waviate_detail::arenaAllocate<T>(sizeIn);
        result.allocationOk = sizeIn == 0 || result.items != nullptr;
        result.count = result.allocationOk ? sizeIn : 0;
        result.generation = waviate_detail::arenaGeneration();
        return result;
    }

    size_t size() const { return isCurrentPass() ? count : 0; }
    bool empty() const { return size() == 0; }
    bool valid() const { return isCurrentPass() && allocationOk; }

    T get(size_t index, T fallback = T{}) const {
        return valid() && index < count ? items[index] : fallback;
    }

    bool set(size_t index, T value) {
        if (!valid() || index >= count) {
            waviate_detail::trapArenaFailure();
            return false;
        }

        items[index] = value;
        return true;
    }

private:
    bool isCurrentPass() const { return generation == waviate_detail::arenaGeneration(); }

    T* items = nullptr;
    size_t count = 0;
    uint64_t generation = 0;
    bool allocationOk = true;
};

template <typename T>
class WaviateVector {
public:
    static WaviateVector create(size_t initialCapacity) {
        WaviateVector result;
        result.generation = waviate_detail::arenaGeneration();
        if (initialCapacity > 0)
            result.reserve(initialCapacity);
        return result;
    }

    size_t size() const { return isCurrentPass() ? count : 0; }
    size_t capacity() const { return isCurrentPass() ? reserved : 0; }
    bool empty() const { return size() == 0; }
    bool valid() const { return isCurrentPass(); }

    bool reserve(size_t minCapacity) {
        if (!isCurrentPass()) {
            waviate_detail::trapArenaFailure();
            return false;
        }

        if (minCapacity <= reserved)
            return true;

        T* next = waviate_detail::arenaAllocate<T>(minCapacity);
        if (next == nullptr && minCapacity > 0)
            return false;

        for (size_t i = 0; i < count; ++i)
            next[i] = items[i];

        items = next;
        reserved = minCapacity;
        return true;
    }

    bool push(T value) {
        if (count == reserved) {
            const size_t nextCapacity = reserved == 0 ? 8 : reserved * 2;
            if (nextCapacity < reserved || !reserve(nextCapacity))
                return false;
        }

        items[count++] = value;
        return true;
    }

    T get(size_t index, T fallback = T{}) const {
        return valid() && index < count ? items[index] : fallback;
    }

    bool set(size_t index, T value) {
        if (!valid() || index >= count) {
            waviate_detail::trapArenaFailure();
            return false;
        }

        items[index] = value;
        return true;
    }

private:
    bool isCurrentPass() const { return generation == waviate_detail::arenaGeneration(); }

    T* items = nullptr;
    size_t count = 0;
    size_t reserved = 0;
    uint64_t generation = 0;
};

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
class WaviateString {
public:
    static WaviateString create() {
        WaviateString result;
        result.generation = waviate_detail::arenaGeneration();
        return result;
    }

    static WaviateString create(const char* text) {
        WaviateString result = create();
        result.append(text);
        return result;
    }

    size_t size() const { return isCurrentPass() ? count : 0; }
    bool empty() const { return size() == 0; }
    bool valid() const { return isCurrentPass(); }
    const char* c_str() const { return valid() && chars != nullptr ? chars : ""; }

    char get(size_t index, char fallback = '\0') const {
        return valid() && index < count ? chars[index] : fallback;
    }

    bool reserve(size_t minCapacity) {
        if (!isCurrentPass()) {
            waviate_detail::trapArenaFailure();
            return false;
        }

        if (minCapacity <= reserved)
            return true;

        if (minCapacity == waviate_detail::maxSize()) {
            waviate_detail::trapArenaFailure();
            return false;
        }

        char* next = waviate_detail::arenaAllocate<char>(minCapacity + 1);
        if (next == nullptr)
            return false;

        for (size_t i = 0; i < count; ++i)
            next[i] = chars[i];

        next[count] = '\0';
        chars = next;
        reserved = minCapacity;
        return true;
    }

    bool append(char value) {
        if (count == reserved) {
            const size_t nextCapacity = reserved == 0 ? 16 : reserved * 2;
            if (nextCapacity < reserved || !reserve(nextCapacity))
                return false;
        }

        chars[count++] = value;
        chars[count] = '\0';
        return true;
    }

    bool append(const char* text) {
        if (text == nullptr)
            return true;

        for (size_t i = 0; text[i] != '\0'; ++i)
            if (!append(text[i]))
                return false;

        return true;
    }

private:
    bool isCurrentPass() const { return generation == waviate_detail::arenaGeneration(); }

    char* chars = nullptr;
    size_t count = 0;
    size_t reserved = 0;
    uint64_t generation = 0;
};

template <typename K, typename V>
class WaviateMap {
private:
    struct Entry {
        K key;
        V value;
        uint8_t used;
    };

public:
    static WaviateMap create(size_t initialCapacity) {
        static_assert(__is_trivially_destructible(K),
                      "Waviate arena maps only support trivially destructible keys");
        static_assert(__is_trivially_destructible(V),
                      "Waviate arena maps only support trivially destructible values");

        WaviateMap result;
        result.generation = waviate_detail::arenaGeneration();
        if (initialCapacity > 0)
            result.reserve(initialCapacity);
        return result;
    }

    size_t size() const { return isCurrentPass() ? count : 0; }
    size_t capacity() const { return isCurrentPass() ? reserved : 0; }
    bool valid() const { return isCurrentPass(); }

    bool reserve(size_t minCapacity) {
        if (!isCurrentPass()) {
            waviate_detail::trapArenaFailure();
            return false;
        }

        if (minCapacity <= reserved)
            return true;

        Entry* next = waviate_detail::arenaAllocate<Entry>(minCapacity);
        if (next == nullptr && minCapacity > 0)
            return false;

        for (size_t i = 0; i < count; ++i)
            next[i] = entries[i];

        entries = next;
        reserved = minCapacity;
        return true;
    }

    bool insert(K key, V value) {
        if (!isCurrentPass()) {
            waviate_detail::trapArenaFailure();
            return false;
        }

        for (size_t i = 0; i < count; ++i) {
            if (entries[i].used != 0 && entries[i].key == key) {
                entries[i].value = value;
                return true;
            }
        }

        if (count == reserved) {
            const size_t nextCapacity = reserved == 0 ? 8 : reserved * 2;
            if (nextCapacity < reserved || !reserve(nextCapacity))
                return false;
        }

        entries[count++] = Entry { key, value, 1 };
        return true;
    }

    bool contains(K key) const {
        if (!isCurrentPass())
            return false;

        for (size_t i = 0; i < count; ++i)
            if (entries[i].used != 0 && entries[i].key == key)
                return true;

        return false;
    }

    V get(K key, V fallback = V{}) const {
        if (!isCurrentPass())
            return fallback;

        for (size_t i = 0; i < count; ++i)
            if (entries[i].used != 0 && entries[i].key == key)
                return entries[i].value;

        return fallback;
    }

private:
    bool isCurrentPass() const { return generation == waviate_detail::arenaGeneration(); }

    Entry* entries = nullptr;
    size_t count = 0;
    size_t reserved = 0;
    uint64_t generation = 0;
};

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
class WaviateCore {
public:
    class MidiVoice;
    class MidiVoices;
    float secondsSinceAppStart() const { return samplesToSeconds(coreSamplesSinceAppStart); }
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

    bool isMidiNoteOn(int note) const { return isValidIndex(note, 128) && coreMidiNoteOn != nullptr && coreMidiNoteOn[note] != 0; }
    uint8_t midiCCValue(int controller) const { return isValidIndex(controller, 128) && coreMidiCCValue != nullptr ? coreMidiCCValue[controller] : 0; }
    int midiNotePressCount() const { return coreMidiNotePressCount; }
    int midiNoteReleaseCount() const { return coreMidiNoteReleaseCount; }
    int midiVoiceCount() const { return coreMidiVoiceCount; }
    int midiNotePressOrder(int index) const { return orderedMidiNote(coreMidiNotePressOrder, coreMidiNotePressCount, index); }
    int midiNoteReleaseOrder(int index) const { return orderedMidiNote(coreMidiNoteReleaseOrder, coreMidiNoteReleaseCount, index); }
    int midiVoiceNote(int index) const { return orderedMidiNote(coreMidiVoiceOrder, coreMidiVoiceCount, index); }
    uint64_t sampleWhenMidiNotePressed(int note) const { return isValidIndex(note, 128) && coreSampleWhenMidiNoteOn != nullptr ? coreSampleWhenMidiNoteOn[note] : 0ULL; }
    uint64_t sampleWhenMidiNoteReleased(int note) const { return isValidIndex(note, 128) && coreSampleWhenMidiNoteOff != nullptr ? coreSampleWhenMidiNoteOff[note] : 0ULL; }
    uint64_t samplesSinceMidiNotePressed(int note) const {
        const uint64_t eventSample = sampleWhenMidiNotePressed(note);
        return hasOrderedMidiNote(coreMidiNotePressOrder, coreMidiNotePressCount, note) && coreSamplesSinceAppStart >= eventSample ? coreSamplesSinceAppStart - eventSample : 0ULL;
    }
    uint64_t samplesSinceMidiNoteReleased(int note) const {
        const uint64_t eventSample = sampleWhenMidiNoteReleased(note);
        return hasOrderedMidiNote(coreMidiNoteReleaseOrder, coreMidiNoteReleaseCount, note) && coreSamplesSinceAppStart >= eventSample ? coreSamplesSinceAppStart - eventSample : 0ULL;
    }
    float midiNoteFrequency(int note) const { return 440.0f * __builtin_powf(2.0f, static_cast<float>(note - 69) / 12.0f); }
    template <typename Tuning> float midiNoteFrequency(int note, const Tuning& tuning) const { return static_cast<float>(tuning(note)); }
    float midiNotePhase(int note) const { return midiNotePhaseForFrequency(note, midiNoteFrequency(note)); }
    template <typename Tuning> float midiNotePhase(int note, const Tuning& tuning) const { return midiNotePhaseForFrequency(note, midiNoteFrequency(note, tuning)); }
    float midiNoteAdsr(int note, float attackSeconds, float decaySeconds, float sustainLevel, float releaseSeconds) const {
        if (!hasOrderedMidiNote(coreMidiNotePressOrder, coreMidiNotePressCount, note)) return 0.0f;
        const uint64_t attackSamples = secondsToSamples(attackSeconds);
        const uint64_t decaySamples = secondsToSamples(decaySeconds);
        const uint64_t releaseSamples = secondsToSamples(releaseSeconds);
        const float sustain = waviate_detail::clamp01(sustainLevel);
        const float heldLevel = envelopeHeldLevel(samplesSinceMidiNotePressed(note), attackSamples, decaySamples, sustain);
        if (isMidiNoteOn(note)) return heldLevel;
        if (!hasOrderedMidiNote(coreMidiNoteReleaseOrder, coreMidiNoteReleaseCount, note) || releaseSamples == 0) return 0.0f;
        const uint64_t heldUntilRelease = sampleWhenMidiNoteReleased(note) >= sampleWhenMidiNotePressed(note) ? sampleWhenMidiNoteReleased(note) - sampleWhenMidiNotePressed(note) : 0ULL;
        const float releaseStart = envelopeHeldLevel(heldUntilRelease, attackSamples, decaySamples, sustain);
        const double releaseRatio = static_cast<double>(samplesSinceMidiNoteReleased(note)) / static_cast<double>(releaseSamples);
        return releaseStart * (1.0f - waviate_detail::clamp01(static_cast<float>(releaseRatio)));
    }

    class MidiVoice {
    public:
        int note() const { return midiNote; }
        bool isHeld() const { return owner != nullptr && owner->isMidiNoteOn(midiNote); }
        uint64_t samplesSincePressed() const { return owner != nullptr ? owner->samplesSinceMidiNotePressed(midiNote) : 0ULL; }
        uint64_t samplesSinceReleased() const { return owner != nullptr ? owner->samplesSinceMidiNoteReleased(midiNote) : 0ULL; }
        float frequency() const { return owner != nullptr ? owner->midiNoteFrequency(midiNote) : 0.0f; }
        float phase() const { return owner != nullptr ? owner->midiNotePhase(midiNote) : 0.0f; }
        template <typename Tuning> float frequency(const Tuning& tuning) const { return owner != nullptr ? owner->midiNoteFrequency(midiNote, tuning) : 0.0f; }
        template <typename Tuning> float phase(const Tuning& tuning) const { return owner != nullptr ? owner->midiNotePhase(midiNote, tuning) : 0.0f; }
        float adsr(float attackSeconds, float decaySeconds, float sustainLevel, float releaseSeconds) const { return owner != nullptr ? owner->midiNoteAdsr(midiNote, attackSeconds, decaySeconds, sustainLevel, releaseSeconds) : 0.0f; }
    private:
        friend class MidiVoices;
        MidiVoice(const WaviateCore* ownerIn, int noteIn) : owner(ownerIn), midiNote(noteIn) {}
        const WaviateCore* owner;
        int midiNote;
    };
    class MidiVoices {
    public:
        class Iterator {
        public:
            MidiVoice operator*() const { return owner->operator[](index); }
            Iterator& operator++() { ++index; return *this; }
            bool operator!=(const Iterator& other) const { return index != other.index; }
        private:
            friend class MidiVoices;
            Iterator(const MidiVoices* ownerIn, int indexIn) : owner(ownerIn), index(indexIn) {}
            const MidiVoices* owner;
            int index;
        };
        int count() const { return voiceCount; }
        MidiVoice operator[](int index) const { return MidiVoice(owner, owner != nullptr ? owner->midiVoiceNote(index) : -1); }
        Iterator begin() const { return Iterator(this, 0); }
        Iterator end() const { return Iterator(this, voiceCount); }
    private:
        friend class WaviateCore;
        MidiVoices(const WaviateCore* ownerIn, int countIn) : owner(ownerIn), voiceCount(countIn) {}
        const WaviateCore* owner;
        int voiceCount;
    };
    MidiVoices midiVoices(int maximumVoices = 128) const { return MidiVoices(this, waviate_detail::clampInt(maximumVoices, 0, coreMidiVoiceCount)); }

    template <typename T>
    WaviateArray<T> newArray(size_t size) const { return WaviateArray<T>::create(size); }

    template <typename T>
    WaviateVector<T> newVector(size_t initialCapacity = 0) const { return WaviateVector<T>::create(initialCapacity); }

    WaviateString newString() const { return WaviateString::create(); }
    WaviateString newString(const char* text) const { return WaviateString::create(text); }

    template <typename K, typename V>
    WaviateMap<K, V> newMap(size_t initialCapacity = 0) const { return WaviateMap<K, V>::create(initialCapacity); }

    float adsr(float attack, float decay, float sustain, float release, float t) const {
        const float a = waviate_detail::maxValue(0.0f, attack);
        const float d = waviate_detail::maxValue(0.0f, decay);
        const float s = waviate_detail::clamp01(sustain);
        const float r = waviate_detail::maxValue(0.0f, release);

        if (t < 0.0f)
            return r > 0.0f ? s * (1.0f - waviate_detail::clamp01(-t / r)) : 0.0f;
        if (a > 0.0f && t < a)
            return waviate_detail::clamp01(t / a);
        if (d > 0.0f && t < a + d)
            return waviate_detail::lerp(1.0f, s, (t - a) / d);

        return s;
    }
    float ADSR(float attack, float decay, float sustain, float release, float t) const {
        return adsr(attack, decay, sustain, release, t);
    }

    float sine(float x) const { return ::sine(x); }
    float saw(float x) const { return ::saw(x); }
    float square(float x) const { return ::square(x); }
    float pulse(float x, float width = 0.5f) const { return ::pulse(x, width); }
    float triangle(float x) const { return ::triangle(x); }
    float semicircle(float x) const { return ::semicircle(x); }
    float sawTan(float x) const { return ::sawTan(x); }
    float triangleTan(float x) const { return ::triangleTan(x); }
    float strongSine(float x) const { return ::strongSine(x); }
    float fractalSquare(float x) const { return ::fractalSquare(x); }

    float perlin(float x, float min = 0.0f, float max = 1.0f) const { return ::perlin(x, min, max); }
    float simplex(float x, float min = 0.0f, float max = 1.0f) const { return ::simplex(x, min, max); }
    float voronoi(float x, float min = 0.0f, float max = 1.0f) const { return ::voronoi(x, min, max); }
    float turbulence(float x, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f, float min = 0.0f, float max = 1.0f) const {
        return ::turbulence(x, octaves, lacunarity, gain, min, max);
    }
    float ridgedMulti(float x, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f, float min = 0.0f, float max = 1.0f) const {
        return ::ridgedMulti(x, octaves, lacunarity, gain, min, max);
    }

protected:
    WaviateCore(float sampleRateIn, uint64_t samplesSinceAppStartIn, const uint8_t* midiNoteOnIn, const uint8_t* midiCCValueIn,
                const uint64_t* noteOnSamplesIn, const uint64_t* noteOffSamplesIn, const uint8_t* pressOrderIn, int pressCountIn,
                const uint8_t* releaseOrderIn, int releaseCountIn, const uint8_t* voiceOrderIn, int voiceCountIn)
        : coreSampleRate(sampleRateIn), coreSamplesSinceAppStart(samplesSinceAppStartIn), coreMidiNoteOn(midiNoteOnIn), coreMidiCCValue(midiCCValueIn),
          coreSampleWhenMidiNoteOn(noteOnSamplesIn), coreSampleWhenMidiNoteOff(noteOffSamplesIn), coreMidiNotePressOrder(pressOrderIn),
          coreMidiNoteReleaseOrder(releaseOrderIn), coreMidiVoiceOrder(voiceOrderIn), coreMidiNotePressCount(pressCountIn),
          coreMidiNoteReleaseCount(releaseCountIn), coreMidiVoiceCount(voiceCountIn) {}

    static bool isValidIndex(int index, int count) {
        return index >= 0 && index < count;
    }

private:
    static int orderedMidiNote(const uint8_t* order, int count, int index) { return order != nullptr && index >= 0 && index < count ? static_cast<int>(order[index]) : -1; }
    static bool hasOrderedMidiNote(const uint8_t* order, int count, int note) {
        for (int index = 0; order != nullptr && index < count; ++index) if (order[index] == note) return true;
        return false;
    }
    static float envelopeHeldLevel(uint64_t elapsed, uint64_t attack, uint64_t decay, float sustain) {
        if (attack > 0 && elapsed < attack) return static_cast<float>(static_cast<double>(elapsed) / static_cast<double>(attack));
        const uint64_t afterAttack = elapsed > attack ? elapsed - attack : 0ULL;
        if (decay > 0 && afterAttack < decay) return 1.0f + (sustain - 1.0f) * static_cast<float>(static_cast<double>(afterAttack) / static_cast<double>(decay));
        return sustain;
    }
    float midiNotePhaseForFrequency(int note, float frequency) const {
        if (!isValidIndex(note, 128) || frequency <= 0.0f || coreSampleRate <= 0.0f || !hasOrderedMidiNote(coreMidiNotePressOrder, coreMidiNotePressCount, note)) return 0.0f;
        const double cycles = static_cast<double>(samplesSinceMidiNotePressed(note)) * static_cast<double>(frequency) / static_cast<double>(coreSampleRate);
        return static_cast<float>(cycles - __builtin_floor(cycles));
    }
    float coreSampleRate;
    uint64_t coreSamplesSinceAppStart;
    const uint8_t* coreMidiNoteOn;
    const uint8_t* coreMidiCCValue;
    const uint64_t* coreSampleWhenMidiNoteOn;
    const uint64_t* coreSampleWhenMidiNoteOff;
    const uint8_t* coreMidiNotePressOrder;
    const uint8_t* coreMidiNoteReleaseOrder;
    const uint8_t* coreMidiVoiceOrder;
    int coreMidiNotePressCount;
    int coreMidiNoteReleaseCount;
    int coreMidiVoiceCount;
};

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
class WaviateSample final : public WaviateCore {
public:
    WaviateSample(const WaviateSampleInput* inputIn, WaviateSampleStateWriter* writerIn)
        : WaviateCore(inputIn != nullptr ? inputIn->sampleRate : 0.0f,
              inputIn != nullptr ? inputIn->samplesSinceAppStart : 0ULL,
              inputIn != nullptr ? inputIn->midiNoteOn : nullptr, inputIn != nullptr ? inputIn->midiCCValue : nullptr,
              inputIn != nullptr ? inputIn->sampleWhenMidiNoteOn : nullptr, inputIn != nullptr ? inputIn->sampleWhenMidiNoteOff : nullptr,
              inputIn != nullptr ? inputIn->midiNotePressOrder : nullptr, inputIn != nullptr ? inputIn->midiNotePressCount : 0,
              inputIn != nullptr ? inputIn->midiNoteReleaseOrder : nullptr, inputIn != nullptr ? inputIn->midiNoteReleaseCount : 0,
              inputIn != nullptr ? inputIn->midiVoiceOrder : nullptr, inputIn != nullptr ? inputIn->midiVoiceCount : 0),
          input(inputIn), writer(writerIn) {}

    int channel() const { return input != nullptr ? static_cast<int>(input->channel) : 0; }
    int sampleInBlock() const { return input != nullptr ? input->sampleInBlock : 0; }
    int blockSize() const { return input != nullptr ? input->blockSize : 0; }
    int inputChannelCount() const { return input != nullptr ? input->inputChannelCount : 0; }
    int sideChainChannelCount() const { return input != nullptr ? input->sideChainChannelCount : 0; }
    int channelCount() const { return input != nullptr ? input->channelCount : 0; }
    float sampleRate() const { return input != nullptr ? input->sampleRate : 0.0f; }
    uint64_t samplesSinceAppStart() const { return input != nullptr ? input->samplesSinceAppStart : 0ULL; }
    bool isSustainDown() const { return input != nullptr && input->sustain; }

    float incomingSample(int channel = -1, int sample = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : this->channel();
        const int resolvedSample = sample >= 0 ? sample : sampleInBlock();
        if (input == nullptr
            || input->inputDeviceSamples == nullptr
            || !isValidIndex(resolvedChannel, input->inputChannelCount)
            || !isValidIndex(resolvedSample, input->blockSize)
            || input->inputDeviceSamples[resolvedChannel] == nullptr) {
            return 0.0f;
        }

        return input->inputDeviceSamples[resolvedChannel][resolvedSample];
    }

    float sideChainSample(int channel = 0, int sample = -1) const {
        const int resolvedSample = sample >= 0 ? sample : sampleInBlock();
        if (input == nullptr
            || input->inputSideChainSamples == nullptr
            || !isValidIndex(channel, input->sideChainChannelCount)
            || !isValidIndex(resolvedSample, input->blockSize)
            || input->inputSideChainSamples[channel] == nullptr) {
            return 0.0f;
        }

        return input->inputSideChainSamples[channel][resolvedSample];
    }

    float currentSample(int channel = -1, int sample = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : this->channel();
        const int resolvedSample = sample >= 0 ? sample : sampleInBlock();
        if (input == nullptr
            || input->currentSampleData == nullptr
            || !isValidIndex(resolvedChannel, input->channelCount)
            || !isValidIndex(resolvedSample, input->blockSize)
            || input->currentSampleData[resolvedChannel] == nullptr) {
            return 0.0f;
        }

        return input->currentSampleData[resolvedChannel][resolvedSample];
    }

    void setCurrentSample(float value, int channel = -1, int sample = -1) {
        const int resolvedChannel = channel >= 0 ? channel : this->channel();
        const int resolvedSample = sample >= 0 ? sample : sampleInBlock();
        if (input == nullptr
            || input->currentSampleData == nullptr
            || !isValidIndex(resolvedChannel, input->channelCount)
            || !isValidIndex(resolvedSample, input->blockSize)
            || input->currentSampleData[resolvedChannel] == nullptr) {
            return;
        }

        input->currentSampleData[resolvedChannel][resolvedSample] = value;
    }

private:
    const WaviateSampleInput* input;
    WaviateSampleStateWriter* writer;
};

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
class WaviateFrequency final : public WaviateCore {
public:
    WaviateFrequency(const WaviateFrequencyInput* inputIn, WaviateFrequencyStateWriter* writerIn)
        : WaviateCore(inputIn != nullptr ? inputIn->sampleRate : 0.0f,
              inputIn != nullptr ? inputIn->samplesSinceAppStart : 0ULL,
              inputIn != nullptr ? inputIn->midiNoteOn : nullptr, inputIn != nullptr ? inputIn->midiCCValue : nullptr,
              inputIn != nullptr ? inputIn->sampleWhenMidiNoteOn : nullptr, inputIn != nullptr ? inputIn->sampleWhenMidiNoteOff : nullptr,
              inputIn != nullptr ? inputIn->midiNotePressOrder : nullptr, inputIn != nullptr ? inputIn->midiNotePressCount : 0,
              inputIn != nullptr ? inputIn->midiNoteReleaseOrder : nullptr, inputIn != nullptr ? inputIn->midiNoteReleaseCount : 0,
              inputIn != nullptr ? inputIn->midiVoiceOrder : nullptr, inputIn != nullptr ? inputIn->midiVoiceCount : 0),
          input(inputIn), writer(writerIn) {}

    int channel() const { return input != nullptr ? static_cast<int>(input->channel) : 0; }
    int bin() const { return input != nullptr ? input->bin : 0; }
    int totalBinCount() const { return input != nullptr ? input->totalBinCount : 0; }
    int sampleWidth() const { return input != nullptr ? input->sampleWidth : 0; }
    int channelCount() const { return input != nullptr ? input->channelCount : 0; }
    float sampleRate() const { return input != nullptr ? input->sampleRate : 0.0f; }
    uint64_t samplesSinceAppStart() const { return input != nullptr ? input->samplesSinceAppStart : 0ULL; }

    WaviateComplex incomingSample(int channel = -1, int bin = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : this->channel();
        const int resolvedBin = bin >= 0 ? bin : this->bin();
        if (input == nullptr
            || input->inputDeviceData == nullptr
            || !isValidIndex(resolvedChannel, input->channelCount)
            || !isValidIndex(resolvedBin, input->totalBinCount)
            || input->inputDeviceData[resolvedChannel] == nullptr) {
            return { 0.0f, 0.0f };
        }

        return input->inputDeviceData[resolvedChannel][resolvedBin];
    }

    WaviateComplex currentSample(int channel = -1, int bin = -1) const {
        const int resolvedChannel = channel >= 0 ? channel : this->channel();
        const int resolvedBin = bin >= 0 ? bin : this->bin();
        if (input == nullptr
            || input->currentFrequencyData == nullptr
            || !isValidIndex(resolvedChannel, input->channelCount)
            || !isValidIndex(resolvedBin, input->totalBinCount)
            || input->currentFrequencyData[resolvedChannel] == nullptr) {
            return { 0.0f, 0.0f };
        }

        return input->currentFrequencyData[resolvedChannel][resolvedBin];
    }

    WaviateComplex sideChainSample(int channel = 0, int bin = -1) const {
        const int resolvedBin = bin >= 0 ? bin : this->bin();
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

)waviate_cpp_api");
    return api;
}
} // namespace waviate::language
