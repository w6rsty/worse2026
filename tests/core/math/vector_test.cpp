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
    EXPECT_FLOAT_EQ(v.lengthSquared(), 25.0f);
}

TEST(Vector3Test, Length)
{
    Vector3 const v{3.0f, 4.0f, 0.0f};
    // sqrt(25) = 5
    EXPECT_FLOAT_EQ(v.length(), 5.0f);
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
