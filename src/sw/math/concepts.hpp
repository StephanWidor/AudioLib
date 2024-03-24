#pragma once
#include <complex>
#include <concepts>

namespace sw::math {

template<typename C>
concept Complex = std::same_as<std::decay_t<C>, std::complex<typename C::value_type>>;

template<typename T>
concept Number = std::is_floating_point_v<T> || std::is_integral_v<T>;

template<typename T>
struct underlying;

template<Number N>
struct underlying<N>
{
    using Type = N;
};

template<Complex C>
struct underlying<C>
{
    using Type = typename C::value_type;
};

template<typename T>
using underlying_t = typename underlying<T>::Type;

}    // namespace sw::math
