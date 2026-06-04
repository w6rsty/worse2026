module;

#include "worse/core/macro.hpp"

export module worse.core.math.matrix;
import worse.core.basic_type;
import worse.core.math;
import worse.core.math.simd;
import worse.core.math.vector;

namespace worse::core::math
{

    // CRTP base shared by the SIMD-backed matrices (Matrix3, Matrix4). Storage
    // is row-major: Rows logical rows, each padded to a 4-wide SIMD register.
    // Matrix3 leaves the trailing column as zero padding, mirroring the
    // Vector3 "w == 0" invariant that dot3/cross/length rely on.
    template <typename Derived, u32 Rows>
    struct alignas(16) SimdMatrix
    {
        f32 mData[Rows * 4];

        WE_FORCEINLINE simd::f32x4 row(u32 i) const noexcept
        {
            return simd::loadu(mData + i * 4);
        }

        WE_FORCEINLINE void setRow(u32 i, simd::f32x4 reg) noexcept
        {
            simd::storeu(mData + i * 4, reg);
        }

        WE_FORCEINLINE Derived& operator+=(Derived const& other) noexcept
        {
            for (u32 i = 0; i < Rows; ++i)
            {
                setRow(i, simd::add(row(i), other.row(i)));
            }
            return static_cast<Derived&>(*this);
        }

        WE_FORCEINLINE Derived& operator-=(Derived const& other) noexcept
        {
            for (u32 i = 0; i < Rows; ++i)
            {
                setRow(i, simd::sub(row(i), other.row(i)));
            }
            return static_cast<Derived&>(*this);
        }

        WE_FORCEINLINE Derived& operator*=(f32 scalar) noexcept
        {
            simd::f32x4 const s = simd::splat(scalar);
            for (u32 i = 0; i < Rows; ++i)
            {
                setRow(i, simd::mul(row(i), s));
            }
            return static_cast<Derived&>(*this);
        }

        // Scalar division is multiply-by-reciprocal: 0 * x stays 0, so Matrix3's
        // padding column is preserved for any finite divisor. A generic
        // simd::div would compute 0 / 0 == NaN there and a later matrix
        // multiply (NaN * 0) would leak the NaN into real lanes.
        WE_FORCEINLINE Derived& operator/=(f32 scalar) noexcept
        {
            return *this *= (1.0f / scalar);
        }

        WE_FORCEINLINE Derived operator-() const noexcept
        {
            Derived result;
            for (u32 i = 0; i < Rows; ++i)
            {
                result.setRow(i, simd::negate(row(i)));
            }
            return result;
        }

        WE_FORCEINLINE friend Derived operator+(Derived const& lhs, Derived const& rhs) noexcept
        {
            Derived result;
            for (u32 i = 0; i < Rows; ++i)
            {
                result.setRow(i, simd::add(lhs.row(i), rhs.row(i)));
            }
            return result;
        }

        WE_FORCEINLINE friend Derived operator-(Derived const& lhs, Derived const& rhs) noexcept
        {
            Derived result;
            for (u32 i = 0; i < Rows; ++i)
            {
                result.setRow(i, simd::sub(lhs.row(i), rhs.row(i)));
            }
            return result;
        }

        WE_FORCEINLINE friend Derived operator*(Derived const& lhs, f32 scalar) noexcept
        {
            Derived result;
            simd::f32x4 const s = simd::splat(scalar);
            for (u32 i = 0; i < Rows; ++i)
            {
                result.setRow(i, simd::mul(lhs.row(i), s));
            }
            return result;
        }

        WE_FORCEINLINE friend Derived operator*(f32 scalar, Derived const& rhs) noexcept
        {
            return rhs * scalar;
        }

        WE_FORCEINLINE friend Derived operator/(Derived const& lhs, f32 scalar) noexcept
        {
            return lhs * (1.0f / scalar);
        }

        // Exact element-wise equality; operator!= is synthesized by C++20.
        WE_FORCEINLINE friend bool operator==(Derived const& lhs, Derived const& rhs) noexcept
        {
            for (u32 i = 0; i < Rows * 4; ++i)
            {
                if (lhs.mData[i] != rhs.mData[i])
                {
                    return false;
                }
            }
            return true;
        }

