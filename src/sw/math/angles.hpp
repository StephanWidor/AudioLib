#pragma once
#include "sw/math/constants.hpp"
#include "sw/math/helpers.hpp"

#include <cmath>

namespace sw::math::angles {

template<std::floating_point F>
constexpr F standardized(const F angle, bool positive = false)
{
    auto standardized = std::fmod(angle, twoPi<F>);
    if (positive)
    {
        if (standardized < zero<F>)
            return std::fmod(standardized + twoPi<F>, twoPi<F>);
    }
    else
    {
        if (standardized > pi<F>)
            return standardized - twoPi<F>;
        else if (standardized < -pi<F>)
            return standardized + twoPi<F>;
    }
    return standardized;
}

template<std::floating_point F>
constexpr bool equalModTwoPi(F angle0, F angle1, F tolerance = defaultTolerance<F>)
{
    angle0 = standardized(angle0, true);
    angle1 = standardized(angle1, true);
    if (std::max(angle0, angle1) >= twoPi<F> - tolerance)
    {
        if (angle0 >= twoPi<F> - tolerance && angle1 <= tolerance)
            angle0 -= twoPi<F>;
        else if (angle1 >= twoPi<F> - tolerance && angle0 <= tolerance)
            angle1 -= twoPi<F>;
    }
    return sw::math::equal(angle0, angle1, tolerance);
}

template<std::floating_point F>
constexpr bool isZeroModTwoPi(const F angle, const F tolerance = defaultTolerance<F>)
{
    return sw::math::equal(standardized(angle, false), math::zero<F>, tolerance);
}

}    // namespace sw::math::angles
