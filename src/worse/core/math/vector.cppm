module;

#include "worse/core/macros.hpp"

export module worse.core.math.vector;
import worse.core.basic_types;
import worse.core.platform;
import worse.core.math;
import worse.core.math.simd;

namespace worse::core::math
{

    template <typename Derived>
    struct alignas(16) SimdVector
    {
        f32 mData[4];

        WE_FORCEINLINE simd::f32x4 reg() const noexcept
        {
            return simd::loadu(mData);
        }

        WE_FORCEINLINE void setReg(simd::f32x4 reg) noexcept
        {
            simd::storeu(mData, reg);
        }

        WE_FORCEINLINE Derived& operator+=(Derived const& other) noexcept
        {
            setReg(simd::add(reg(), other.reg()));
            return static_cast<Derived&>(*this);
        }

        WE_FORCEINLINE Derived& operator-=(Derived const& other) noexcept
        {
            setReg(simd::sub(reg(), other.reg()));
            return static_cast<Derived&>(*this);
        }

        WE_FORCEINLINE Derived& operator*=(f32 scalar) noexcept
        {
            setReg(simd::mul(reg(), simd::splat(scalar)));
            return static_cast<Derived&>(*this);
        }

        WE_FORCEINLINE Derived& operator/=(f32 scalar) noexcept
        {
            setReg(simd::div(reg(), simd::splat(scalar)));
            return static_cast<Derived&>(*this);
        }

        WE_FORCEINLINE Derived operator-() const noexcept
        {
            Derived result;
            result.setReg(simd::negate(reg()));
            return result;
        }

        WE_FORCEINLINE friend Derived operator+(Derived const& lhs, Derived const& rhs) noexcept
        {
            Derived result;
            result.setReg(simd::add(lhs.reg(), rhs.reg()));
            return result;
        }

        WE_FORCEINLINE friend Derived operator-(Derived const& lhs, Derived const& rhs) noexcept
        {
            Derived result;
            result.setReg(simd::sub(lhs.reg(), rhs.reg()));
            return result;
        }

        // Component-wise (Hadamard) product, used for color modulation and
        // non-uniform scaling.
        WE_FORCEINLINE friend Derived operator*(Derived const& lhs, Derived const& rhs) noexcept
        {
            Derived result;
            result.setReg(simd::mul(lhs.reg(), rhs.reg()));
            return result;
        }

        WE_FORCEINLINE friend Derived operator*(Derived const& lhs, f32 scalar) noexcept
        {
            Derived result;
            result.setReg(simd::mul(lhs.reg(), simd::splat(scalar)));
            return result;
        }

        WE_FORCEINLINE friend Derived operator*(f32 scalar, Derived const& rhs) noexcept
        {
            return rhs * scalar;
        }

        WE_FORCEINLINE friend Derived operator/(Derived const& lhs, f32 scalar) noexcept
        {
            Derived result;
            result.setReg(simd::div(lhs.reg(), simd::splat(scalar)));
            return result;
        }
    };

    export class Vector4 : public SimdVector<Vector4>
    {
    public:
        Vector4() = default;

        WE_FORCEINLINE Vector4(f32 x, f32 y, f32 z, f32 w) noexcept
            : SimdVector{{x, y, z, w}}
        {
        }

        WE_FORCEINLINE explicit Vector4(f32 scalar) noexcept
            : SimdVector{{scalar, scalar, scalar, scalar}}
        {
        }

        WE_FORCEINLINE explicit Vector4(Float4 const& scalar)
            : SimdVector{{scalar.x, scalar.y, scalar.z, scalar.w}}
        {
        }

        WE_FORCEINLINE f32& x() noexcept { return mData[0]; }
        WE_FORCEINLINE f32& y() noexcept { return mData[1]; }
        WE_FORCEINLINE f32& z() noexcept { return mData[2]; }
        WE_FORCEINLINE f32& w() noexcept { return mData[3]; }
        WE_FORCEINLINE f32 const& x() const noexcept { return mData[0]; }
        WE_FORCEINLINE f32 const& y() const noexcept { return mData[1]; }
        WE_FORCEINLINE f32 const& z() const noexcept { return mData[2]; }
        WE_FORCEINLINE f32 const& w() const noexcept { return mData[3]; }

        // Component-wise division. Every lane carries real data, so the generic
        // SIMD divide is safe here.
        WE_FORCEINLINE friend Vector4 operator/(Vector4 const& lhs, Vector4 const& rhs) noexcept
        {
            Vector4 result;
            result.setReg(simd::div(lhs.reg(), rhs.reg()));
            return result;
        }

        WE_FORCEINLINE friend f32 dot(Vector4 const& lhs, Vector4 const& rhs) noexcept
        {
            return simd::dot4(lhs.reg(), rhs.reg());
        }

        WE_FORCEINLINE friend f32 lengthSquared(Vector4 const& v) noexcept
        {
            return simd::lengthSq4(v.reg());
        }