        // True when every matching element pair is within EPSILON.
        WE_FORCEINLINE friend bool approxEqual(Derived const& lhs, Derived const& rhs) noexcept
        {
            for (u32 i = 0; i < Rows * 4; ++i)
            {
                if (abs(lhs.mData[i] - rhs.mData[i]) > EPSILON)
                {
                    return false;
                }
            }
            return true;
        }
    };

    // --- Matrix4 --------------------------------------------------------------

    export class Matrix4 : public SimdMatrix<Matrix4, 4>
    {
    public:
        Matrix4() = default;

        WE_FORCEINLINE Matrix4(f32 m00, f32 m01, f32 m02, f32 m03,
                               f32 m10, f32 m11, f32 m12, f32 m13,
                               f32 m20, f32 m21, f32 m22, f32 m23,
                               f32 m30, f32 m31, f32 m32, f32 m33) noexcept
            : SimdMatrix{{m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33}}
        {
        }

        // Builds a diagonal matrix; Matrix4{1.0f} is the identity.
        WE_FORCEINLINE explicit Matrix4(f32 diagonal) noexcept
            : SimdMatrix{{diagonal, 0.0f, 0.0f, 0.0f, 0.0f, diagonal, 0.0f, 0.0f, 0.0f, 0.0f, diagonal, 0.0f, 0.0f, 0.0f, 0.0f, diagonal}}
        {
        }

        // Named constants (defined just below the class).
        static Matrix4 const Zero, Identity;

        WE_FORCEINLINE f32& operator()(u32 r, u32 c) noexcept
        {
            WE_ASSERT(r < 4 && c < 4);
            return mData[r * 4 + c];
        }

        WE_FORCEINLINE f32 const& operator()(u32 r, u32 c) const noexcept
        {
            WE_ASSERT(r < 4 && c < 4);
            return mData[r * 4 + c];
        }

        // Matrix product. Row-major storage with the row-vector convention
        // (v * M), so (A * B).row(i) is the linear combination of B's rows
        // weighted by the components of A's row i.
        WE_FORCEINLINE friend Matrix4 operator*(Matrix4 const& lhs, Matrix4 const& rhs) noexcept
        {
            simd::f32x4 const r0 = rhs.row(0);
            simd::f32x4 const r1 = rhs.row(1);
            simd::f32x4 const r2 = rhs.row(2);
            simd::f32x4 const r3 = rhs.row(3);
            Matrix4 result;
            result.setRow(0, simd::mat4MulVec(r0, r1, r2, r3, lhs.row(0)));
            result.setRow(1, simd::mat4MulVec(r0, r1, r2, r3, lhs.row(1)));
            result.setRow(2, simd::mat4MulVec(r0, r1, r2, r3, lhs.row(2)));
            result.setRow(3, simd::mat4MulVec(r0, r1, r2, r3, lhs.row(3)));
            return result;
        }

        // Un-hide the inherited scalar *= shadowed by the matrix *= below.
        using SimdMatrix::operator*=;

        WE_FORCEINLINE Matrix4& operator*=(Matrix4 const& other) noexcept
        {
            *this = *this * other;
            return *this;
        }

        // Row-vector times matrix: v * M.
        WE_FORCEINLINE friend Vector4 operator*(Vector4 const& lhs, Matrix4 const& rhs) noexcept
        {
            Vector4 result;
            result.setReg(simd::mat4MulVec(rhs.row(0), rhs.row(1), rhs.row(2), rhs.row(3), lhs.reg()));
            return result;
        }

        WE_FORCEINLINE friend Matrix4 transpose(Matrix4 const& m) noexcept
        {
            simd::f32x4 r0 = m.row(0);
            simd::f32x4 r1 = m.row(1);
            simd::f32x4 r2 = m.row(2);
            simd::f32x4 r3 = m.row(3);
            simd::transpose4(r0, r1, r2, r3);
            Matrix4 result;
            result.setRow(0, r0);
            result.setRow(1, r1);
            result.setRow(2, r2);
            result.setRow(3, r3);
            return result;
        }

