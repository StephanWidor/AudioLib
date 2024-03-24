#pragma once
#include "sw/ranges/defines.hpp"

#include <algorithm>

namespace sw::ranges {

template<typename T>
T accumulate(TypedInputRange<T> auto &&range)
{
    T accumulator = static_cast<T>(0);
    std::ranges::for_each(range, [&accumulator](const auto v) { accumulator += v; });
    return accumulator;
}

}    // namespace sw::ranges
