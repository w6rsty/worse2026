#include <gtest/gtest.h>

#include <cmath>

import worse.core.basic_type;

namespace
{
    namespace bt = worse;
} // namespace

TEST(BasicTypesTest, UnsignedLimits)
{
    static_assert(bt::U8_MIN == 0);
    static_assert(bt::U8_MAX == 255);
    static_assert(bt::U16_MAX == 65535);
    static_assert(bt::U32_MAX == 4294967295u);
    static_assert(bt::U64_MAX == 18446744073709551615ull);
    static_assert(bt::U32_MIN < bt::U32_MAX);
    static_assert(bt::USIZE_MIN == 0);
    static_assert(bt::USIZE_MIN < bt::USIZE_MAX);
    SUCCEED();
}

TEST(BasicTypesTest, SignedLimits)
{
    static_assert(bt::I8_MIN == -128);
    static_assert(bt::I8_MAX == 127);
    static_assert(bt::I16_MIN == -32768);
    static_assert(bt::I32_MIN == -2147483647 - 1);
    static_assert(bt::I32_MAX == 2147483647);
    static_assert(bt::I64_MIN < bt::I64_MAX);
    static_assert(bt::ISIZE_MIN < bt::ISIZE_MAX);
    SUCCEED();
}

TEST(BasicTypesTest, FloatLimits)
{
    static_assert(bt::F32_MIN > 0.0f);
    static_assert(bt::F32_MAX > 0.0f);
    static_assert(bt::F32_LOWEST < 0.0f);
    static_assert(bt::F32_EPSILON > 0.0f);
    static_assert(bt::F64_MIN > 0.0);
    static_assert(bt::F64_LOWEST < 0.0);

    EXPECT_TRUE(std::isinf(bt::F32_INFINITY));
    EXPECT_GT(bt::F32_INFINITY, bt::F32_MAX);
    EXPECT_TRUE(std::isnan(bt::F32_NAN));

    EXPECT_TRUE(std::isinf(bt::F64_INFINITY));
    EXPECT_GT(bt::F64_INFINITY, bt::F64_MAX);
    EXPECT_TRUE(std::isnan(bt::F64_NAN));
}
