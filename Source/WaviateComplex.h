#pragma once

#include <cmath>

template <typename T>
struct WaviateBasicComplex
{
    T real {};
    T imag {};

    constexpr WaviateBasicComplex() noexcept = default;
    constexpr WaviateBasicComplex(T realIn, T imagIn = {}) noexcept : real(realIn), imag(imagIn) {}

    template <typename U>
    explicit constexpr WaviateBasicComplex(const WaviateBasicComplex<U>& other) noexcept
        : real(static_cast<T>(other.real)), imag(static_cast<T>(other.imag)) {}

    [[nodiscard]] constexpr WaviateBasicComplex operator+() const noexcept { return *this; }
    [[nodiscard]] constexpr WaviateBasicComplex operator-() const noexcept { return { -real, -imag }; }
    [[nodiscard]] constexpr WaviateBasicComplex operator+(WaviateBasicComplex rhs) const noexcept { return { real + rhs.real, imag + rhs.imag }; }
    [[nodiscard]] constexpr WaviateBasicComplex operator-(WaviateBasicComplex rhs) const noexcept { return { real - rhs.real, imag - rhs.imag }; }
    [[nodiscard]] constexpr WaviateBasicComplex operator*(WaviateBasicComplex rhs) const noexcept
    {
        return { real * rhs.real - imag * rhs.imag, real * rhs.imag + imag * rhs.real };
    }
    [[nodiscard]] constexpr WaviateBasicComplex operator/(WaviateBasicComplex rhs) const noexcept
    {
        const auto denominator = rhs.real * rhs.real + rhs.imag * rhs.imag;
        return { (real * rhs.real + imag * rhs.imag) / denominator,
                 (imag * rhs.real - real * rhs.imag) / denominator };
    }
    [[nodiscard]] constexpr WaviateBasicComplex operator*(T scalar) const noexcept { return { real * scalar, imag * scalar }; }
    [[nodiscard]] constexpr WaviateBasicComplex operator/(T scalar) const noexcept { return { real / scalar, imag / scalar }; }

    constexpr WaviateBasicComplex& operator+=(WaviateBasicComplex rhs) noexcept { return *this = *this + rhs; }
    constexpr WaviateBasicComplex& operator-=(WaviateBasicComplex rhs) noexcept { return *this = *this - rhs; }
    constexpr WaviateBasicComplex& operator*=(WaviateBasicComplex rhs) noexcept { return *this = *this * rhs; }
    constexpr WaviateBasicComplex& operator/=(WaviateBasicComplex rhs) noexcept { return *this = *this / rhs; }
    constexpr WaviateBasicComplex& operator*=(T scalar) noexcept { real *= scalar; imag *= scalar; return *this; }
    constexpr WaviateBasicComplex& operator/=(T scalar) noexcept { real /= scalar; imag /= scalar; return *this; }

    [[nodiscard]] constexpr T norm() const noexcept { return real * real + imag * imag; }
    [[nodiscard]] T magnitude() const noexcept { return std::sqrt(norm()); }
    [[nodiscard]] T phase() const noexcept { return std::atan2(imag, real); }
    [[nodiscard]] constexpr WaviateBasicComplex conjugate() const noexcept { return { real, -imag }; }
};

template <typename T>
[[nodiscard]] constexpr WaviateBasicComplex<T> operator*(T scalar, WaviateBasicComplex<T> value) noexcept { return value * scalar; }

using fcomplex = WaviateBasicComplex<float>;
using dcomplex = WaviateBasicComplex<double>;
using WaviateComplex = fcomplex;

namespace waviate::complex
{
    template <typename T> [[nodiscard]] inline T abs(WaviateBasicComplex<T> value) noexcept { return value.magnitude(); }
    template <typename T> [[nodiscard]] inline T arg(WaviateBasicComplex<T> value) noexcept { return value.phase(); }
    template <typename T> [[nodiscard]] constexpr WaviateBasicComplex<T> conj(WaviateBasicComplex<T> value) noexcept { return value.conjugate(); }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> polar(T magnitude, T phase) noexcept
    {
        return { magnitude * std::cos(phase), magnitude * std::sin(phase) };
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> exp(WaviateBasicComplex<T> value) noexcept
    {
        const auto scale = std::exp(value.real);
        return { scale * std::cos(value.imag), scale * std::sin(value.imag) };
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> log(WaviateBasicComplex<T> value) noexcept
    {
        return { std::log(value.magnitude()), value.phase() };
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> pow(WaviateBasicComplex<T> base, WaviateBasicComplex<T> exponent) noexcept
    {
        return exp(exponent * log(base));
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> pow(WaviateBasicComplex<T> base, T exponent) noexcept
    {
        return polar(std::pow(base.magnitude(), exponent), base.phase() * exponent);
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> sin(WaviateBasicComplex<T> value) noexcept
    {
        return { std::sin(value.real) * std::cosh(value.imag), std::cos(value.real) * std::sinh(value.imag) };
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> cos(WaviateBasicComplex<T> value) noexcept
    {
        return { std::cos(value.real) * std::cosh(value.imag), -std::sin(value.real) * std::sinh(value.imag) };
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> tan(WaviateBasicComplex<T> value) noexcept { return sin(value) / cos(value); }
}
