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
        function("getSeconds", "float", {}, SymbolKind::Method, "Seconds since app start."),
        function("samplesToSeconds", "float", { param("uint64_t", "samples") }, SymbolKind::Method, "Convert samples to seconds."),
        function("secondsToSamples", "uint64_t", { param("float", "seconds") }, SymbolKind::Method, "Convert seconds to samples."),
        function("sampleRateHz", "float", {}, SymbolKind::Method, "Sample rate in hertz."),
        function("sampleRateKHz", "float", {}, SymbolKind::Method, "Sample rate in kilohertz."),
        function("adsr", "float", { param("float", "attack"), param("float", "decay"), param("float", "sustain"), param("float", "release"), param("float", "t") }, SymbolKind::Method, "ADSR envelope."),
        function("ADSR", "float", { param("float", "attack"), param("float", "decay"), param("float", "sustain"), param("float", "release"), param("float", "t") }, SymbolKind::Method, "ADSR envelope."),
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
    std::string api;
    api.reserve(32768);
    api.append(R"waviate_cpp_api(
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
    WaviateCore(float sampleRateIn, uint64_t samplesSinceAppStartIn)
        : coreSampleRate(sampleRateIn), coreSamplesSinceAppStart(samplesSinceAppStartIn) {}

    static bool isValidIndex(int index, int count) {
        return index >= 0 && index < count;
    }

private:
    float coreSampleRate;
    uint64_t coreSamplesSinceAppStart;
};

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
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

)waviate_cpp_api");
    api.append(R"waviate_cpp_api(
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

)waviate_cpp_api");
    return api;
}
} // namespace waviate::language
