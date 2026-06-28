#include <gtest/gtest.h>

import worse.core.math;
import worse.core.math.vector;

namespace
{
    using worse::core::math::Float2;
    using worse::core::math::Float3;
    using worse::core::math::Float4;
    using worse::core::math::Vector2;
    using worse::core::math::Vector3;
    using worse::core::math::Vector4;

    void expectVec4(Vector4 const& v, float x, float y, float z, float w)
    {
        EXPECT_FLOAT_EQ(v.x(), x);
        EXPECT_FLOAT_EQ(v.y(), y);
        EXPECT_FLOAT_EQ(v.z(), z);
        EXPECT_FLOAT_EQ(v.w(), w);
    }

    void expectVec3(Vector3 const& v, float x, float y, float z)
    {
        EXPECT_FLOAT_EQ(v.x(), x);
        EXPECT_FLOAT_EQ(v.y(), y);
        EXPECT_FLOAT_EQ(v.z(), z);
    }

    void expectVec2(Vector2 const& v, float x, float y)
    {
        EXPECT_FLOAT_EQ(v.x(), x);
        EXPECT_FLOAT_EQ(v.y(), y);
    }
} // namespace

// --- Vector4 ---------------------------------------------------------------

TEST(Vector4Test, ComponentConstructor)
{
    expectVec4(Vector4{1.0f, 2.0f, 3.0f, 4.0f}, 1.0f, 2.0f, 3.0f, 4.0f);
}

TEST(Vector4Test, ScalarConstructor)
{
    expectVec4(Vector4{2.5f}, 2.5f, 2.5f, 2.5f, 2.5f);
}

TEST(Vector4Test, FromFloat4)
{
    expectVec4(Vector4{Float4{1.0f, 2.0f, 3.0f, 4.0f}}, 1.0f, 2.0f, 3.0f, 4.0f);
}

TEST(Vector4Test, ToFloat4)
{
    Float4 const f = Vector4{1.0f, 2.0f, 3.0f, 4.0f}.toFloat4();
    EXPECT_FLOAT_EQ(f.x, 1.0f);
    EXPECT_FLOAT_EQ(f.y, 2.0f);
    EXPECT_FLOAT_EQ(f.z, 3.0f);
    EXPECT_FLOAT_EQ(f.w, 4.0f);
}

TEST(Vector4Test, AddAssign)
{
    Vector4 a{1.0f, 2.0f, 3.0f, 4.0f};
    a += Vector4{10.0f, 20.0f, 30.0f, 40.0f};
    expectVec4(a, 11.0f, 22.0f, 33.0f, 44.0f);
}

TEST(Vector4Test, SubAssign)
{
    Vector4 a{10.0f, 20.0f, 30.0f, 40.0f};
    a -= Vector4{1.0f, 2.0f, 3.0f, 4.0f};
    expectVec4(a, 9.0f, 18.0f, 27.0f, 36.0f);
}

TEST(Vector4Test, MulAssignScalar)
{
    Vector4 a{1.0f, 2.0f, 3.0f, 4.0f};
    a *= 2.0f;
    expectVec4(a, 2.0f, 4.0f, 6.0f, 8.0f);
}

TEST(Vector4Test, DivAssignScalar)
{
    Vector4 a{2.0f, 4.0f, 6.0f, 8.0f};
    a /= 2.0f;
    expectVec4(a, 1.0f, 2.0f, 3.0f, 4.0f);
}

TEST(Vector4Test, OperatorAdd)
{
    Vector4 const a{1.0f, 2.0f, 3.0f, 4.0f};
    Vector4 const b{10.0f, 20.0f, 30.0f, 40.0f};
    expectVec4(a + b, 11.0f, 22.0f, 33.0f, 44.0f);
}

TEST(Vector4Test, OperatorSub)
{
    Vector4 const a{10.0f, 20.0f, 30.0f, 40.0f};
    Vector4 const b{1.0f, 2.0f, 3.0f, 4.0f};
    expectVec4(a - b, 9.0f, 18.0f, 27.0f, 36.0f);
}

TEST(Vector4Test, OperatorMulScalar)
{
    Vector4 const a{1.0f, 2.0f, 3.0f, 4.0f};
    expectVec4(a * 2.0f, 2.0f, 4.0f, 6.0f, 8.0f);
    expectVec4(2.0f * a, 2.0f, 4.0f, 6.0f, 8.0f);
}