        WE_FORCEINLINE friend f32 determinant(Matrix4 const& m) noexcept
        {
            f32 const m00 = m.mData[0], m01 = m.mData[1], m02 = m.mData[2], m03 = m.mData[3];
            f32 const m10 = m.mData[4], m11 = m.mData[5], m12 = m.mData[6], m13 = m.mData[7];
            f32 const m20 = m.mData[8], m21 = m.mData[9], m22 = m.mData[10], m23 = m.mData[11];
            f32 const m30 = m.mData[12], m31 = m.mData[13], m32 = m.mData[14], m33 = m.mData[15];

            f32 const s0 = m20 * m31 - m21 * m30;
            f32 const s1 = m20 * m32 - m22 * m30;
            f32 const s2 = m20 * m33 - m23 * m30;
            f32 const s3 = m21 * m32 - m22 * m31;
            f32 const s4 = m21 * m33 - m23 * m31;
            f32 const s5 = m22 * m33 - m23 * m32;

            f32 const c0 = m00 * m11 - m01 * m10;
            f32 const c1 = m00 * m12 - m02 * m10;
            f32 const c2 = m00 * m13 - m03 * m10;
            f32 const c3 = m01 * m12 - m02 * m11;
            f32 const c4 = m01 * m13 - m03 * m11;
            f32 const c5 = m02 * m13 - m03 * m12;

            return c0 * s5 - c1 * s4 + c2 * s3 + c3 * s2 - c4 * s1 + c5 * s0;
        }

        // Unsafe inverse: divides by the determinant unconditionally, so a
        // singular matrix yields inf/NaN. Use inverseSafe() when invertibility
        // is not guaranteed.
        WE_FORCEINLINE friend Matrix4 inverse(Matrix4 const& m) noexcept
        {
            f32 const m00 = m.mData[0], m01 = m.mData[1], m02 = m.mData[2], m03 = m.mData[3];
            f32 const m10 = m.mData[4], m11 = m.mData[5], m12 = m.mData[6], m13 = m.mData[7];
            f32 const m20 = m.mData[8], m21 = m.mData[9], m22 = m.mData[10], m23 = m.mData[11];
            f32 const m30 = m.mData[12], m31 = m.mData[13], m32 = m.mData[14], m33 = m.mData[15];

            f32 const s0 = m20 * m31 - m21 * m30;
            f32 const s1 = m20 * m32 - m22 * m30;
            f32 const s2 = m20 * m33 - m23 * m30;
            f32 const s3 = m21 * m32 - m22 * m31;
            f32 const s4 = m21 * m33 - m23 * m31;
            f32 const s5 = m22 * m33 - m23 * m32;

            f32 const c0 = m00 * m11 - m01 * m10;
            f32 const c1 = m00 * m12 - m02 * m10;
            f32 const c2 = m00 * m13 - m03 * m10;
            f32 const c3 = m01 * m12 - m02 * m11;
            f32 const c4 = m01 * m13 - m03 * m11;
            f32 const c5 = m02 * m13 - m03 * m12;

            f32 const det = c0 * s5 - c1 * s4 + c2 * s3 + c3 * s2 - c4 * s1 + c5 * s0;
            f32 const t   = 1.0f / det;

            Matrix4 result;
            result.mData[0] = (m11 * s5 - m12 * s4 + m13 * s3) * t;
            result.mData[1] = (-m01 * s5 + m02 * s4 - m03 * s3) * t;
            result.mData[2] = (m31 * c5 - m32 * c4 + m33 * c3) * t;
            result.mData[3] = (-m21 * c5 + m22 * c4 - m23 * c3) * t;

            result.mData[4] = (-m10 * s5 + m12 * s2 - m13 * s1) * t;
            result.mData[5] = (m00 * s5 - m02 * s2 + m03 * s1) * t;
            result.mData[6] = (-m30 * c5 + m32 * c2 - m33 * c1) * t;
            result.mData[7] = (m20 * c5 - m22 * c2 + m23 * c1) * t;

            result.mData[8]  = (m10 * s4 - m11 * s2 + m13 * s0) * t;
            result.mData[9]  = (-m00 * s4 + m01 * s2 - m03 * s0) * t;
            result.mData[10] = (m30 * c4 - m31 * c2 + m33 * c0) * t;
            result.mData[11] = (-m20 * c4 + m21 * c2 - m23 * c0) * t;

            result.mData[12] = (-m10 * s3 + m11 * s1 - m12 * s0) * t;
            result.mData[13] = (m00 * s3 - m01 * s1 + m02 * s0) * t;
            result.mData[14] = (-m30 * c3 + m31 * c1 - m32 * c0) * t;
            result.mData[15] = (m20 * c3 - m21 * c1 + m22 * c0) * t;
            return result;
        }

