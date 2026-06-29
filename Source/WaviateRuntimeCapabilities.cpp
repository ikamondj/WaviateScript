#include "WaviateRuntimeCapabilities.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <mutex>

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/DynamicLibrary.h>

namespace
{
constexpr std::string_view waviateRuntimeFunctions[] = {
    "waviate_consume_fuel",
    "waviate_fuel_trap",
    "__waviate_internal_arena_allocate",
    "__waviate_internal_arena_generation",
    "waviate_load_audio_from_location"
};

constexpr std::string_view pureMathFunctions[] = {
    "acos", "acosf", "asin", "asinf", "atan", "atanf", "atan2", "atan2f",
    "cos", "cosf", "sin", "sinf", "tan", "tanf",
    "acosh", "acoshf", "asinh", "asinhf", "atanh", "atanhf",
    "cosh", "coshf", "sinh", "sinhf", "tanh", "tanhf",
    "exp", "expf", "exp2", "exp2f", "expm1", "expm1f",
    "frexp", "frexpf", "ilogb", "ilogbf", "ldexp", "ldexpf",
    "log", "logf", "log10", "log10f", "log1p", "log1pf", "log2", "log2f",
    "logb", "logbf", "modf", "modff", "scalbn", "scalbnf", "scalbln", "scalblnf",
    "cbrt", "cbrtf", "fabs", "fabsf", "hypot", "hypotf", "pow", "powf",
    "sqrt", "sqrtf", "erf", "erff", "erfc", "erfcf", "lgamma", "lgammaf",
    "tgamma", "tgammaf", "ceil", "ceilf", "floor", "floorf",
    "nearbyint", "nearbyintf", "rint", "rintf", "lrint", "lrintf",
    "llrint", "llrintf", "round", "roundf", "lround", "lroundf",
    "llround", "llroundf", "trunc", "truncf", "fmod", "fmodf",
    "remainder", "remainderf", "remquo", "remquof", "copysign", "copysignf",
    "nan", "nanf", "nextafter", "nextafterf", "nexttoward", "nexttowardf",
    "fmax", "fmaxf", "fmin", "fminf", "fdim", "fdimf", "fma", "fmaf",
    "fpclassify", "isfinite", "isinf", "isnan", "isnormal", "signbit"
};

constexpr std::string_view basicMemoryFunctions[] = {
    "memcpy", "memmove", "memset", "memcmp", "strlen"
};

template <size_t size>
bool containsName (const std::string_view name, const std::string_view (&names)[size]) noexcept
{
    return std::find (std::begin (names), std::end (names), name) != std::end (names);
}

std::string_view withoutLlvmPrefix (std::string_view name) noexcept
{
    if (! name.empty() && name.front() == '\1')
        name.remove_prefix (1);

    return name;
}

std::string_view withoutStdCallSuffix (std::string_view name) noexcept
{
    const auto at = name.rfind ('@');
    if (at == std::string_view::npos || at == 0 || at + 1 >= name.size())
        return name;

    const auto suffix = name.substr (at + 1);
    const auto allDigits = std::all_of (suffix.begin(), suffix.end(), [] (char c) {
        return c >= '0' && c <= '9';
    });

    return allDigits ? name.substr (0, at) : name;
}

bool isAuditedStdFunction (const std::string_view name) noexcept
{
    return containsName (name, pureMathFunctions) || containsName (name, basicMemoryFunctions);
}

bool isAuditedDecoratedStdFunction (std::string_view name) noexcept
{
    name = withoutLlvmPrefix (name);
    name = withoutStdCallSuffix (name);

    if (isAuditedStdFunction (name))
        return true;

    if (name.size() > 1 && name.front() == '_' && name[1] != '_')
    {
        name.remove_prefix (1);
        return isAuditedStdFunction (name);
    }

    return false;
}

double runtimeAcos (double x) noexcept { return std::acos (x); }
float runtimeAcosf (float x) noexcept { return std::acos (x); }
double runtimeAsin (double x) noexcept { return std::asin (x); }
float runtimeAsinf (float x) noexcept { return std::asin (x); }
double runtimeAtan (double x) noexcept { return std::atan (x); }
float runtimeAtanf (float x) noexcept { return std::atan (x); }
double runtimeAtan2 (double y, double x) noexcept { return std::atan2 (y, x); }
float runtimeAtan2f (float y, float x) noexcept { return std::atan2 (y, x); }
double runtimeCos (double x) noexcept { return std::cos (x); }
float runtimeCosf (float x) noexcept { return std::cos (x); }
double runtimeSin (double x) noexcept { return std::sin (x); }
float runtimeSinf (float x) noexcept { return std::sin (x); }
double runtimeTan (double x) noexcept { return std::tan (x); }
float runtimeTanf (float x) noexcept { return std::tan (x); }
double runtimeAcosh (double x) noexcept { return std::acosh (x); }
float runtimeAcoshf (float x) noexcept { return std::acosh (x); }
double runtimeAsinh (double x) noexcept { return std::asinh (x); }
float runtimeAsinhf (float x) noexcept { return std::asinh (x); }
double runtimeAtanh (double x) noexcept { return std::atanh (x); }
float runtimeAtanhf (float x) noexcept { return std::atanh (x); }
double runtimeCosh (double x) noexcept { return std::cosh (x); }
float runtimeCoshf (float x) noexcept { return std::cosh (x); }
double runtimeSinh (double x) noexcept { return std::sinh (x); }
float runtimeSinhf (float x) noexcept { return std::sinh (x); }
double runtimeTanh (double x) noexcept { return std::tanh (x); }
float runtimeTanhf (float x) noexcept { return std::tanh (x); }
double runtimeExp (double x) noexcept { return std::exp (x); }
float runtimeExpf (float x) noexcept { return std::exp (x); }
double runtimeExp2 (double x) noexcept { return std::exp2 (x); }
float runtimeExp2f (float x) noexcept { return std::exp2 (x); }
double runtimeExpm1 (double x) noexcept { return std::expm1 (x); }
float runtimeExpm1f (float x) noexcept { return std::expm1 (x); }
double runtimeFrexp (double x, int* exponent) noexcept { return std::frexp (x, exponent); }
float runtimeFrexpf (float x, int* exponent) noexcept { return std::frexp (x, exponent); }
int runtimeIlogb (double x) noexcept { return std::ilogb (x); }
int runtimeIlogbf (float x) noexcept { return std::ilogb (x); }
double runtimeLdexp (double x, int exponent) noexcept { return std::ldexp (x, exponent); }
float runtimeLdexpf (float x, int exponent) noexcept { return std::ldexp (x, exponent); }
double runtimeLog (double x) noexcept { return std::log (x); }
float runtimeLogf (float x) noexcept { return std::log (x); }
double runtimeLog10 (double x) noexcept { return std::log10 (x); }
float runtimeLog10f (float x) noexcept { return std::log10 (x); }
double runtimeLog1p (double x) noexcept { return std::log1p (x); }
float runtimeLog1pf (float x) noexcept { return std::log1p (x); }
double runtimeLog2 (double x) noexcept { return std::log2 (x); }
float runtimeLog2f (float x) noexcept { return std::log2 (x); }
double runtimeLogb (double x) noexcept { return std::logb (x); }
float runtimeLogbf (float x) noexcept { return std::logb (x); }
double runtimeModf (double x, double* integerPart) noexcept { return std::modf (x, integerPart); }
float runtimeModff (float x, float* integerPart) noexcept { return std::modf (x, integerPart); }
double runtimeScalbn (double x, int exponent) noexcept { return std::scalbn (x, exponent); }
float runtimeScalbnf (float x, int exponent) noexcept { return std::scalbn (x, exponent); }
double runtimeScalbln (double x, long exponent) noexcept { return std::scalbln (x, exponent); }
float runtimeScalblnf (float x, long exponent) noexcept { return std::scalbln (x, exponent); }
double runtimeCbrt (double x) noexcept { return std::cbrt (x); }
float runtimeCbrtf (float x) noexcept { return std::cbrt (x); }
double runtimeFabs (double x) noexcept { return std::fabs (x); }
float runtimeFabsf (float x) noexcept { return std::fabs (x); }
double runtimeHypot (double x, double y) noexcept { return std::hypot (x, y); }
float runtimeHypotf (float x, float y) noexcept { return std::hypot (x, y); }
double runtimePow (double base, double exponent) noexcept { return std::pow (base, exponent); }
float runtimePowf (float base, float exponent) noexcept { return std::pow (base, exponent); }
double runtimeSqrt (double x) noexcept { return std::sqrt (x); }
float runtimeSqrtf (float x) noexcept { return std::sqrt (x); }
double runtimeErf (double x) noexcept { return std::erf (x); }
float runtimeErff (float x) noexcept { return std::erf (x); }
double runtimeErfc (double x) noexcept { return std::erfc (x); }
float runtimeErfcf (float x) noexcept { return std::erfc (x); }
double runtimeLgamma (double x) noexcept { return std::lgamma (x); }
float runtimeLgammaf (float x) noexcept { return std::lgamma (x); }
double runtimeTgamma (double x) noexcept { return std::tgamma (x); }
float runtimeTgammaf (float x) noexcept { return std::tgamma (x); }
double runtimeCeil (double x) noexcept { return std::ceil (x); }
float runtimeCeilf (float x) noexcept { return std::ceil (x); }
double runtimeFloor (double x) noexcept { return std::floor (x); }
float runtimeFloorf (float x) noexcept { return std::floor (x); }
double runtimeNearbyint (double x) noexcept { return std::nearbyint (x); }
float runtimeNearbyintf (float x) noexcept { return std::nearbyint (x); }
double runtimeRint (double x) noexcept { return std::rint (x); }
float runtimeRintf (float x) noexcept { return std::rint (x); }
long runtimeLrint (double x) noexcept { return std::lrint (x); }
long runtimeLrintf (float x) noexcept { return std::lrint (x); }
long long runtimeLlrint (double x) noexcept { return std::llrint (x); }
long long runtimeLlrintf (float x) noexcept { return std::llrint (x); }
double runtimeRound (double x) noexcept { return std::round (x); }
float runtimeRoundf (float x) noexcept { return std::round (x); }
long runtimeLround (double x) noexcept { return std::lround (x); }
long runtimeLroundf (float x) noexcept { return std::lround (x); }
long long runtimeLlround (double x) noexcept { return std::llround (x); }
long long runtimeLlroundf (float x) noexcept { return std::llround (x); }
double runtimeTrunc (double x) noexcept { return std::trunc (x); }
float runtimeTruncf (float x) noexcept { return std::trunc (x); }
double runtimeFmod (double x, double y) noexcept { return std::fmod (x, y); }
float runtimeFmodf (float x, float y) noexcept { return std::fmod (x, y); }
double runtimeRemainder (double x, double y) noexcept { return std::remainder (x, y); }
float runtimeRemainderf (float x, float y) noexcept { return std::remainder (x, y); }
double runtimeRemquo (double x, double y, int* quotient) noexcept { return std::remquo (x, y, quotient); }
float runtimeRemquof (float x, float y, int* quotient) noexcept { return std::remquo (x, y, quotient); }
double runtimeCopysign (double magnitude, double sign) noexcept { return std::copysign (magnitude, sign); }
float runtimeCopysignf (float magnitude, float sign) noexcept { return std::copysign (magnitude, sign); }
double runtimeNan (const char* tagp) noexcept { return std::nan (tagp); }
float runtimeNanf (const char* tagp) noexcept { return std::nanf (tagp); }
double runtimeNextafter (double from, double to) noexcept { return std::nextafter (from, to); }
float runtimeNextafterf (float from, float to) noexcept { return std::nextafter (from, to); }
double runtimeNexttoward (double from, long double to) noexcept { return std::nexttoward (from, to); }
float runtimeNexttowardf (float from, long double to) noexcept { return std::nexttoward (from, to); }
double runtimeFmax (double x, double y) noexcept { return std::fmax (x, y); }
float runtimeFmaxf (float x, float y) noexcept { return std::fmax (x, y); }
double runtimeFmin (double x, double y) noexcept { return std::fmin (x, y); }
float runtimeFminf (float x, float y) noexcept { return std::fmin (x, y); }
double runtimeFdim (double x, double y) noexcept { return std::fdim (x, y); }
float runtimeFdimf (float x, float y) noexcept { return std::fdim (x, y); }
double runtimeFma (double x, double y, double z) noexcept { return std::fma (x, y, z); }
float runtimeFmaf (float x, float y, float z) noexcept { return std::fma (x, y, z); }
int runtimeFpclassify (double x) noexcept { return std::fpclassify (x); }
bool runtimeIsfinite (double x) noexcept { return std::isfinite (x); }
bool runtimeIsinf (double x) noexcept { return std::isinf (x); }
bool runtimeIsnan (double x) noexcept { return std::isnan (x); }
bool runtimeIsnormal (double x) noexcept { return std::isnormal (x); }
bool runtimeSignbit (double x) noexcept { return std::signbit (x); }

void* runtimeMemcpy (void* destination, const void* source, std::size_t count) noexcept
{
    return std::memcpy (destination, source, count);
}

void* runtimeMemmove (void* destination, const void* source, std::size_t count) noexcept
{
    return std::memmove (destination, source, count);
}

void* runtimeMemset (void* destination, int value, std::size_t count) noexcept
{
    return std::memset (destination, value, count);
}

int runtimeMemcmp (const void* lhs, const void* rhs, std::size_t count) noexcept
{
    return std::memcmp (lhs, rhs, count);
}

std::size_t runtimeStrlen (const char* text) noexcept
{
    return std::strlen (text);
}

template <typename Function>
void addSymbol (const char* name, Function function)
{
    llvm::sys::DynamicLibrary::AddSymbol (name, reinterpret_cast<void*> (function));
}

template <typename Function>
void addCAndUnderscoreSymbol (const char* name, Function function)
{
    addSymbol (name, function);

    char decorated[64] {};
    decorated[0] = '_';
    std::strncpy (decorated + 1, name, sizeof (decorated) - 2);
    addSymbol (decorated, function);
}
} // namespace