// --- Vector3 ---------------------------------------------------------------

TEST(Vector3Test, ComponentConstructor)
{
    expectVec3(Vector3{1.0f, 2.0f, 3.0f}, 1.0f, 2.0f, 3.0f);
}

TEST(Vector3Test, ScalarConstructor)
{
    expectVec3(Vector3{2.5f}, 2.5f, 2.5f, 2.5f);
}

TEST(Vector3Test, FromFloat3)
{
    expectVec3(Vector3{Float3{1.0f, 2.0f, 3.0f}}, 1.0f, 2.0f, 3.0f);
}

TEST(Vector3Test, ToFloat3)
{
    Float3 const f = Vector3{1.0f, 2.0f, 3.0f}.toFloat3();
    EXPECT_FLOAT_EQ(f.x, 1.0f);
    EXPECT_FLOAT_EQ(f.y, 2.0f);
    EXPECT_FLOAT_EQ(f.z, 3.0f);
}

TEST(Vector3Test, AddAssign)
{
    Vector3 a{1.0f, 2.0f, 3.0f};
    a += Vector3{10.0f, 20.0f, 30.0f};
    expectVec3(a, 11.0f, 22.0f, 33.0f);
}

TEST(Vector3Test, SubAssign)
{
    Vector3 a{10.0f, 20.0f, 30.0f};
    a -= Vector3{1.0f, 2.0f, 3.0f};
    expectVec3(a, 9.0f, 18.0f, 27.0f);
}

TEST(Vector3Test, MulAssignScalar)
{
    Vector3 a{1.0f, 2.0f, 3.0f};
    a *= 3.0f;
    expectVec3(a, 3.0f, 6.0f, 9.0f);
}

TEST(Vector3Test, DivAssignScalar)
{
    Vector3 a{3.0f, 6.0f, 9.0f};
    a /= 3.0f;
    expectVec3(a, 1.0f, 2.0f, 3.0f);
}

TEST(Vector3Test, OperatorAdd)
{
    Vector3 const a{1.0f, 2.0f, 3.0f};
    Vector3 const b{10.0f, 20.0f, 30.0f};
    expectVec3(a + b, 11.0f, 22.0f, 33.0f);
}

TEST(Vector3Test, OperatorSub)
{
    Vector3 const a{10.0f, 20.0f, 30.0f};
    Vector3 const b{1.0f, 2.0f, 3.0f};
    expectVec3(a - b, 9.0f, 18.0f, 27.0f);
}

TEST(Vector3Test, OperatorMulScalar)
{
    Vector3 const a{1.0f, 2.0f, 3.0f};
    expectVec3(a * 3.0f, 3.0f, 6.0f, 9.0f);
    expectVec3(3.0f * a, 3.0f, 6.0f, 9.0f);
}

TEST(Vector3Test, Dot)
{
    Vector3 const a{1.0f, 2.0f, 3.0f};
    Vector3 const b{4.0f, 5.0f, 6.0f};
    // 1*4 + 2*5 + 3*6 = 32
    EXPECT_FLOAT_EQ(dot(a, b), 32.0f);
}

TEST(Vector3Test, Cross)
{
    Vector3 const x{1.0f, 0.0f, 0.0f};
    Vector3 const y{0.0f, 1.0f, 0.0f};
    // x cross y == z
    expectVec3(cross(x, y), 0.0f, 0.0f, 1.0f);
}

TEST(Vector3Test, LengthSquared)
{
    Vector3 const v{3.0f, 4.0f, 0.0f};
    // 3*3 + 4*4 + 0*0 = 25
    EXPECT_FLOAT_EQ(lengthSquared(v), 25.0f);
}

TEST(Vector3Test, Length)
{
    Vector3 const v{3.0f, 4.0f, 0.0f};
    // sqrt(25) = 5
    EXPECT_FLOAT_EQ(length(v), 5.0f);
}

// --- Vector2 ---------------------------------------------------------------

TEST(Vector2Test, ComponentConstructor)
{
    expectVec2(Vector2{1.0f, 2.0f}, 1.0f, 2.0f);
}

