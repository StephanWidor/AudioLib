#include <gtest/gtest.h>
#include <random>
#include <sw/math/angles.hpp>

namespace sw::math::tests {

template<std::floating_point F>
class AnglesTest : public ::testing::Test
{};
using FloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(AnglesTest, FloatTypes);

TYPED_TEST(AnglesTest, standardized)
{
    using F = TypeParam;

    const auto standardizedFromComplex = [](const F angle, bool positive) {
        auto phaseFromComplex = std::arg(std::polar(math::one<F>, angle));
        if (positive && phaseFromComplex < math::zero<F>)
            phaseFromComplex += math::twoPi<F>;
        return phaseFromComplex;
    };

    const auto checkAngle = [&](const F angle, bool positive) {
        const auto expectedPhase = standardizedFromComplex(angle, positive);
        const auto phase = angles::standardized(angle, positive);
        EXPECT_NEAR(phase, expectedPhase, math::defaultTolerance<F>);
    };

    std::mt19937 randGen(1u);
    std::uniform_real_distribution distribution(static_cast<F>(-100), static_cast<F>(100));

    for (auto i = 0u; i < 100u; ++i)
        checkAngle(distribution(randGen), false);
    for (auto i = 0u; i < 100u; ++i)
        checkAngle(distribution(randGen), true);
}

TYPED_TEST(AnglesTest, equalModTwoPi)
{
    using F = TypeParam;
    constexpr auto tol = defaultTolerance<F>;
    constexpr auto smallValue = static_cast<F>(0.4) * tol;

    const auto check = [](const F angle) {
        EXPECT_TRUE(angles::equalModTwoPi(angle - smallValue, angle + smallValue));
        EXPECT_FALSE(angles::equalModTwoPi(angle - tol, angle + tol));
    };

    check(zero<F>);
    check(twoPi<F>);
    check(two<F> * twoPi<F>);

    std::mt19937 randGen(1u);
    std::uniform_real_distribution distribution(static_cast<F>(-100), static_cast<F>(100));
    for (auto i = 0u; i < 100u; ++i)
        check(distribution(randGen));
}

}    // namespace sw::math::tests
