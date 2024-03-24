#pragma once
#include "sw/math/concepts.hpp"

#include <numbers>

namespace sw::math {

template<std::floating_point F = double>
constexpr F pi = std::numbers::pi_v<F>;

template<std::floating_point F = double>
constexpr F twoPi = static_cast<F>(2.0 * pi<double>);

template<std::floating_point F = double>
constexpr F piHalf = static_cast<F>(0.5 * pi<double>);

template<Number N>
constexpr N zero = static_cast<N>(0);

template<Number N>
constexpr N one = static_cast<N>(1);

template<Number N>
constexpr N two = static_cast<N>(2);

template<std::floating_point F>
constexpr F oneHalf = static_cast<F>(0.5);

namespace detail {

template<typename T>
struct DefaultTolerance;

template<Number N>
struct DefaultTolerance<N>
{
    using Type = underlying_t<N>;
    static constexpr Type value = static_cast<Type>(1e5) * std::numeric_limits<Type>::epsilon();
};

template<>
struct DefaultTolerance<float>
{
    using Type = float;
    static constexpr Type value = 5e-4f;
};

template<>
struct DefaultTolerance<double>
{
    using Type = double;
    static constexpr Type value = 1e-10f;
};

template<Complex C>
struct DefaultTolerance<C>
{
    using Type = underlying_t<C>;
    static constexpr Type value = DefaultTolerance<Type>::value;
};

}    // namespace detail

template<typename T>
constexpr underlying_t<T> defaultTolerance = detail::DefaultTolerance<T>::value;

template<typename T>
constexpr underlying_t<T> defaultToleranceQ = detail::DefaultTolerance<T>::value * detail::DefaultTolerance<T>::value;

}    // namespace sw::math