TEST(Vector2Test, ScalarConstructor)
{
    expectVec2(Vector2{3.5f}, 3.5f, 3.5f);
}

TEST(Vector2Test, FromFloat2)
{
    expectVec2(Vector2{Float2{5.0f, 6.0f}}, 5.0f, 6.0f);
}

TEST(Vector2Test, ToFloat2)
{
    Float2 const f = Vector2{7.0f, 8.0f}.toFloat2();
    EXPECT_FLOAT_EQ(f.x, 7.0f);
    EXPECT_FLOAT_EQ(f.y, 8.0f);
}

TEST(Vector2Test, AddAssign)
{
    Vector2 a{1.0f, 2.0f};
    a += Vector2{10.0f, 20.0f};
    expectVec2(a, 11.0f, 22.0f);
}

TEST(Vector2Test, SubAssign)
{
    Vector2 a{10.0f, 20.0f};
    a -= Vector2{1.0f, 2.0f};
    expectVec2(a, 9.0f, 18.0f);
}

TEST(Vector2Test, MulAssignScalar)
{
    Vector2 a{1.0f, 2.0f};
    a *= 3.0f;
    expectVec2(a, 3.0f, 6.0f);
}

TEST(Vector2Test, DivAssignScalar)
{
    Vector2 a{6.0f, 8.0f};
    a /= 2.0f;
    expectVec2(a, 3.0f, 4.0f);
}

TEST(Vector2Test, OperatorAdd)
{
    Vector2 const a{1.0f, 2.0f};
    Vector2 const b{10.0f, 20.0f};
    expectVec2(a + b, 11.0f, 22.0f);
}

TEST(Vector2Test, OperatorSub)
{
    Vector2 const a{10.0f, 20.0f};
    Vector2 const b{1.0f, 2.0f};
    expectVec2(a - b, 9.0f, 18.0f);
}

TEST(Vector2Test, OperatorMulScalar)
{
    Vector2 const a{1.0f, 2.0f};
    expectVec2(a * 2.0f, 2.0f, 4.0f);
    expectVec2(2.0f * a, 2.0f, 4.0f);
}

// --- Vector4: extended ops -------------------------------------------------

TEST(Vector4Test, Negate)
{
    expectVec4(-Vector4{1.0f, -2.0f, 3.0f, -4.0f}, -1.0f, 2.0f, -3.0f, 4.0f);
}

TEST(Vector4Test, OperatorDivScalar)
{
    expectVec4(Vector4{2.0f, 4.0f, 6.0f, 8.0f} / 2.0f, 1.0f, 2.0f, 3.0f, 4.0f);
}

TEST(Vector4Test, ComponentMul)
{
    Vector4 const a{1.0f, 2.0f, 3.0f, 4.0f};
    Vector4 const b{2.0f, 3.0f, 4.0f, 5.0f};
    expectVec4(a * b, 2.0f, 6.0f, 12.0f, 20.0f);
}

TEST(Vector4Test, ComponentDiv)
{
    Vector4 const a{10.0f, 20.0f, 30.0f, 40.0f};
    Vector4 const b{2.0f, 4.0f, 5.0f, 8.0f};
    expectVec4(a / b, 5.0f, 5.0f, 6.0f, 5.0f);
}

TEST(Vector4Test, Dot)
{
    Vector4 const a{1.0f, 2.0f, 3.0f, 4.0f};
    Vector4 const b{5.0f, 6.0f, 7.0f, 8.0f};
    // 1*5 + 2*6 + 3*7 + 4*8 = 70
    EXPECT_FLOAT_EQ(dot(a, b), 70.0f);
}

TEST(Vector4Test, LengthSquared)
{
    // 0 + 9 + 0 + 16 = 25
    EXPECT_FLOAT_EQ(lengthSquared(Vector4{0.0f, 3.0f, 0.0f, 4.0f}), 25.0f);
}

TEST(Vector4Test, Length)
{
    EXPECT_FLOAT_EQ(length(Vector4{0.0f, 3.0f, 0.0f, 4.0f}), 5.0f);
}

TEST(Vector4Test, Normalize)
{
    expectVec4(normalize(Vector4{0.0f, 3.0f, 0.0f, 4.0f}),
               0.0f,
               0.6f,
               0.0f,
               0.8f);
}

