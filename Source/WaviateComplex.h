#pragma once

#include <cmath>

template <typename T>
struct WaviateBasicComplex
{
    constexpr WaviateBasicComplex() noexcept = default;
    constexpr WaviateBasicComplex(T realIn, T imaginaryIn = {}) noexcept
        : realValue(realIn), imaginaryValue(imaginaryIn) {}

    template <typename U>
    explicit constexpr WaviateBasicComplex(const WaviateBasicComplex<U>& other) noexcept
        : realValue(static_cast<T>(other.real())), imaginaryValue(static_cast<T>(other.imaginary())) {}

    [[nodiscard]] constexpr T real() const noexcept { return realValue; }
    [[nodiscard]] constexpr T imaginary() const noexcept { return imaginaryValue; }

    [[nodiscard]] constexpr WaviateBasicComplex operator+() const noexcept { return *this; }
    [[nodiscard]] constexpr WaviateBasicComplex operator-() const noexcept { return { -realValue, -imaginaryValue }; }
    [[nodiscard]] constexpr WaviateBasicComplex operator+(WaviateBasicComplex rhs) const noexcept { return { realValue + rhs.realValue, imaginaryValue + rhs.imaginaryValue }; }
    [[nodiscard]] constexpr WaviateBasicComplex operator-(WaviateBasicComplex rhs) const noexcept { return { realValue - rhs.realValue, imaginaryValue - rhs.imaginaryValue }; }
    [[nodiscard]] constexpr WaviateBasicComplex operator*(WaviateBasicComplex rhs) const noexcept
    {
        return { realValue * rhs.realValue - imaginaryValue * rhs.imaginaryValue,
                 realValue * rhs.imaginaryValue + imaginaryValue * rhs.realValue };
    }
    [[nodiscard]] constexpr WaviateBasicComplex operator/(WaviateBasicComplex rhs) const noexcept
    {
        const auto denominator = rhs.realValue * rhs.realValue + rhs.imaginaryValue * rhs.imaginaryValue;
        return { (realValue * rhs.realValue + imaginaryValue * rhs.imaginaryValue) / denominator,
                 (imaginaryValue * rhs.realValue - realValue * rhs.imaginaryValue) / denominator };
    }
    [[nodiscard]] constexpr WaviateBasicComplex operator*(T scalar) const noexcept { return { realValue * scalar, imaginaryValue * scalar }; }
    [[nodiscard]] constexpr WaviateBasicComplex operator/(T scalar) const noexcept { return { realValue / scalar, imaginaryValue / scalar }; }

    constexpr WaviateBasicComplex& operator+=(WaviateBasicComplex rhs) noexcept { return *this = *this + rhs; }
    constexpr WaviateBasicComplex& operator-=(WaviateBasicComplex rhs) noexcept { return *this = *this - rhs; }
    constexpr WaviateBasicComplex& operator*=(WaviateBasicComplex rhs) noexcept { return *this = *this * rhs; }
    constexpr WaviateBasicComplex& operator/=(WaviateBasicComplex rhs) noexcept { return *this = *this / rhs; }
    constexpr WaviateBasicComplex& operator*=(T scalar) noexcept { realValue *= scalar; imaginaryValue *= scalar; return *this; }
    constexpr WaviateBasicComplex& operator/=(T scalar) noexcept { realValue /= scalar; imaginaryValue /= scalar; return *this; }

    [[nodiscard]] constexpr T norm() const noexcept { return realValue * realValue + imaginaryValue * imaginaryValue; }
    [[nodiscard]] T magnitude() const noexcept { return std::sqrt(norm()); }
    [[nodiscard]] T phase() const noexcept { return std::atan2(imaginaryValue, realValue); }
    [[nodiscard]] constexpr WaviateBasicComplex conjugate() const noexcept { return { realValue, -imaginaryValue }; }

private:
    T realValue {};
    T imaginaryValue {};
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
        const auto scale = std::exp(value.real());
        return { scale * std::cos(value.imaginary()), scale * std::sin(value.imaginary()) };
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
        return { std::sin(value.real()) * std::cosh(value.imaginary()), std::cos(value.real()) * std::sinh(value.imaginary()) };
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> cos(WaviateBasicComplex<T> value) noexcept
    {
        return { std::cos(value.real()) * std::cosh(value.imaginary()), -std::sin(value.real()) * std::sinh(value.imaginary()) };
    }
    template <typename T> [[nodiscard]] inline WaviateBasicComplex<T> tan(WaviateBasicComplex<T> value) noexcept { return sin(value) / cos(value); }
}