        WE_FORCEINLINE friend f32 length(Vector4 const& v) noexcept
        {
            return squareRoot(simd::lengthSq4(v.reg()));
        }

        // Unsafe normalize: produces NaN/inf for a zero vector. Use
        // normalizeSafe() when the input length is not guaranteed.
        WE_FORCEINLINE friend Vector4 normalize(Vector4 const& v) noexcept
        {
            simd::f32x4 const r = v.reg();
            Vector4 result;
            result.setReg(simd::mul(r, simd::rsqrt(simd::splat(simd::lengthSq4(r)))));
            return result;
        }

        WE_FORCEINLINE friend Vector4 normalizeSafe(Vector4 const& v) noexcept
        {
            f32 const lenSq = lengthSquared(v);
            if (lenSq <= EPSILON * EPSILON)
            {
                return Vector4{0.0f};
            }
            Vector4 result;
            result.setReg(simd::mul(v.reg(), simd::rsqrt(simd::splat(lenSq))));
            return result;
        }

        WE_FORCEINLINE friend f32 distanceSquared(Vector4 const& lhs, Vector4 const& rhs) noexcept
        {
            return lengthSquared(lhs - rhs);
        }

        WE_FORCEINLINE friend f32 distance(Vector4 const& lhs, Vector4 const& rhs) noexcept
        {
            return length(lhs - rhs);
        }

        Float4 toFloat4() const noexcept
        {
            return Float4{mData[0], mData[1], mData[2], mData[3]};
        }
    };

    Vector4 toVector4(Float4 const& scalar) noexcept
    {
        return Vector4{scalar.x, scalar.y, scalar.z, scalar.w};
    }

    export class Vector3 : public SimdVector<Vector3>
    {
    public:
        Vector3() = default;

        WE_FORCEINLINE Vector3(f32 x, f32 y, f32 z) noexcept
            : SimdVector{{x, y, z, 0.0f}}
        {
        }

        WE_FORCEINLINE explicit Vector3(f32 scalar) noexcept
            : SimdVector{{scalar, scalar, scalar, 0.0f}}
        {
        }

        WE_FORCEINLINE explicit Vector3(Float3 const& scalar)
            : SimdVector{{scalar.x, scalar.y, scalar.z, 0.0f}}
        {
        }

        WE_FORCEINLINE f32& x() noexcept { return mData[0]; }
        WE_FORCEINLINE f32& y() noexcept { return mData[1]; }
        WE_FORCEINLINE f32& z() noexcept { return mData[2]; }
        WE_FORCEINLINE f32 const& x() const noexcept { return mData[0]; }
        WE_FORCEINLINE f32 const& y() const noexcept { return mData[1]; }
        WE_FORCEINLINE f32 const& z() const noexcept { return mData[2]; }

        // Component-wise division. The unused w lane holds 0 for both operands,
        // so the generic divide would yield 0/0 == NaN there; reset it to keep
        // the "w == 0" invariant that dot3/cross/length rely on.
        WE_FORCEINLINE friend Vector3 operator/(Vector3 const& lhs, Vector3 const& rhs) noexcept
        {
            Vector3 result;
            result.setReg(simd::div(lhs.reg(), rhs.reg()));
            result.mData[3] = 0.0f;
            return result;
        }

        WE_FORCEINLINE friend f32 dot(Vector3 const& lhs, Vector3 const& rhs) noexcept
        {
            return simd::dot3(lhs.reg(), rhs.reg());
        }

        WE_FORCEINLINE friend Vector3 cross(Vector3 const& lhs, Vector3 const& rhs) noexcept
        {
            Vector3 result;
            result.setReg(simd::cross3(lhs.reg(), rhs.reg()));
            return result;
        }

        WE_FORCEINLINE friend f32 lengthSquared(Vector3 const& v) noexcept
        {
            return simd::lengthSq3(v.reg());
        }

        WE_FORCEINLINE friend f32 length(Vector3 const& v) noexcept
        {
            return squareRoot(simd::lengthSq3(v.reg()));
        }

        // Unsafe normalize: produces NaN/inf for a zero vector. Use
        // normalizeSafe() when the input length is not guaranteed.
        WE_FORCEINLINE friend Vector3 normalize(Vector3 const& v) noexcept
        {
            simd::f32x4 const r = v.reg();
            Vector3 result;
            result.setReg(simd::mul(r, simd::rsqrt(simd::splat(simd::lengthSq3(r)))));
            return result;
        }

        WE_FORCEINLINE friend Vector3 normalizeSafe(Vector3 const& v) noexcept
        {
            f32 const lenSq = lengthSquared(v);
            if (lenSq <= EPSILON * EPSILON)
            {
                return Vector3{0.0f};
            }
            Vector3 result;
            result.setReg(simd::mul(v.reg(), simd::rsqrt(simd::splat(lenSq))));
            return result;
        }

        WE_FORCEINLINE friend f32 distanceSquared(Vector3 const& lhs, Vector3 const& rhs) noexcept
        {
            return lengthSquared(lhs - rhs);
        }