TEST(Vector4Test, NormalizeSafeZero)
{
    expectVec4(normalizeSafe(Vector4{0.0f}), 0.0f, 0.0f, 0.0f, 0.0f);
}

TEST(Vector4Test, Distance)
{
    Vector4 const a{1.0f, 2.0f, 3.0f, 4.0f};
    Vector4 const b{1.0f, 5.0f, 7.0f, 4.0f};
    EXPECT_FLOAT_EQ(distanceSquared(a, b), 25.0f);
    EXPECT_FLOAT_EQ(distance(a, b), 5.0f);
}

// --- Vector3: extended ops -------------------------------------------------

TEST(Vector3Test, Negate)
{
    expectVec3(-Vector3{1.0f, -2.0f, 3.0f}, -1.0f, 2.0f, -3.0f);
}

TEST(Vector3Test, OperatorDivScalar)
{
    expectVec3(Vector3{3.0f, 6.0f, 9.0f} / 3.0f, 1.0f, 2.0f, 3.0f);
}

TEST(Vector3Test, ComponentMul)
{
    Vector3 const a{1.0f, 2.0f, 3.0f};
    Vector3 const b{2.0f, 3.0f, 4.0f};
    expectVec3(a * b, 2.0f, 6.0f, 12.0f);
}

TEST(Vector3Test, ComponentDiv)
{
    Vector3 const a{6.0f, 12.0f, 20.0f};
    Vector3 const b{2.0f, 3.0f, 4.0f};
    Vector3 const c = a / b;
    expectVec3(c, 3.0f, 4.0f, 5.0f);
    // A finite lengthSquared confirms the unused w lane stayed 0 (a stray
    // 0/0 == NaN there would poison dot3).
    EXPECT_FLOAT_EQ(lengthSquared(c), 50.0f);
}

TEST(Vector3Test, Normalize)
{
    expectVec3(normalize(Vector3{3.0f, 4.0f, 0.0f}), 0.6f, 0.8f, 0.0f);
}

TEST(Vector3Test, NormalizeIsUnitLength)
{
    EXPECT_FLOAT_EQ(length(normalize(Vector3{1.0f, 2.0f, 3.0f})), 1.0f);
}

TEST(Vector3Test, NormalizeSafeZero)
{
    expectVec3(normalizeSafe(Vector3{0.0f}), 0.0f, 0.0f, 0.0f);
}

TEST(Vector3Test, Distance)
{
    Vector3 const a{1.0f, 2.0f, 3.0f};
    Vector3 const b{4.0f, 6.0f, 3.0f};
    EXPECT_FLOAT_EQ(distanceSquared(a, b), 25.0f);
    EXPECT_FLOAT_EQ(distance(a, b), 5.0f);
}

// --- Vector2: extended ops -------------------------------------------------

TEST(Vector2Test, Negate)
{
    expectVec2(-Vector2{1.0f, -2.0f}, -1.0f, 2.0f);
}

TEST(Vector2Test, OperatorDivScalar)
{
    expectVec2(Vector2{6.0f, 8.0f} / 2.0f, 3.0f, 4.0f);
}

TEST(Vector2Test, ComponentMul)
{
    expectVec2(Vector2{1.0f, 2.0f} * Vector2{3.0f, 4.0f}, 3.0f, 8.0f);
}

TEST(Vector2Test, ComponentDiv)
{
    expectVec2(Vector2{6.0f, 8.0f} / Vector2{2.0f, 4.0f}, 3.0f, 2.0f);
}

TEST(Vector2Test, Dot)
{
    // 1*3 + 2*4 = 11
    EXPECT_FLOAT_EQ(dot(Vector2{1.0f, 2.0f}, Vector2{3.0f, 4.0f}), 11.0f);
}

TEST(Vector2Test, LengthSquared)
{
    EXPECT_FLOAT_EQ(lengthSquared(Vector2{3.0f, 4.0f}), 25.0f);
}

TEST(Vector2Test, Length)
{
    EXPECT_FLOAT_EQ(length(Vector2{3.0f, 4.0f}), 5.0f);
}

TEST(Vector2Test, Normalize)
{
    expectVec2(normalize(Vector2{3.0f, 4.0f}), 0.6f, 0.8f);
}

