#include <gtest/gtest.h>

#include <limits>

import worse.core.math;

namespace
{
    namespace math = worse::core::math;
} // namespace

TEST(MathTest, Min)
{
    EXPECT_FLOAT_EQ(math::min(2.0f, 5.0f), 2.0f);
    EXPECT_FLOAT_EQ(math::min(5.0f, 2.0f), 2.0f);
}

TEST(MathTest, Max)
{
    EXPECT_FLOAT_EQ(math::max(2.0f, 5.0f), 5.0f);
    EXPECT_FLOAT_EQ(math::max(5.0f, 2.0f), 5.0f);
}

TEST(MathTest, Abs)
{
    EXPECT_FLOAT_EQ(math::abs(-3.0f), 3.0f);
    EXPECT_FLOAT_EQ(math::abs(3.0f), 3.0f);
}

TEST(MathTest, Clamp)
{
    EXPECT_FLOAT_EQ(math::clamp(-1.0f, 0.0f, 1.0f), 0.0f);
    EXPECT_FLOAT_EQ(math::clamp(0.5f, 0.0f, 1.0f), 0.5f);
    EXPECT_FLOAT_EQ(math::clamp(2.0f, 0.0f, 1.0f), 1.0f);
}

TEST(MathTest, Saturate)
{
    EXPECT_FLOAT_EQ(math::saturate(-1.0f), 0.0f);
    EXPECT_FLOAT_EQ(math::saturate(0.3f), 0.3f);
    EXPECT_FLOAT_EQ(math::saturate(2.0f), 1.0f);
}

TEST(MathTest, Lerp)
{
    EXPECT_FLOAT_EQ(math::lerp(0.0f, 10.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(math::lerp(0.0f, 10.0f, 1.0f), 10.0f);
    EXPECT_FLOAT_EQ(math::lerp(0.0f, 10.0f, 0.5f), 5.0f);
}

TEST(MathTest, AngleConstants)
{
    EXPECT_FLOAT_EQ(math::PI, 3.14159265f);
    EXPECT_FLOAT_EQ(math::TWO_PI, 2.0f * math::PI);
    EXPECT_FLOAT_EQ(math::HALF_PI, 0.5f * math::PI);
    EXPECT_FLOAT_EQ(math::INV_PI, 1.0f / math::PI);
}

TEST(MathTest, AngleConversion)
{
    EXPECT_FLOAT_EQ(math::radians(180.0f), math::PI);
    EXPECT_FLOAT_EQ(math::degrees(math::PI), 180.0f);
}

TEST(MathTest, Trig)
{
    EXPECT_FLOAT_EQ(math::sin(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(math::cos(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(math::sin(math::HALF_PI), 1.0f);
    EXPECT_FLOAT_EQ(math::atan2(1.0f, 1.0f), math::PI / 4.0f);
}

TEST(MathTest, Exponential)
{
    EXPECT_FLOAT_EQ(math::pow(2.0f, 10.0f), 1024.0f);
    EXPECT_FLOAT_EQ(math::exp(0.0f), 1.0f);
    EXPECT_FLOAT_EQ(math::log2(8.0f), 3.0f);
}

TEST(MathTest, Rounding)
{
    EXPECT_FLOAT_EQ(math::floor(2.7f), 2.0f);
    EXPECT_FLOAT_EQ(math::ceil(2.1f), 3.0f);
    EXPECT_FLOAT_EQ(math::round(2.5f), 3.0f);
    EXPECT_FLOAT_EQ(math::trunc(-2.7f), -2.0f);
    EXPECT_FLOAT_EQ(math::frac(2.25f), 0.25f);
    EXPECT_FLOAT_EQ(math::mod(5.0f, 3.0f), 2.0f);
}

TEST(MathTest, Sign)
{
    EXPECT_FLOAT_EQ(math::sign(-3.0f), -1.0f);
    EXPECT_FLOAT_EQ(math::sign(3.0f), 1.0f);
    EXPECT_FLOAT_EQ(math::sign(0.0f), 0.0f);
}

TEST(MathTest, Square)
{
    EXPECT_FLOAT_EQ(math::square(4.0f), 16.0f);
}

TEST(MathTest, Rsqrt)
{
    EXPECT_FLOAT_EQ(math::rsqrt(4.0f), 0.5f);
    EXPECT_FLOAT_EQ(math::rsqrt(16.0f), 0.25f);
}

TEST(MathTest, Step)
{
    EXPECT_FLOAT_EQ(math::step(0.5f, 0.7f), 1.0f);
    EXPECT_FLOAT_EQ(math::step(0.5f, 0.3f), 0.0f);
}

TEST(MathTest, Smoothstep)
{
    EXPECT_FLOAT_EQ(math::smoothstep(0.0f, 1.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(math::smoothstep(0.0f, 1.0f, 1.0f), 1.0f);
    EXPECT_FLOAT_EQ(math::smoothstep(0.0f, 1.0f, 0.5f), 0.5f);
}

TEST(MathTest, ApproxEqual)
{
    EXPECT_TRUE(math::approxEqual(1.0f, 1.0f + 1e-7f));
    EXPECT_FALSE(math::approxEqual(1.0f, 1.0f + 1e-3f));
}

TEST(MathTest, Classification)
{
    constexpr float inf = std::numeric_limits<float>::infinity();
    constexpr float nan = std::numeric_limits<float>::quiet_NaN();

    EXPECT_TRUE(math::isFinite(1.0f));
    EXPECT_FALSE(math::isFinite(inf));
    EXPECT_FALSE(math::isFinite(nan));

    EXPECT_TRUE(math::isNaN(nan));
    EXPECT_FALSE(math::isNaN(1.0f));

    EXPECT_TRUE(math::isInf(inf));
    EXPECT_FALSE(math::isInf(1.0f));
}

TEST(MathTest, ConstexprUsable)
{
    static_assert(math::min(1.0f, 2.0f) == 1.0f);
    static_assert(math::max(1.0f, 2.0f) == 2.0f);
    static_assert(math::clamp(5.0f, 0.0f, 1.0f) == 1.0f);
    static_assert(math::saturate(-2.0f) == 0.0f);
    static_assert(math::lerp(0.0f, 8.0f, 0.25f) == 2.0f);
    static_assert(math::PI > 3.0f);
    static_assert(math::radians(180.0f) > 3.14f && math::radians(180.0f) < 3.15f);
    static_assert(math::square(4.0f) == 16.0f);
    static_assert(math::sign(-3.0f) == -1.0f);
    static_assert(math::step(0.5f, 0.7f) == 1.0f);
    static_assert(math::smoothstep(0.0f, 1.0f, 0.5f) == 0.5f);
    static_assert(math::approxEqual(1.0f, 1.0f));
    SUCCEED();
}
