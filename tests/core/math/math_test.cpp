#include <gtest/gtest.h>

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

TEST(MathTest, ConstexprUsable)
{
    static_assert(math::min(1.0f, 2.0f) == 1.0f);
    static_assert(math::max(1.0f, 2.0f) == 2.0f);
    static_assert(math::clamp(5.0f, 0.0f, 1.0f) == 1.0f);
    static_assert(math::saturate(-2.0f) == 0.0f);
    static_assert(math::lerp(0.0f, 8.0f, 0.25f) == 2.0f);
    SUCCEED();
}