TEST(Vector2Test, NormalizeSafeZero)
{
    expectVec2(normalizeSafe(Vector2{0.0f}), 0.0f, 0.0f);
}

TEST(Vector2Test, Distance)
{
    Vector2 const a{1.0f, 2.0f};
    Vector2 const b{4.0f, 6.0f};
    EXPECT_FLOAT_EQ(distanceSquared(a, b), 25.0f);
    EXPECT_FLOAT_EQ(distance(a, b), 5.0f);
}

// --- Vector4: tier-2 ops ---------------------------------------------------

TEST(Vector4Test, Lerp)
{
    Vector4 const a{0.0f, 0.0f, 0.0f, 0.0f};
    Vector4 const b{10.0f, 20.0f, 30.0f, 40.0f};
    expectVec4(lerp(a, b, 0.0f), 0.0f, 0.0f, 0.0f, 0.0f);
    expectVec4(lerp(a, b, 1.0f), 10.0f, 20.0f, 30.0f, 40.0f);
    expectVec4(lerp(a, b, 0.5f), 5.0f, 10.0f, 15.0f, 20.0f);
}

TEST(Vector4Test, MinMax)
{
    Vector4 const a{1.0f, 5.0f, 3.0f, 8.0f};
    Vector4 const b{4.0f, 2.0f, 3.0f, 6.0f};
    expectVec4(min(a, b), 1.0f, 2.0f, 3.0f, 6.0f);
    expectVec4(max(a, b), 4.0f, 5.0f, 3.0f, 8.0f);
}

TEST(Vector4Test, Abs)
{
    expectVec4(abs(Vector4{-1.0f, 2.0f, -3.0f, -4.0f}), 1.0f, 2.0f, 3.0f, 4.0f);
}

TEST(Vector4Test, Clamp)
{
    Vector4 const v{-1.0f, 5.0f, 0.5f, 2.0f};
    expectVec4(clamp(v, Vector4{0.0f}, Vector4{1.0f}), 0.0f, 1.0f, 0.5f, 1.0f);
    expectVec4(clamp(v, 0.0f, 1.0f), 0.0f, 1.0f, 0.5f, 1.0f);
}

TEST(Vector4Test, Saturate)
{
    expectVec4(saturate(Vector4{-1.0f, 2.0f, 0.3f, 0.7f}), 0.0f, 1.0f, 0.3f, 0.7f);
}

