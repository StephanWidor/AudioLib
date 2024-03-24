#pragma once
#include "sw/math/math.hpp"
#include "sw/ranges/defines.hpp"

#include <algorithm>

namespace sw {

template<std::floating_point F>
F phaseAngle(const F frequency, const F timeDiff)
{
    return frequency * timeDiff * math::twoPi<F>;
};

template<std::floating_point F>
F frequency(const F phaseAngle, const F timeDiff)
{
    return phaseAngle / (timeDiff * math::twoPi<F>);
};

}    // namespace sw