        // Returns the zero matrix when the matrix is singular (|det| <= EPSILON).
        WE_FORCEINLINE friend Matrix4 inverseSafe(Matrix4 const& m) noexcept
        {
            if (abs(determinant(m)) <= EPSILON)
            {
                return Matrix4::Zero;
            }
            return inverse(m);
        }
    };

    inline Matrix4 const Matrix4::Zero     = Matrix4{0.0f};
    inline Matrix4 const Matrix4::Identity = Matrix4{1.0f};

    // --- Matrix3 --------------------------------------------------------------

    export class Matrix3 : public SimdMatrix<Matrix3, 3>
    {
    public:
        Matrix3() = default;

        WE_FORCEINLINE Matrix3(f32 m00, f32 m01, f32 m02,
                               f32 m10, f32 m11, f32 m12,
                               f32 m20, f32 m21, f32 m22) noexcept
            : SimdMatrix{{m00, m01, m02, 0.0f, m10, m11, m12, 0.0f, m20, m21, m22, 0.0f}}
        {
        }

        // Builds a diagonal matrix; Matrix3{1.0f} is the identity.
        WE_FORCEINLINE explicit Matrix3(f32 diagonal) noexcept
            : SimdMatrix{{diagonal, 0.0f, 0.0f, 0.0f, 0.0f, diagonal, 0.0f, 0.0f, 0.0f, 0.0f, diagonal, 0.0f}}
        {
        }

        // Named constants (defined just below the class).
        static Matrix3 const Zero, Identity;

        WE_FORCEINLINE f32& operator()(u32 r, u32 c) noexcept
        {
            WE_ASSERT(r < 3 && c < 3);
            return mData[r * 4 + c];
        }

        WE_FORCEINLINE f32 const& operator()(u32 r, u32 c) const noexcept
        {
            WE_ASSERT(r < 3 && c < 3);
            return mData[r * 4 + c];
        }

        // Matrix product. The zero() fourth row keeps the padding column 0:
        // every contributing row already has a 0 there, so the result does too.
        WE_FORCEINLINE friend Matrix3 operator*(Matrix3 const& lhs, Matrix3 const& rhs) noexcept
        {
            simd::f32x4 const r0 = rhs.row(0);
            simd::f32x4 const r1 = rhs.row(1);
            simd::f32x4 const r2 = rhs.row(2);
            simd::f32x4 const z  = simd::zero();
            Matrix3 result;
            result.setRow(0, simd::mat4MulVec(r0, r1, r2, z, lhs.row(0)));
            result.setRow(1, simd::mat4MulVec(r0, r1, r2, z, lhs.row(1)));
            result.setRow(2, simd::mat4MulVec(r0, r1, r2, z, lhs.row(2)));
            return result;
        }

        // Un-hide the inherited scalar *= shadowed by the matrix *= below.
        using SimdMatrix::operator*=;

        WE_FORCEINLINE Matrix3& operator*=(Matrix3 const& other) noexcept
        {
            *this = *this * other;
            return *this;
        }

        // Row-vector times matrix: v * M.
        WE_FORCEINLINE friend Vector3 operator*(Vector3 const& lhs, Matrix3 const& rhs) noexcept
        {
            Vector3 result;
            result.setReg(simd::mat4MulVec(rhs.row(0), rhs.row(1), rhs.row(2), simd::zero(), lhs.reg()));
            result.mData[3] = 0.0f;
            return result;
        }

        WE_FORCEINLINE friend Matrix3 transpose(Matrix3 const& m) noexcept
        {
            return Matrix3{m.mData[0], m.mData[4], m.mData[8], m.mData[1], m.mData[5], m.mData[9], m.mData[2], m.mData[6], m.mData[10]};
        }

        WE_FORCEINLINE friend f32 determinant(Matrix3 const& m) noexcept
        {
            f32 const a = m.mData[0], b = m.mData[1], c = m.mData[2];
            f32 const d = m.mData[4], e = m.mData[5], f = m.mData[6];
            f32 const g = m.mData[8], h = m.mData[9], i = m.mData[10];
            return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
        }

