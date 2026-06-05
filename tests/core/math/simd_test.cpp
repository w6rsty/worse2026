#include <gtest/gtest.h>

#include <array>

import worse.core.math.simd;

#if defined(WE_ARCH_AARCH64) || defined(WE_ARCH_AMD64)

namespace
{
    namespace simd = worse::core::math::simd;

    // Spill a SIMD register into a plain array so individual lanes can be
    // compared with EXPECT_FLOAT_EQ.
    std::array<float, 4> toArray(simd::f32x4 v) noexcept
    {
        alignas(16) float out[4] = {};
        simd::storeu(out, v);
        return {out[0], out[1], out[2], out[3]};
    }

    void expectLanes(simd::f32x4 v, float x, float y, float z, float w)
    {
        auto const a = toArray(v);
        EXPECT_FLOAT_EQ(a[0], x);
        EXPECT_FLOAT_EQ(a[1], y);
        EXPECT_FLOAT_EQ(a[2], z);
        EXPECT_FLOAT_EQ(a[3], w);
    }
} // namespace

TEST(SimdTest, LoadStoreRoundtrip)
{
    alignas(16) float const src[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    simd::f32x4 const v            = simd::loadu(src);

    alignas(16) float dst[4] = {};
    simd::storeu(dst, v);

    EXPECT_FLOAT_EQ(dst[0], 1.0f);
    EXPECT_FLOAT_EQ(dst[1], 2.0f);
    EXPECT_FLOAT_EQ(dst[2], 3.0f);
    EXPECT_FLOAT_EQ(dst[3], 4.0f);
}

TEST(SimdTest, Set)
{
    expectLanes(simd::set(5.0f, 6.0f, 7.0f, 8.0f), 5.0f, 6.0f, 7.0f, 8.0f);
}

TEST(SimdTest, Splat)
{
    expectLanes(simd::splat(3.5f), 3.5f, 3.5f, 3.5f, 3.5f);
}

TEST(SimdTest, Zero)
{
    expectLanes(simd::zero(), 0.0f, 0.0f, 0.0f, 0.0f);
}

TEST(SimdTest, Add)
{
    simd::f32x4 const lhs = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    simd::f32x4 const rhs = simd::set(10.0f, 20.0f, 30.0f, 40.0f);
    expectLanes(simd::add(lhs, rhs), 11.0f, 22.0f, 33.0f, 44.0f);
}

TEST(SimdTest, Sub)
{
    simd::f32x4 const lhs = simd::set(10.0f, 20.0f, 30.0f, 40.0f);
    simd::f32x4 const rhs = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    expectLanes(simd::sub(lhs, rhs), 9.0f, 18.0f, 27.0f, 36.0f);
}

TEST(SimdTest, Mul)
{
    simd::f32x4 const lhs = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    simd::f32x4 const rhs = simd::set(2.0f, 3.0f, 4.0f, 5.0f);
    expectLanes(simd::mul(lhs, rhs), 2.0f, 6.0f, 12.0f, 20.0f);
}

TEST(SimdTest, Div)
{
    simd::f32x4 const lhs = simd::set(10.0f, 20.0f, 30.0f, 40.0f);
    simd::f32x4 const rhs = simd::set(2.0f, 4.0f, 5.0f, 8.0f);
    expectLanes(simd::div(lhs, rhs), 5.0f, 5.0f, 6.0f, 5.0f);
}

TEST(SimdTest, Negate)
{
    expectLanes(simd::negate(simd::set(1.0f, -2.0f, 3.0f, -4.0f)),
                -1.0f,
                2.0f,
                -3.0f,
                4.0f);
}

TEST(SimdTest, Fmadd)
{
    // fmadd(a, b, c) == c + a * b, lane-wise.
    simd::f32x4 const a = simd::set(2.0f, 3.0f, 4.0f, 5.0f);
    simd::f32x4 const b = simd::splat(10.0f);
    simd::f32x4 const c = simd::splat(1.0f);
    expectLanes(simd::fmadd(a, b, c), 21.0f, 31.0f, 41.0f, 51.0f);
}

TEST(SimdTest, GetLane0)
{
    EXPECT_FLOAT_EQ(simd::getLane0(simd::set(9.0f, 8.0f, 7.0f, 6.0f)), 9.0f);
}

TEST(SimdTest, HorizontalAdd)
{
    EXPECT_FLOAT_EQ(simd::hadd4(simd::set(1.0f, 2.0f, 3.0f, 4.0f)), 10.0f);
    EXPECT_FLOAT_EQ(simd::hadd4(simd::zero()), 0.0f);
}

TEST(SimdTest, ShuffleReverse)
{
    simd::f32x4 const v = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    expectLanes(simd::shuffle<3, 2, 1, 0>(v), 4.0f, 3.0f, 2.0f, 1.0f);
}

TEST(SimdTest, ShuffleBroadcastPattern)
{
    simd::f32x4 const v = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    expectLanes(simd::shuffle<0, 0, 2, 2>(v), 1.0f, 1.0f, 3.0f, 3.0f);
}

TEST(SimdTest, Transpose4)
{
    simd::f32x4 r0 = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    simd::f32x4 r1 = simd::set(5.0f, 6.0f, 7.0f, 8.0f);
    simd::f32x4 r2 = simd::set(9.0f, 10.0f, 11.0f, 12.0f);
    simd::f32x4 r3 = simd::set(13.0f, 14.0f, 15.0f, 16.0f);

    simd::transpose4(r0, r1, r2, r3);

    expectLanes(r0, 1.0f, 5.0f, 9.0f, 13.0f);
    expectLanes(r1, 2.0f, 6.0f, 10.0f, 14.0f);
    expectLanes(r2, 3.0f, 7.0f, 11.0f, 15.0f);
    expectLanes(r3, 4.0f, 8.0f, 12.0f, 16.0f);
}

TEST(SimdTest, Dot4)
{
    simd::f32x4 const a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    simd::f32x4 const b = simd::set(5.0f, 6.0f, 7.0f, 8.0f);
    // 1*5 + 2*6 + 3*7 + 4*8 = 70
    EXPECT_FLOAT_EQ(simd::dot4(a, b), 70.0f);
}

TEST(SimdTest, Dot3)
{
    simd::f32x4 const a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    simd::f32x4 const b = simd::set(5.0f, 6.0f, 7.0f, 8.0f);
    // 1*5 + 2*6 + 3*7 = 38; the w lane (4*8) is masked out.
    EXPECT_FLOAT_EQ(simd::dot3(a, b), 38.0f);
}

TEST(SimdTest, Cross3BasisVectors)
{
    simd::f32x4 const x = simd::set(1.0f, 0.0f, 0.0f, 0.0f);
    simd::f32x4 const y = simd::set(0.0f, 1.0f, 0.0f, 0.0f);
    // x cross y == z
    expectLanes(simd::cross3(x, y), 0.0f, 0.0f, 1.0f, 0.0f);
}

TEST(SimdTest, Cross3General)
{
    simd::f32x4 const lhs = simd::set(1.0f, 2.0f, 3.0f, 0.0f);
    simd::f32x4 const rhs = simd::set(4.0f, 5.0f, 6.0f, 0.0f);
    // (2*6-3*5, 3*4-1*6, 1*5-2*4, 0) = (-3, 6, -3, 0)
    expectLanes(simd::cross3(lhs, rhs), -3.0f, 6.0f, -3.0f, 0.0f);
}

TEST(SimdTest, QuatMulIdentity)
{
    // Identity quaternion in (x, y, z, w) layout.
    simd::f32x4 const identity = simd::set(0.0f, 0.0f, 0.0f, 1.0f);
    simd::f32x4 const q        = simd::set(0.1f, 0.2f, 0.3f, 0.4f);

    expectLanes(simd::quatMul(identity, q), 0.1f, 0.2f, 0.3f, 0.4f);
    expectLanes(simd::quatMul(q, identity), 0.1f, 0.2f, 0.3f, 0.4f);
}

TEST(SimdTest, QuatMulBasis)
{
    // Pure-imaginary unit quaternions: i * j == k.
    simd::f32x4 const i = simd::set(1.0f, 0.0f, 0.0f, 0.0f);
    simd::f32x4 const j = simd::set(0.0f, 1.0f, 0.0f, 0.0f);
    expectLanes(simd::quatMul(i, j), 0.0f, 0.0f, 1.0f, 0.0f);
}

TEST(SimdTest, Mat4MulVecIdentity)
{
    simd::f32x4 const r0 = simd::set(1.0f, 0.0f, 0.0f, 0.0f);
    simd::f32x4 const r1 = simd::set(0.0f, 1.0f, 0.0f, 0.0f);
    simd::f32x4 const r2 = simd::set(0.0f, 0.0f, 1.0f, 0.0f);
    simd::f32x4 const r3 = simd::set(0.0f, 0.0f, 0.0f, 1.0f);
    simd::f32x4 const v  = simd::set(2.0f, 3.0f, 4.0f, 5.0f);

    expectLanes(simd::mat4MulVec(r0, r1, r2, r3, v), 2.0f, 3.0f, 4.0f, 5.0f);
}

TEST(SimdTest, Mat4MulVecLinearCombination)
{
    // result == r0*v.x + r1*v.y + r2*v.z + r3*v.w
    simd::f32x4 const r0 = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    simd::f32x4 const r1 = simd::set(5.0f, 6.0f, 7.0f, 8.0f);
    simd::f32x4 const r2 = simd::set(9.0f, 10.0f, 11.0f, 12.0f);
    simd::f32x4 const r3 = simd::set(13.0f, 14.0f, 15.0f, 16.0f);
    simd::f32x4 const v  = simd::splat(1.0f);

    expectLanes(simd::mat4MulVec(r0, r1, r2, r3, v), 28.0f, 32.0f, 36.0f, 40.0f);
}

TEST(SimdTest, LengthSq3)
{
    // 3*3 + 4*4 + 0*0 = 25; the w lane is ignored.
    EXPECT_FLOAT_EQ(simd::lengthSq3(simd::set(3.0f, 4.0f, 0.0f, 5.0f)), 25.0f);
}

TEST(SimdTest, LengthSq4)
{
    // 1 + 4 + 9 + 16 = 30
    EXPECT_FLOAT_EQ(simd::lengthSq4(simd::set(1.0f, 2.0f, 3.0f, 4.0f)), 30.0f);
}

TEST(SimdTest, Sqrt)
{
    expectLanes(simd::sqrt(simd::set(1.0f, 4.0f, 9.0f, 16.0f)),
                1.0f,
                2.0f,
                3.0f,
                4.0f);
}

TEST(SimdTest, Rsqrt)
{
    // rsqrt(x) == 1 / sqrt(x), lane-wise.
    expectLanes(simd::rsqrt(simd::set(1.0f, 4.0f, 16.0f, 0.25f)),
                1.0f,
                0.5f,
                0.25f,
                2.0f);
}

TEST(SimdTest, Min)
{
    simd::f32x4 const lhs = simd::set(1.0f, 5.0f, 3.0f, 8.0f);
    simd::f32x4 const rhs = simd::set(4.0f, 2.0f, 3.0f, 6.0f);
    expectLanes(simd::min(lhs, rhs), 1.0f, 2.0f, 3.0f, 6.0f);
}

TEST(SimdTest, Max)
{
    simd::f32x4 const lhs = simd::set(1.0f, 5.0f, 3.0f, 8.0f);
    simd::f32x4 const rhs = simd::set(4.0f, 2.0f, 3.0f, 6.0f);
    expectLanes(simd::max(lhs, rhs), 4.0f, 5.0f, 3.0f, 8.0f);
}

TEST(SimdTest, Abs)
{
    expectLanes(simd::abs(simd::set(-1.0f, 2.0f, -3.0f, -4.0f)),
                1.0f,
                2.0f,
                3.0f,
                4.0f);
}

#endif // SIMD-capable arch