        WE_FORCEINLINE friend f32 distance(Vector3 const& lhs, Vector3 const& rhs) noexcept
        {
            return length(lhs - rhs);
        }

        Float3 toFloat3() const noexcept
        {
            return Float3{mData[0], mData[1], mData[2]};
        }
    };

    Vector3 toVector3(Float3 const& scalar) noexcept
    {
        return Vector3{scalar.x, scalar.y, scalar.z};
    }

    export class Vector2
    {
    private:
        f32 mX;
        f32 mY;

    public:
        Vector2() = default;

        WE_FORCEINLINE Vector2(f32 x, f32 y)
            : mX{x}, mY{y}
        {
        }

        WE_FORCEINLINE explicit Vector2(f32 scalar)
            : mX{scalar}, mY{scalar}
        {
        }

        WE_FORCEINLINE explicit Vector2(Float2 const& scalar)
            : mX{scalar.x}, mY{scalar.y}
        {
        }

        WE_FORCEINLINE f32& x() noexcept { return mX; }
        WE_FORCEINLINE f32& y() noexcept { return mY; }
        WE_FORCEINLINE f32 const& x() const noexcept { return mX; }
        WE_FORCEINLINE f32 const& y() const noexcept { return mY; }

        WE_FORCEINLINE Vector2& operator+=(Vector2 const& other) noexcept
        {
            mX += other.mX;
            mY += other.mY;
            return *this;
        }

        WE_FORCEINLINE Vector2& operator-=(Vector2 const& other) noexcept
        {
            mX -= other.mX;
            mY -= other.mY;
            return *this;
        }

        WE_FORCEINLINE Vector2& operator*=(f32 scalar) noexcept
        {
            mX *= scalar;
            mY *= scalar;
            return *this;
        }

        WE_FORCEINLINE Vector2& operator/=(f32 scalar) noexcept
        {
            mX /= scalar;
            mY /= scalar;
            return *this;
        }

        WE_FORCEINLINE Vector2 operator-() const noexcept
        {
            return Vector2{-mX, -mY};
        }

        WE_FORCEINLINE friend Vector2 operator+(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return Vector2{lhs.mX + rhs.mX, lhs.mY + rhs.mY};
        }

        WE_FORCEINLINE friend Vector2 operator-(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return Vector2{lhs.mX - rhs.mX, lhs.mY - rhs.mY};
        }

        // Component-wise (Hadamard) product.
        WE_FORCEINLINE friend Vector2 operator*(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return Vector2{lhs.mX * rhs.mX, lhs.mY * rhs.mY};
        }

        WE_FORCEINLINE friend Vector2 operator*(Vector2 const& lhs, f32 scalar) noexcept
        {
            return Vector2{lhs.mX * scalar, lhs.mY * scalar};
        }

        WE_FORCEINLINE friend Vector2 operator*(f32 scalar, Vector2 const& rhs) noexcept
        {
            return rhs * scalar;
        }

        WE_FORCEINLINE friend Vector2 operator/(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return Vector2{lhs.mX / rhs.mX, lhs.mY / rhs.mY};
        }

        WE_FORCEINLINE friend Vector2 operator/(Vector2 const& lhs, f32 scalar) noexcept
        {
            return Vector2{lhs.mX / scalar, lhs.mY / scalar};
        }

        WE_FORCEINLINE friend f32 dot(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return lhs.mX * rhs.mX + lhs.mY * rhs.mY;
        }

        WE_FORCEINLINE friend f32 lengthSquared(Vector2 const& v) noexcept
        {
            return v.mX * v.mX + v.mY * v.mY;
        }

        WE_FORCEINLINE friend f32 length(Vector2 const& v) noexcept
        {
            return squareRoot(lengthSquared(v));
        }

        // Unsafe normalize: produces NaN/inf for a zero vector. Use
        // normalizeSafe() when the input length is not guaranteed.
        WE_FORCEINLINE friend Vector2 normalize(Vector2 const& v) noexcept
        {
            f32 const invLen = 1.0f / length(v);
            return Vector2{v.mX * invLen, v.mY * invLen};
        }

        WE_FORCEINLINE friend Vector2 normalizeSafe(Vector2 const& v) noexcept
        {
            f32 const lenSq = lengthSquared(v);
            if (lenSq <= EPSILON * EPSILON)
            {
                return Vector2{0.0f};
            }
            f32 const invLen = 1.0f / squareRoot(lenSq);
            return Vector2{v.mX * invLen, v.mY * invLen};
        }

        WE_FORCEINLINE friend f32 distanceSquared(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return lengthSquared(lhs - rhs);
        }

        WE_FORCEINLINE friend f32 distance(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return length(lhs - rhs);
        }

        Float2 toFloat2() const noexcept
        {
            return Float2{mX, mY};
        }
    };

    Vector2 toVector2(Float2 const& scalar) noexcept
    {
        return Vector2{scalar.x, scalar.y};
    }

} // namespace worse::core::math