        // Unsafe inverse: divides by the determinant unconditionally, so a
        // singular matrix yields inf/NaN. Use inverseSafe() when invertibility
        // is not guaranteed.
        WE_FORCEINLINE friend Matrix3 inverse(Matrix3 const& m) noexcept
        {
            f32 const a = m.mData[0], b = m.mData[1], c = m.mData[2];
            f32 const d = m.mData[4], e = m.mData[5], f = m.mData[6];
            f32 const g = m.mData[8], h = m.mData[9], i = m.mData[10];

            f32 const A = (e * i - f * h);
            f32 const B = -(d * i - f * g);
            f32 const C = (d * h - e * g);
            f32 const D = -(b * i - c * h);
            f32 const E = (a * i - c * g);
            f32 const F = -(a * h - b * g);
            f32 const G = (b * f - c * e);
            f32 const H = -(a * f - c * d);
            f32 const I = (a * e - b * d);

            f32 const det = a * A + b * B + c * C;
            f32 const s   = 1.0f / det;

            // inverse = adjugate / det; the adjugate transposes the cofactors.
            return Matrix3{A * s, D * s, G * s, B * s, E * s, H * s, C * s, F * s, I * s};
        }

        // Returns the zero matrix when the matrix is singular (|det| <= EPSILON).
        WE_FORCEINLINE friend Matrix3 inverseSafe(Matrix3 const& m) noexcept
        {
            if (abs(determinant(m)) <= EPSILON)
            {
                return Matrix3::Zero;
            }
            return inverse(m);
        }
    };

    inline Matrix3 const Matrix3::Zero     = Matrix3{0.0f};
    inline Matrix3 const Matrix3::Identity = Matrix3{1.0f};

    // --- Matrix2 --------------------------------------------------------------

    export class Matrix2
    {
    private:
        f32 mData[4];

    public:
        Matrix2() = default;

        WE_FORCEINLINE Matrix2(f32 m00, f32 m01, f32 m10, f32 m11) noexcept
            : mData{m00, m01, m10, m11}
        {
        }

        // Builds a diagonal matrix; Matrix2{1.0f} is the identity.
        WE_FORCEINLINE explicit Matrix2(f32 diagonal) noexcept
            : mData{diagonal, 0.0f, 0.0f, diagonal}
        {
        }

        // Named constants (defined just below the class).
        static Matrix2 const Zero, Identity;

        WE_FORCEINLINE f32& operator()(u32 r, u32 c) noexcept
        {
            WE_ASSERT(r < 2 && c < 2);
            return mData[r * 2 + c];
        }

        WE_FORCEINLINE f32 const& operator()(u32 r, u32 c) const noexcept
        {
            WE_ASSERT(r < 2 && c < 2);
            return mData[r * 2 + c];
        }

        WE_FORCEINLINE Matrix2& operator+=(Matrix2 const& other) noexcept
        {
            mData[0] += other.mData[0];
            mData[1] += other.mData[1];
            mData[2] += other.mData[2];
            mData[3] += other.mData[3];
            return *this;
        }

        WE_FORCEINLINE Matrix2& operator-=(Matrix2 const& other) noexcept
        {
            mData[0] -= other.mData[0];
            mData[1] -= other.mData[1];
            mData[2] -= other.mData[2];
            mData[3] -= other.mData[3];
            return *this;
        }

        WE_FORCEINLINE Matrix2& operator*=(f32 scalar) noexcept
        {
            mData[0] *= scalar;
            mData[1] *= scalar;
            mData[2] *= scalar;
            mData[3] *= scalar;
            return *this;
        }

        WE_FORCEINLINE Matrix2& operator/=(f32 scalar) noexcept
        {
            mData[0] /= scalar;
            mData[1] /= scalar;
            mData[2] /= scalar;
            mData[3] /= scalar;
            return *this;
        }

        WE_FORCEINLINE Matrix2 operator-() const noexcept
        {
            return Matrix2{-mData[0], -mData[1], -mData[2], -mData[3]};
        }

        WE_FORCEINLINE friend Matrix2 operator+(Matrix2 const& lhs, Matrix2 const& rhs) noexcept
        {
            return Matrix2{lhs.mData[0] + rhs.mData[0], lhs.mData[1] + rhs.mData[1], lhs.mData[2] + rhs.mData[2], lhs.mData[3] + rhs.mData[3]};
        }

        WE_FORCEINLINE friend Matrix2 operator-(Matrix2 const& lhs, Matrix2 const& rhs) noexcept
        {
            return Matrix2{lhs.mData[0] - rhs.mData[0], lhs.mData[1] - rhs.mData[1], lhs.mData[2] - rhs.mData[2], lhs.mData[3] - rhs.mData[3]};
        }

