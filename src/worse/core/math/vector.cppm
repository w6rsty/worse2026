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

        Float4 toFloat4() const noexcept
        {
            return Float4{mData[0], mData[1], mData[2], mData[3]};
        }
    };

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

        WE_FORCEINLINE f32 lengthSquared() const noexcept
        {
            return simd::lengthSq3(reg());
        }

        WE_FORCEINLINE f32 length() const noexcept
        {
            return Sqrt(simd::lengthSq3(reg()));
        }

        Float3 toFloat3() const noexcept
        {
            return Float3{mData[0], mData[1], mData[2]};
        }
    };

    export class Vector2
    {
    private:
        f32 mX;
        f32 mY;

    public:
        Vector2() = default;

        WE_FORCEINLINE Vector2(f32 x, f32 y)
            : mX(x), mY(y)
        {
        }

        WE_FORCEINLINE explicit Vector2(f32 scalar)
            : mX(scalar), mY(scalar)
        {
        }

        WE_FORCEINLINE explicit Vector2(Float2 const& scalar)
            : mX(scalar.x), mY(scalar.y)
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

        WE_FORCEINLINE friend Vector2 operator+(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return Vector2(lhs.mX + rhs.mX, lhs.mY + rhs.mY);
        }

        WE_FORCEINLINE friend Vector2 operator-(Vector2 const& lhs, Vector2 const& rhs) noexcept
        {
            return Vector2(lhs.mX - rhs.mX, lhs.mY - rhs.mY);
        }

        WE_FORCEINLINE friend Vector2 operator*(Vector2 const& lhs, f32 scalar) noexcept
        {
            return Vector2(lhs.mX * scalar, lhs.mY * scalar);
        }

        WE_FORCEINLINE friend Vector2 operator*(f32 scalar, Vector2 const& rhs) noexcept
        {
            return rhs * scalar;
        }

        Float2 toFloat2() const noexcept
        {
            return Float2{mX, mY};
        }
    };

} // namespace worse::core::math