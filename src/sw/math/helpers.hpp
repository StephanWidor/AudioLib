#pragma once
#include "sw/math/concepts.hpp"
#include "sw/math/constants.hpp"

namespace sw::math {

template<Complex C>
constexpr bool isZero(const C &c, const underlying_t<C> tolerance = defaultTolerance<C>)
{
    return std::norm(c) <= tolerance * tolerance;
}

template<typename T>
constexpr bool isZero(const T &t, const underlying_t<T> tolerance = defaultTolerance<T>)
{
    return std::abs(t) <= tolerance;
}

template<typename T>
constexpr bool equal(const T &f0, const T &f1, const underlying_t<T> tolerance = defaultTolerance<T>)
{
    return isZero<T>(f0 - f1, tolerance);
}

constexpr bool isPowerOfTwo(const std::integral auto x)
{
    return (x != 0 && (x & (x - 1)) == 0);
}

template<std::floating_point F>
constexpr F maxRatio(const F f0, const F f1)
{
    return f0 > f1 ? f0 / f1 : f1 / f0;
}

}    // namespace sw::math