        WE_FORCEINLINE friend Matrix2 operator*(Matrix2 const& lhs, f32 scalar) noexcept
        {
            return Matrix2{lhs.mData[0] * scalar, lhs.mData[1] * scalar, lhs.mData[2] * scalar, lhs.mData[3] * scalar};
        }

        WE_FORCEINLINE friend Matrix2 operator*(f32 scalar, Matrix2 const& rhs) noexcept
        {
            return rhs * scalar;
        }

        WE_FORCEINLINE friend Matrix2 operator/(Matrix2 const& lhs, f32 scalar) noexcept
        {
            return Matrix2{lhs.mData[0] / scalar, lhs.mData[1] / scalar, lhs.mData[2] / scalar, lhs.mData[3] / scalar};
        }

        // Matrix product, row-major with the row-vector convention (v * M).
        WE_FORCEINLINE friend Matrix2 operator*(Matrix2 const& lhs, Matrix2 const& rhs) noexcept
        {
            return Matrix2{lhs.mData[0] * rhs.mData[0] + lhs.mData[1] * rhs.mData[2],
                           lhs.mData[0] * rhs.mData[1] + lhs.mData[1] * rhs.mData[3],
                           lhs.mData[2] * rhs.mData[0] + lhs.mData[3] * rhs.mData[2],
                           lhs.mData[2] * rhs.mData[1] + lhs.mData[3] * rhs.mData[3]};
        }

        WE_FORCEINLINE Matrix2& operator*=(Matrix2 const& other) noexcept
        {
            *this = *this * other;
            return *this;
        }

        // Row-vector times matrix: v * M.
        WE_FORCEINLINE friend Vector2 operator*(Vector2 const& lhs, Matrix2 const& rhs) noexcept
        {
            return Vector2{lhs.x() * rhs.mData[0] + lhs.y() * rhs.mData[2],
                           lhs.x() * rhs.mData[1] + lhs.y() * rhs.mData[3]};
        }

        WE_FORCEINLINE friend Matrix2 transpose(Matrix2 const& m) noexcept
        {
            return Matrix2{m.mData[0], m.mData[2], m.mData[1], m.mData[3]};
        }

        // Exact element-wise equality; operator!= is synthesized by C++20.
        WE_FORCEINLINE friend bool operator==(Matrix2 const& lhs, Matrix2 const& rhs) noexcept
        {
            return lhs.mData[0] == rhs.mData[0] && lhs.mData[1] == rhs.mData[1] &&
                   lhs.mData[2] == rhs.mData[2] && lhs.mData[3] == rhs.mData[3];
        }

        // True when every matching element pair is within EPSILON.
        WE_FORCEINLINE friend bool approxEqual(Matrix2 const& lhs, Matrix2 const& rhs) noexcept
        {
            return abs(lhs.mData[0] - rhs.mData[0]) <= EPSILON &&
                   abs(lhs.mData[1] - rhs.mData[1]) <= EPSILON &&
                   abs(lhs.mData[2] - rhs.mData[2]) <= EPSILON &&
                   abs(lhs.mData[3] - rhs.mData[3]) <= EPSILON;
        }

        WE_FORCEINLINE friend f32 determinant(Matrix2 const& m) noexcept
        {
            return m.mData[0] * m.mData[3] - m.mData[1] * m.mData[2];
        }

        // Unsafe inverse: divides by the determinant unconditionally, so a
        // singular matrix yields inf/NaN. Use inverseSafe() when invertibility
        // is not guaranteed.
        WE_FORCEINLINE friend Matrix2 inverse(Matrix2 const& m) noexcept
        {
            f32 const s = 1.0f / determinant(m);
            return Matrix2{m.mData[3] * s, -m.mData[1] * s, -m.mData[2] * s, m.mData[0] * s};
        }

        // Returns the zero matrix when the matrix is singular (|det| <= EPSILON).
        WE_FORCEINLINE friend Matrix2 inverseSafe(Matrix2 const& m) noexcept
        {
            if (abs(determinant(m)) <= EPSILON)
            {
                return Matrix2::Zero;
            }
            return inverse(m);
        }
    };

    inline Matrix2 const Matrix2::Zero     = Matrix2{0.0f};
    inline Matrix2 const Matrix2::Identity = Matrix2{1.0f};

} // namespace worse::core::math
