#include <gtest/gtest.h>

import worse.core.math;
import worse.core.math.vector;
import worse.core.math.matrix;

namespace
{
    using worse::core::math::Matrix2;
    using worse::core::math::Matrix3;
    using worse::core::math::Matrix4;
    using worse::core::math::Vector2;
    using worse::core::math::Vector3;
    using worse::core::math::Vector4;

    void expectMat4(Matrix4 const& actual, Matrix4 const& expected)
    {
        for (unsigned r = 0; r < 4; ++r)
        {
            for (unsigned c = 0; c < 4; ++c)
            {
                EXPECT_FLOAT_EQ(actual(r, c), expected(r, c)) << "at (" << r << ", " << c << ")";
            }
        }
    }

    void expectMat3(Matrix3 const& actual, Matrix3 const& expected)
    {
        for (unsigned r = 0; r < 3; ++r)
        {
            for (unsigned c = 0; c < 3; ++c)
            {
                EXPECT_FLOAT_EQ(actual(r, c), expected(r, c)) << "at (" << r << ", " << c << ")";
            }
        }
    }

    void expectMat2(Matrix2 const& actual, Matrix2 const& expected)
    {
        for (unsigned r = 0; r < 2; ++r)
        {
            for (unsigned c = 0; c < 2; ++c)
            {
                EXPECT_FLOAT_EQ(actual(r, c), expected(r, c)) << "at (" << r << ", " << c << ")";
            }
        }
    }

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

// --- Matrix4 ---------------------------------------------------------------

TEST(Matrix4Test, ElementConstructor)
{
    Matrix4 const m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};
    EXPECT_FLOAT_EQ(m(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m(1, 2), 7.0f);
    EXPECT_FLOAT_EQ(m(3, 3), 16.0f);
}

TEST(Matrix4Test, DiagonalConstructor)
{
    expectMat4(Matrix4{2.0f}, Matrix4{2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f});
}

TEST(Matrix4Test, Identity)
{
    expectMat4(Matrix4::Identity, Matrix4{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
}

TEST(Matrix4Test, Zero)
{
    expectMat4(Matrix4::Zero, Matrix4{0.0f});
}

TEST(Matrix4Test, Add)
{
    Matrix4 const a{1.0f};
    Matrix4 const b{2.0f};
    expectMat4(a + b, Matrix4{3.0f});
}

TEST(Matrix4Test, Sub)
{
    Matrix4 const a{5.0f};
    Matrix4 const b{2.0f};
    expectMat4(a - b, Matrix4{3.0f});
}

TEST(Matrix4Test, ScalarMul)
{
    expectMat4(Matrix4{3.0f} * 2.0f, Matrix4{6.0f});
    expectMat4(2.0f * Matrix4{3.0f}, Matrix4{6.0f});
}

TEST(Matrix4Test, ScalarDiv)
{
    expectMat4(Matrix4{6.0f} / 2.0f, Matrix4{3.0f});
}

TEST(Matrix4Test, Negate)
{
    expectMat4(-Matrix4{3.0f}, Matrix4{-3.0f});
}

TEST(Matrix4Test, CompoundAssign)
{
    Matrix4 a{4.0f};
    a += Matrix4{1.0f};
    expectMat4(a, Matrix4{5.0f});
    a -= Matrix4{2.0f};
    expectMat4(a, Matrix4{3.0f});
    a *= 2.0f;
    expectMat4(a, Matrix4{6.0f});
    a /= 3.0f;
    expectMat4(a, Matrix4{2.0f});
}

TEST(Matrix4Test, MultiplyIdentity)
{
    Matrix4 const m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};
    expectMat4(m * Matrix4::Identity, m);
    expectMat4(Matrix4::Identity * m, m);
}

TEST(Matrix4Test, Multiply)
{
    Matrix4 const a{1.0f, 2.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    Matrix4 const b{1.0f, 0.0f, 0.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    expectMat4(a * b, Matrix4{7.0f, 2.0f, 0.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
}

TEST(Matrix4Test, MultiplyAssign)
{
    Matrix4 a{1.0f, 2.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    a *= a;
    expectMat4(a, Matrix4{1.0f, 4.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
}

TEST(Matrix4Test, VectorMultiplyIdentity)
{
    expectVec4(Vector4{1.0f, 2.0f, 3.0f, 4.0f} * Matrix4::Identity, 1.0f, 2.0f, 3.0f, 4.0f);
}

TEST(Matrix4Test, VectorMultiply)
{
    Matrix4 const m{1.0f, 0.0f, 0.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    expectVec4(Vector4{1.0f, 1.0f, 0.0f, 0.0f} * m, 4.0f, 1.0f, 0.0f, 0.0f);
}

TEST(Matrix4Test, Transpose)
{
    Matrix4 const m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};
    expectMat4(transpose(m), Matrix4{1.0f, 5.0f, 9.0f, 13.0f, 2.0f, 6.0f, 10.0f, 14.0f, 3.0f, 7.0f, 11.0f, 15.0f, 4.0f, 8.0f, 12.0f, 16.0f});
}

TEST(Matrix4Test, TransposeInvolution)
{
    Matrix4 const m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};
    expectMat4(transpose(transpose(m)), m);
}

TEST(Matrix4Test, Determinant)
{
    EXPECT_FLOAT_EQ(determinant(Matrix4::Identity), 1.0f);
    // Diagonal: product of the diagonal entries.
    EXPECT_FLOAT_EQ(determinant(Matrix4{2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f}),
                    120.0f);
    // Upper-triangular: still the product of the diagonal.
    EXPECT_FLOAT_EQ(determinant(Matrix4{2.0f, 1.0f, 1.0f, 1.0f, 0.0f, 3.0f, 1.0f, 1.0f, 0.0f, 0.0f, 4.0f, 1.0f, 0.0f, 0.0f, 0.0f, 5.0f}),
                    120.0f);
}

TEST(Matrix4Test, Inverse)
{
    // Upper-triangular unimodular matrix: the inverse has exact integer entries.
    Matrix4 const m{1.0f, 2.0f, 3.0f, 0.0f, 0.0f, 1.0f, 4.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    expectMat4(inverse(m), Matrix4{1.0f, -2.0f, 5.0f, 0.0f, 0.0f, 1.0f, -4.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
}

TEST(Matrix4Test, InverseRoundTrip)
{
    // A full (no-zero) circulant matrix, well conditioned and invertible.
    Matrix4 const m{4.0f, 3.0f, 2.0f, 1.0f, 1.0f, 4.0f, 3.0f, 2.0f, 2.0f, 1.0f, 4.0f, 3.0f, 3.0f, 2.0f, 1.0f, 4.0f};
    EXPECT_TRUE(approxEqual(m * inverse(m), Matrix4::Identity));
    EXPECT_TRUE(approxEqual(inverse(m) * m, Matrix4::Identity));
}

TEST(Matrix4Test, InverseSafeSingular)
{
    expectMat4(inverseSafe(Matrix4::Zero), Matrix4::Zero);
}

TEST(Matrix4Test, Equality)
{
    EXPECT_TRUE(Matrix4{3.0f} == Matrix4{3.0f});
    EXPECT_TRUE(Matrix4{3.0f} != Matrix4{4.0f});
}

TEST(Matrix4Test, ApproxEqual)
{
    EXPECT_TRUE(approxEqual(Matrix4{1.0f}, Matrix4{1.0f}));
    EXPECT_FALSE(approxEqual(Matrix4{1.0f}, Matrix4{1.5f}));
}

// --- Matrix3 ---------------------------------------------------------------

TEST(Matrix3Test, ElementConstructor)
{
    Matrix3 const m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    EXPECT_FLOAT_EQ(m(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m(1, 1), 5.0f);
    EXPECT_FLOAT_EQ(m(2, 0), 7.0f);
}

TEST(Matrix3Test, Identity)
{
    expectMat3(Matrix3::Identity, Matrix3{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});
}

TEST(Matrix3Test, AddSubScale)
{
    expectMat3(Matrix3{1.0f} + Matrix3{2.0f}, Matrix3{3.0f});
    expectMat3(Matrix3{5.0f} - Matrix3{2.0f}, Matrix3{3.0f});
    expectMat3(Matrix3{3.0f} * 2.0f, Matrix3{6.0f});
    expectMat3(Matrix3{6.0f} / 2.0f, Matrix3{3.0f});
    expectMat3(-Matrix3{3.0f}, Matrix3{-3.0f});
}

TEST(Matrix3Test, MultiplyIdentity)
{
    Matrix3 const m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    expectMat3(m * Matrix3::Identity, m);
    expectMat3(Matrix3::Identity * m, m);
}

TEST(Matrix3Test, Multiply)
{
    Matrix3 const a{1.0f, 2.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    Matrix3 const b{1.0f, 0.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    expectMat3(a * b, Matrix3{7.0f, 2.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f});
}

TEST(Matrix3Test, VectorMultiply)
{
    Matrix3 const m{1.0f, 0.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    expectVec3(Vector3{1.0f, 1.0f, 0.0f} * m, 4.0f, 1.0f, 0.0f);
    expectVec3(Vector3{1.0f, 2.0f, 3.0f} * Matrix3::Identity, 1.0f, 2.0f, 3.0f);
}

TEST(Matrix3Test, Transpose)
{
    Matrix3 const m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    expectMat3(transpose(m), Matrix3{1.0f, 4.0f, 7.0f, 2.0f, 5.0f, 8.0f, 3.0f, 6.0f, 9.0f});
}

TEST(Matrix3Test, Determinant)
{
    EXPECT_FLOAT_EQ(determinant(Matrix3::Identity), 1.0f);
    EXPECT_FLOAT_EQ(determinant(Matrix3{2.0f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 4.0f}),
                    24.0f);
    // {1..9} has linearly dependent rows: determinant is zero.
    EXPECT_FLOAT_EQ(determinant(Matrix3{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f}),
                    0.0f);
}

TEST(Matrix3Test, Inverse)
{
    // Upper-triangular unimodular matrix: exact integer inverse.
    Matrix3 const m{1.0f, 2.0f, 3.0f, 0.0f, 1.0f, 4.0f, 0.0f, 0.0f, 1.0f};
    expectMat3(inverse(m), Matrix3{1.0f, -2.0f, 5.0f, 0.0f, 1.0f, -4.0f, 0.0f, 0.0f, 1.0f});
}

TEST(Matrix3Test, InverseRoundTrip)
{
    Matrix3 const m{2.0f, 1.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0.0f, 1.0f, 2.0f};
    EXPECT_TRUE(approxEqual(m * inverse(m), Matrix3::Identity));
}

TEST(Matrix3Test, InverseSafeSingular)
{
    expectMat3(inverseSafe(Matrix3{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f}),
               Matrix3::Zero);
}

TEST(Matrix3Test, Equality)
{
    EXPECT_TRUE(Matrix3{3.0f} == Matrix3{3.0f});
    EXPECT_TRUE(Matrix3{3.0f} != Matrix3{4.0f});
    EXPECT_TRUE(approxEqual(Matrix3{1.0f}, Matrix3{1.0f}));
}

// The padding column (slots 3, 7, 11) must stay exactly 0 so that matrix
// multiply and matrix-vector products do not leak garbage into real lanes.
TEST(Matrix3Test, DivisionPreservesPadding)
{
    Matrix3 m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    m /= 2.0f;
    EXPECT_FLOAT_EQ(m.mData[3], 0.0f);
    EXPECT_FLOAT_EQ(m.mData[7], 0.0f);
    EXPECT_FLOAT_EQ(m.mData[11], 0.0f);
}

TEST(Matrix3Test, MultiplyPreservesPadding)
{
    Matrix3 const a{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    Matrix3 const b{9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f};
    Matrix3 const c = a * b;
    EXPECT_FLOAT_EQ(c.mData[3], 0.0f);
    EXPECT_FLOAT_EQ(c.mData[7], 0.0f);
    EXPECT_FLOAT_EQ(c.mData[11], 0.0f);
}

// --- Matrix2 ---------------------------------------------------------------

TEST(Matrix2Test, ElementConstructor)
{
    Matrix2 const m{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_FLOAT_EQ(m(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(m(0, 1), 2.0f);
    EXPECT_FLOAT_EQ(m(1, 0), 3.0f);
    EXPECT_FLOAT_EQ(m(1, 1), 4.0f);
}

TEST(Matrix2Test, Identity)
{
    expectMat2(Matrix2::Identity, Matrix2{1.0f, 0.0f, 0.0f, 1.0f});
}

TEST(Matrix2Test, AddSubScale)
{
    expectMat2(Matrix2{1.0f} + Matrix2{2.0f}, Matrix2{3.0f});
    expectMat2(Matrix2{5.0f} - Matrix2{2.0f}, Matrix2{3.0f});
    expectMat2(Matrix2{3.0f} * 2.0f, Matrix2{6.0f});
    expectMat2(2.0f * Matrix2{3.0f}, Matrix2{6.0f});
    expectMat2(Matrix2{6.0f} / 2.0f, Matrix2{3.0f});
    expectMat2(-Matrix2{3.0f}, Matrix2{-3.0f});
}

TEST(Matrix2Test, CompoundAssign)
{
    Matrix2 a{4.0f};
    a += Matrix2{1.0f};
    expectMat2(a, Matrix2{5.0f});
    a -= Matrix2{2.0f};
    expectMat2(a, Matrix2{3.0f});
    a *= 2.0f;
    expectMat2(a, Matrix2{6.0f});
    a /= 3.0f;
    expectMat2(a, Matrix2{2.0f});
}

TEST(Matrix2Test, Multiply)
{
    Matrix2 const a{1.0f, 2.0f, 3.0f, 4.0f};
    Matrix2 const b{5.0f, 6.0f, 7.0f, 8.0f};
    expectMat2(a * b, Matrix2{19.0f, 22.0f, 43.0f, 50.0f});
    expectMat2(a * Matrix2::Identity, a);
}

TEST(Matrix2Test, VectorMultiply)
{
    Matrix2 const m{1.0f, 2.0f, 3.0f, 4.0f};
    expectVec2(Vector2{1.0f, 2.0f} * m, 7.0f, 10.0f);
    expectVec2(Vector2{1.0f, 2.0f} * Matrix2::Identity, 1.0f, 2.0f);
}

TEST(Matrix2Test, Transpose)
{
    expectMat2(transpose(Matrix2{1.0f, 2.0f, 3.0f, 4.0f}), Matrix2{1.0f, 3.0f, 2.0f, 4.0f});
}

TEST(Matrix2Test, Determinant)
{
    EXPECT_FLOAT_EQ(determinant(Matrix2::Identity), 1.0f);
    EXPECT_FLOAT_EQ(determinant(Matrix2{1.0f, 2.0f, 3.0f, 4.0f}), -2.0f);
}

TEST(Matrix2Test, Inverse)
{
    Matrix2 const m{1.0f, 2.0f, 3.0f, 4.0f};
    expectMat2(inverse(m), Matrix2{-2.0f, 1.0f, 1.5f, -0.5f});
    EXPECT_TRUE(approxEqual(m * inverse(m), Matrix2::Identity));
}

TEST(Matrix2Test, InverseSafeSingular)
{
    // Rows are linearly dependent: determinant is zero.
    expectMat2(inverseSafe(Matrix2{1.0f, 2.0f, 2.0f, 4.0f}), Matrix2::Zero);
}

TEST(Matrix2Test, Equality)
{
    EXPECT_TRUE(Matrix2{3.0f} == Matrix2{3.0f});
    EXPECT_TRUE(Matrix2{3.0f} != Matrix2{4.0f});
    EXPECT_TRUE(approxEqual(Matrix2{1.0f}, Matrix2{1.0f}));
}