namespace waviate::runtime
{
bool isAuditedExternalFunction (std::string_view name) noexcept
{
    name = withoutLlvmPrefix (name);

    if (containsName (name, waviateRuntimeFunctions))
        return true;

    return isAuditedDecoratedStdFunction (name);
}

void registerAuditedRuntimeSymbols()
{
    static std::once_flag once;
    std::call_once (once, [] {
        addCAndUnderscoreSymbol ("acos", &runtimeAcos);
        addCAndUnderscoreSymbol ("acosf", &runtimeAcosf);
        addCAndUnderscoreSymbol ("asin", &runtimeAsin);
        addCAndUnderscoreSymbol ("asinf", &runtimeAsinf);
        addCAndUnderscoreSymbol ("atan", &runtimeAtan);
        addCAndUnderscoreSymbol ("atanf", &runtimeAtanf);
        addCAndUnderscoreSymbol ("atan2", &runtimeAtan2);
        addCAndUnderscoreSymbol ("atan2f", &runtimeAtan2f);
        addCAndUnderscoreSymbol ("cos", &runtimeCos);
        addCAndUnderscoreSymbol ("cosf", &runtimeCosf);
        addCAndUnderscoreSymbol ("sin", &runtimeSin);
        addCAndUnderscoreSymbol ("sinf", &runtimeSinf);
        addCAndUnderscoreSymbol ("tan", &runtimeTan);
        addCAndUnderscoreSymbol ("tanf", &runtimeTanf);
        addCAndUnderscoreSymbol ("acosh", &runtimeAcosh);
        addCAndUnderscoreSymbol ("acoshf", &runtimeAcoshf);
        addCAndUnderscoreSymbol ("asinh", &runtimeAsinh);
        addCAndUnderscoreSymbol ("asinhf", &runtimeAsinhf);
        addCAndUnderscoreSymbol ("atanh", &runtimeAtanh);
        addCAndUnderscoreSymbol ("atanhf", &runtimeAtanhf);
        addCAndUnderscoreSymbol ("cosh", &runtimeCosh);
        addCAndUnderscoreSymbol ("coshf", &runtimeCoshf);
        addCAndUnderscoreSymbol ("sinh", &runtimeSinh);
        addCAndUnderscoreSymbol ("sinhf", &runtimeSinhf);
        addCAndUnderscoreSymbol ("tanh", &runtimeTanh);
        addCAndUnderscoreSymbol ("tanhf", &runtimeTanhf);
        addCAndUnderscoreSymbol ("exp", &runtimeExp);
        addCAndUnderscoreSymbol ("expf", &runtimeExpf);
        addCAndUnderscoreSymbol ("exp2", &runtimeExp2);
        addCAndUnderscoreSymbol ("exp2f", &runtimeExp2f);
        addCAndUnderscoreSymbol ("expm1", &runtimeExpm1);
        addCAndUnderscoreSymbol ("expm1f", &runtimeExpm1f);
        addCAndUnderscoreSymbol ("frexp", &runtimeFrexp);
        addCAndUnderscoreSymbol ("frexpf", &runtimeFrexpf);
        addCAndUnderscoreSymbol ("ilogb", &runtimeIlogb);
        addCAndUnderscoreSymbol ("ilogbf", &runtimeIlogbf);
        addCAndUnderscoreSymbol ("ldexp", &runtimeLdexp);
        addCAndUnderscoreSymbol ("ldexpf", &runtimeLdexpf);
        addCAndUnderscoreSymbol ("log", &runtimeLog);
        addCAndUnderscoreSymbol ("logf", &runtimeLogf);
        addCAndUnderscoreSymbol ("log10", &runtimeLog10);
        addCAndUnderscoreSymbol ("log10f", &runtimeLog10f);
        addCAndUnderscoreSymbol ("log1p", &runtimeLog1p);
        addCAndUnderscoreSymbol ("log1pf", &runtimeLog1pf);
        addCAndUnderscoreSymbol ("log2", &runtimeLog2);
        addCAndUnderscoreSymbol ("log2f", &runtimeLog2f);
        addCAndUnderscoreSymbol ("logb", &runtimeLogb);
        addCAndUnderscoreSymbol ("logbf", &runtimeLogbf);
        addCAndUnderscoreSymbol ("modf", &runtimeModf);
        addCAndUnderscoreSymbol ("modff", &runtimeModff);
        addCAndUnderscoreSymbol ("scalbn", &runtimeScalbn);
        addCAndUnderscoreSymbol ("scalbnf", &runtimeScalbnf);
        addCAndUnderscoreSymbol ("scalbln", &runtimeScalbln);
        addCAndUnderscoreSymbol ("scalblnf", &runtimeScalblnf);
        addCAndUnderscoreSymbol ("cbrt", &runtimeCbrt);
        addCAndUnderscoreSymbol ("cbrtf", &runtimeCbrtf);
        addCAndUnderscoreSymbol ("fabs", &runtimeFabs);
        addCAndUnderscoreSymbol ("fabsf", &runtimeFabsf);
        addCAndUnderscoreSymbol ("hypot", &runtimeHypot);
        addCAndUnderscoreSymbol ("hypotf", &runtimeHypotf);
        addCAndUnderscoreSymbol ("pow", &runtimePow);
        addCAndUnderscoreSymbol ("powf", &runtimePowf);
        addCAndUnderscoreSymbol ("sqrt", &runtimeSqrt);
        addCAndUnderscoreSymbol ("sqrtf", &runtimeSqrtf);
        addCAndUnderscoreSymbol ("erf", &runtimeErf);
        addCAndUnderscoreSymbol ("erff", &runtimeErff);
        addCAndUnderscoreSymbol ("erfc", &runtimeErfc);
        addCAndUnderscoreSymbol ("erfcf", &runtimeErfcf);
        addCAndUnderscoreSymbol ("lgamma", &runtimeLgamma);
        addCAndUnderscoreSymbol ("lgammaf", &runtimeLgammaf);
        addCAndUnderscoreSymbol ("tgamma", &runtimeTgamma);
        addCAndUnderscoreSymbol ("tgammaf", &runtimeTgammaf);
        addCAndUnderscoreSymbol ("ceil", &runtimeCeil);
        addCAndUnderscoreSymbol ("ceilf", &runtimeCeilf);
        addCAndUnderscoreSymbol ("floor", &runtimeFloor);
        addCAndUnderscoreSymbol ("floorf", &runtimeFloorf);
        addCAndUnderscoreSymbol ("nearbyint", &runtimeNearbyint);
        addCAndUnderscoreSymbol ("nearbyintf", &runtimeNearbyintf);
        addCAndUnderscoreSymbol ("rint", &runtimeRint);
        addCAndUnderscoreSymbol ("rintf", &runtimeRintf);
        addCAndUnderscoreSymbol ("lrint", &runtimeLrint);
        addCAndUnderscoreSymbol ("lrintf", &runtimeLrintf);
        addCAndUnderscoreSymbol ("llrint", &runtimeLlrint);
        addCAndUnderscoreSymbol ("llrintf", &runtimeLlrintf);
        addCAndUnderscoreSymbol ("round", &runtimeRound);
        addCAndUnderscoreSymbol ("roundf", &runtimeRoundf);
        addCAndUnderscoreSymbol ("lround", &runtimeLround);
        addCAndUnderscoreSymbol ("lroundf", &runtimeLroundf);
        addCAndUnderscoreSymbol ("llround", &runtimeLlround);
        addCAndUnderscoreSymbol ("llroundf", &runtimeLlroundf);
        addCAndUnderscoreSymbol ("trunc", &runtimeTrunc);
        addCAndUnderscoreSymbol ("truncf", &runtimeTruncf);
        addCAndUnderscoreSymbol ("fmod", &runtimeFmod);
        addCAndUnderscoreSymbol ("fmodf", &runtimeFmodf);
        addCAndUnderscoreSymbol ("remainder", &runtimeRemainder);
        addCAndUnderscoreSymbol ("remainderf", &runtimeRemainderf);
        addCAndUnderscoreSymbol ("remquo", &runtimeRemquo);
        addCAndUnderscoreSymbol ("remquof", &runtimeRemquof);
        addCAndUnderscoreSymbol ("copysign", &runtimeCopysign);
        addCAndUnderscoreSymbol ("copysignf", &runtimeCopysignf);
        addCAndUnderscoreSymbol ("nan", &runtimeNan);
        addCAndUnderscoreSymbol ("nanf", &runtimeNanf);
        addCAndUnderscoreSymbol ("nextafter", &runtimeNextafter);
        addCAndUnderscoreSymbol ("nextafterf", &runtimeNextafterf);
        addCAndUnderscoreSymbol ("nexttoward", &runtimeNexttoward);
        addCAndUnderscoreSymbol ("nexttowardf", &runtimeNexttowardf);
        addCAndUnderscoreSymbol ("fmax", &runtimeFmax);
        addCAndUnderscoreSymbol ("fmaxf", &runtimeFmaxf);
        addCAndUnderscoreSymbol ("fmin", &runtimeFmin);
        addCAndUnderscoreSymbol ("fminf", &runtimeFminf);
        addCAndUnderscoreSymbol ("fdim", &runtimeFdim);
        addCAndUnderscoreSymbol ("fdimf", &runtimeFdimf);
        addCAndUnderscoreSymbol ("fma", &runtimeFma);
        addCAndUnderscoreSymbol ("fmaf", &runtimeFmaf);
        addCAndUnderscoreSymbol ("fpclassify", &runtimeFpclassify);
        addCAndUnderscoreSymbol ("isfinite", &runtimeIsfinite);
        addCAndUnderscoreSymbol ("isinf", &runtimeIsinf);
        addCAndUnderscoreSymbol ("isnan", &runtimeIsnan);
        addCAndUnderscoreSymbol ("isnormal", &runtimeIsnormal);
        addCAndUnderscoreSymbol ("signbit", &runtimeSignbit);
        addCAndUnderscoreSymbol ("memcpy", &runtimeMemcpy);
        addCAndUnderscoreSymbol ("memmove", &runtimeMemmove);
        addCAndUnderscoreSymbol ("memset", &runtimeMemset);
        addCAndUnderscoreSymbol ("memcmp", &runtimeMemcmp);
        addCAndUnderscoreSymbol ("strlen", &runtimeStrlen);
    });
}
} // namespace waviate::runtime