TEST(Vector4Test, Equality)
{
    Vector4 const a{1.0f, 2.0f, 3.0f, 4.0f};
    Vector4 const b{1.0f, 2.0f, 3.0f, 4.0f};
    Vector4 const c{1.0f, 2.0f, 3.0f, 5.0f};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(Vector4Test, ApproxEqual)
{
    Vector4 const a{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_TRUE(approxEqual(a, a));
    EXPECT_TRUE(approxEqual(a, a + Vector4{1.0e-7f, 0.0f, 0.0f, 0.0f}));
    EXPECT_FALSE(approxEqual(a, a + Vector4{1.0f, 0.0f, 0.0f, 0.0f}));
}

TEST(Vector4Test, Reflect)
{
    Vector4 const i{1.0f, -1.0f, 0.0f, 0.0f};
    Vector4 const n{0.0f, 1.0f, 0.0f, 0.0f};
    expectVec4(reflect(i, n), 1.0f, 1.0f, 0.0f, 0.0f);
}

TEST(Vector4Test, Refract)
{
    Vector4 const n{0.0f, 1.0f, 0.0f, 0.0f};
    // eta == 1 leaves the incident vector unchanged.
    expectVec4(refract(Vector4{1.0f, -1.0f, 0.0f, 0.0f}, n, 1.0f),
               1.0f,
               -1.0f,
               0.0f,
               0.0f);
    // Total internal reflection collapses to the zero vector.
    expectVec4(refract(normalize(Vector4{1.0f, -1.0f, 0.0f, 0.0f}), n, 2.0f),
               0.0f,
               0.0f,
               0.0f,
               0.0f);
}

TEST(Vector4Test, ProjectReject)
{
    Vector4 const a{2.0f, 3.0f, 0.0f, 0.0f};
    Vector4 const b{1.0f, 0.0f, 0.0f, 0.0f};
    expectVec4(project(a, b), 2.0f, 0.0f, 0.0f, 0.0f);
    expectVec4(reject(a, b), 0.0f, 3.0f, 0.0f, 0.0f);
}

// --- Vector3: tier-2 ops ---------------------------------------------------

TEST(Vector3Test, Lerp)
{
    Vector3 const a{0.0f, 0.0f, 0.0f};
    Vector3 const b{10.0f, 20.0f, 30.0f};
    expectVec3(lerp(a, b, 0.0f), 0.0f, 0.0f, 0.0f);
    expectVec3(lerp(a, b, 1.0f), 10.0f, 20.0f, 30.0f);
    expectVec3(lerp(a, b, 0.5f), 5.0f, 10.0f, 15.0f);
}

TEST(Vector3Test, MinMax)
{
    Vector3 const a{1.0f, 5.0f, 3.0f};
    Vector3 const b{4.0f, 2.0f, 3.0f};
    expectVec3(min(a, b), 1.0f, 2.0f, 3.0f);
    expectVec3(max(a, b), 4.0f, 5.0f, 3.0f);
}

TEST(Vector3Test, Abs)
{
    expectVec3(abs(Vector3{-1.0f, 2.0f, -3.0f}), 1.0f, 2.0f, 3.0f);
}

TEST(Vector3Test, Clamp)
{
    Vector3 const v{-1.0f, 5.0f, 0.5f};
    expectVec3(clamp(v, Vector3{0.0f}, Vector3{1.0f}), 0.0f, 1.0f, 0.5f);
    expectVec3(clamp(v, 0.0f, 1.0f), 0.0f, 1.0f, 0.5f);
}

TEST(Vector3Test, Saturate)
{
    expectVec3(saturate(Vector3{-1.0f, 2.0f, 0.3f}), 0.0f, 1.0f, 0.3f);
}

TEST(Vector3Test, Equality)
{
    Vector3 const a{1.0f, 2.0f, 3.0f};
    Vector3 const b{1.0f, 2.0f, 3.0f};
    Vector3 const c{1.0f, 2.0f, 4.0f};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(Vector3Test, ApproxEqual)
{
    Vector3 const a{1.0f, 2.0f, 3.0f};
    EXPECT_TRUE(approxEqual(a, a));
    EXPECT_TRUE(approxEqual(a, a + Vector3{1.0e-7f, 0.0f, 0.0f}));
    EXPECT_FALSE(approxEqual(a, a + Vector3{1.0f, 0.0f, 0.0f}));
}

TEST(Vector3Test, Reflect)
{
    // Incident vector bouncing off a floor with an upward normal.
    Vector3 const i{1.0f, -1.0f, 0.0f};
    Vector3 const n{0.0f, 1.0f, 0.0f};
    expectVec3(reflect(i, n), 1.0f, 1.0f, 0.0f);
}

TEST(Vector3Test, Refract)
{
    Vector3 const n{0.0f, 1.0f, 0.0f};
    // eta == 1 leaves the incident vector unchanged.
    expectVec3(refract(Vector3{1.0f, -1.0f, 0.0f}, n, 1.0f), 1.0f, -1.0f, 0.0f);
    // Total internal reflection collapses to the zero vector.
    expectVec3(refract(normalize(Vector3{1.0f, -1.0f, 0.0f}), n, 2.0f),
               0.0f,
               0.0f,
               0.0f);
}

TEST(Vector3Test, ProjectReject)
{
    Vector3 const a{2.0f, 3.0f, 0.0f};
    Vector3 const b{1.0f, 0.0f, 0.0f};
    Vector3 const p = project(a, b);
    Vector3 const r = reject(a, b);
    expectVec3(p, 2.0f, 0.0f, 0.0f);
    expectVec3(r, 0.0f, 3.0f, 0.0f);
    // project + reject reconstructs the original vector.
    expectVec3(p + r, 2.0f, 3.0f, 0.0f);
}

// --- Vector2: tier-2 ops ---------------------------------------------------

TEST(Vector2Test, Lerp)
{
    Vector2 const a{0.0f, 0.0f};
    Vector2 const b{10.0f, 20.0f};
    expectVec2(lerp(a, b, 0.0f), 0.0f, 0.0f);
    expectVec2(lerp(a, b, 1.0f), 10.0f, 20.0f);
    expectVec2(lerp(a, b, 0.5f), 5.0f, 10.0f);
}

TEST(Vector2Test, MinMax)
{
    Vector2 const a{1.0f, 5.0f};
    Vector2 const b{4.0f, 2.0f};
    expectVec2(min(a, b), 1.0f, 2.0f);
    expectVec2(max(a, b), 4.0f, 5.0f);
}

TEST(Vector2Test, Abs)
{
    expectVec2(abs(Vector2{-1.0f, 2.0f}), 1.0f, 2.0f);
}

TEST(Vector2Test, Clamp)
{
    Vector2 const v{-1.0f, 5.0f};
    expectVec2(clamp(v, Vector2{0.0f}, Vector2{1.0f}), 0.0f, 1.0f);
    expectVec2(clamp(v, 0.0f, 1.0f), 0.0f, 1.0f);
}

TEST(Vector2Test, Saturate)
{
    expectVec2(saturate(Vector2{-1.0f, 0.3f}), 0.0f, 0.3f);
}

TEST(Vector2Test, Equality)
{
    Vector2 const a{1.0f, 2.0f};
    Vector2 const b{1.0f, 2.0f};
    Vector2 const c{1.0f, 3.0f};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(Vector2Test, ApproxEqual)
{
    Vector2 const a{1.0f, 2.0f};
    EXPECT_TRUE(approxEqual(a, a));
    EXPECT_TRUE(approxEqual(a, a + Vector2{1.0e-7f, 0.0f}));
    EXPECT_FALSE(approxEqual(a, a + Vector2{1.0f, 0.0f}));
}

TEST(Vector2Test, Reflect)
{
    Vector2 const i{1.0f, -1.0f};
    Vector2 const n{0.0f, 1.0f};
    expectVec2(reflect(i, n), 1.0f, 1.0f);
}

TEST(Vector2Test, Refract)
{
    Vector2 const n{0.0f, 1.0f};
    // eta == 1 leaves the incident vector unchanged.
    expectVec2(refract(Vector2{1.0f, -1.0f}, n, 1.0f), 1.0f, -1.0f);
    // Total internal reflection collapses to the zero vector.
    expectVec2(refract(normalize(Vector2{1.0f, -1.0f}), n, 2.0f), 0.0f, 0.0f);
}

TEST(Vector2Test, ProjectReject)
{
    Vector2 const a{2.0f, 3.0f};
    Vector2 const b{1.0f, 0.0f};
    expectVec2(project(a, b), 2.0f, 0.0f);
    expectVec2(reject(a, b), 0.0f, 3.0f);
}

// --- Named constants -------------------------------------------------------

TEST(Vector4Test, Constants)
{
    expectVec4(Vector4::Zero, 0.0f, 0.0f, 0.0f, 0.0f);
    expectVec4(Vector4::One, 1.0f, 1.0f, 1.0f, 1.0f);
    expectVec4(Vector4::UnitX, 1.0f, 0.0f, 0.0f, 0.0f);
    expectVec4(Vector4::UnitY, 0.0f, 1.0f, 0.0f, 0.0f);
    expectVec4(Vector4::UnitZ, 0.0f, 0.0f, 1.0f, 0.0f);
    expectVec4(Vector4::UnitW, 0.0f, 0.0f, 0.0f, 1.0f);
}

TEST(Vector3Test, Constants)
{
    expectVec3(Vector3::Zero, 0.0f, 0.0f, 0.0f);
    expectVec3(Vector3::One, 1.0f, 1.0f, 1.0f);
    expectVec3(Vector3::UnitX, 1.0f, 0.0f, 0.0f);
    expectVec3(Vector3::UnitY, 0.0f, 1.0f, 0.0f);
    expectVec3(Vector3::UnitZ, 0.0f, 0.0f, 1.0f);
}

TEST(Vector2Test, Constants)
{
    expectVec2(Vector2::Zero, 0.0f, 0.0f);
    expectVec2(Vector2::One, 1.0f, 1.0f);
    expectVec2(Vector2::UnitX, 1.0f, 0.0f);
    expectVec2(Vector2::UnitY, 0.0f, 1.0f);
}
