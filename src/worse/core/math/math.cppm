module;

#include "worse/core/macros.hpp"

#include <cmath>

export module worse.core.math;
import worse.core.basic_types;

export namespace worse::core::math
{

    struct Float2
    {
        f32 x, y;
    };

    struct Float3
    {
        f32 x, y, z;
    };

    struct Float4
    {
        f32 x, y, z, w;
    };

    // Tolerance used for near-zero comparisons (e.g. zero-length detection).
    constexpr f32 EPSILON = 1e-6f;

    // Angle constants.
    constexpr f32 PI         = 3.14159265358979323846f;
    constexpr f32 TWO_PI     = 2.0f * PI;
    constexpr f32 HALF_PI    = 0.5f * PI;
    constexpr f32 INV_PI     = 1.0f / PI;
    constexpr f32 DEG_TO_RAD = PI / 180.0f;
    constexpr f32 RAD_TO_DEG = 180.0f / PI;

    WE_FORCEINLINE f32 squareRoot(f32 value) noexcept
    {
        return std::sqrtf(value);
    }

    WE_FORCEINLINE constexpr f32 min(f32 lhs, f32 rhs) noexcept
    {
        return lhs < rhs ? lhs : rhs;
    }

    WE_FORCEINLINE constexpr f32 max(f32 lhs, f32 rhs) noexcept
    {
        return lhs > rhs ? lhs : rhs;
    }

    WE_FORCEINLINE constexpr f32 abs(f32 value) noexcept
    {
        return value < 0.0f ? -value : value;
    }

    WE_FORCEINLINE constexpr f32 clamp(f32 value, f32 lo, f32 hi) noexcept
    {
        return min(max(value, lo), hi);
    }

    WE_FORCEINLINE constexpr f32 saturate(f32 value) noexcept
    {
        return clamp(value, 0.0f, 1.0f);
    }

    // Unclamped linear interpolation (HLSL semantics).
    WE_FORCEINLINE constexpr f32 lerp(f32 a, f32 b, f32 t) noexcept
    {
        return a + (b - a) * t;
    }

    // --- Angle conversion -------------------------------------------------

    WE_FORCEINLINE constexpr f32 radians(f32 deg) noexcept
    {
        return deg * DEG_TO_RAD;
    }

    WE_FORCEINLINE constexpr f32 degrees(f32 rad) noexcept
    {
        return rad * RAD_TO_DEG;
    }

    // --- Trigonometry -----------------------------------------------------

    WE_FORCEINLINE f32 sin(f32 x) noexcept
    {
        return std::sinf(x);
    }

    WE_FORCEINLINE f32 cos(f32 x) noexcept
    {
        return std::cosf(x);
    }

    WE_FORCEINLINE f32 tan(f32 x) noexcept
    {
        return std::tanf(x);
    }

    WE_FORCEINLINE f32 asin(f32 x) noexcept
    {
        return std::asinf(x);
    }

    WE_FORCEINLINE f32 acos(f32 x) noexcept
    {
        return std::acosf(x);
    }

    WE_FORCEINLINE f32 atan(f32 x) noexcept
    {
        return std::atanf(x);
    }

    WE_FORCEINLINE f32 atan2(f32 y, f32 x) noexcept
    {
        return std::atan2f(y, x);
    }

    // --- Exponential ------------------------------------------------------

    WE_FORCEINLINE f32 pow(f32 base, f32 exponent) noexcept
    {
        return std::powf(base, exponent);
    }

    WE_FORCEINLINE f32 exp(f32 x) noexcept
    {
        return std::expf(x);
    }

    WE_FORCEINLINE f32 log(f32 x) noexcept
    {
        return std::logf(x);
    }

    WE_FORCEINLINE f32 log2(f32 x) noexcept
    {
        return std::log2f(x);
    }

    WE_FORCEINLINE f32 exp2(f32 x) noexcept
    {
        return std::exp2f(x);
    }

    // --- Rounding ---------------------------------------------------------

    WE_FORCEINLINE f32 floor(f32 x) noexcept
    {
        return std::floorf(x);
    }

    WE_FORCEINLINE f32 ceil(f32 x) noexcept
    {
        return std::ceilf(x);
    }

    WE_FORCEINLINE f32 round(f32 x) noexcept
    {
        return std::roundf(x);
    }

    WE_FORCEINLINE f32 trunc(f32 x) noexcept
    {
        return std::truncf(x);
    }

    // Fractional part, x - floor(x) (HLSL frac semantics).
    WE_FORCEINLINE f32 frac(f32 x) noexcept
    {
        return x - std::floorf(x);
    }

    // Floating-point remainder of x / y.
    WE_FORCEINLINE f32 mod(f32 x, f32 y) noexcept
    {
        return std::fmodf(x, y);
    }

    // --- Utility helpers --------------------------------------------------

    // -1, 0, or +1 according to the sign of x.
    WE_FORCEINLINE constexpr f32 sign(f32 x) noexcept
    {
        return x > 0.0f ? 1.0f : (x < 0.0f ? -1.0f : 0.0f);
    }

    WE_FORCEINLINE constexpr f32 square(f32 x) noexcept
    {
        return x * x;
    }

    // Reciprocal square root, 1 / sqrt(x).
    WE_FORCEINLINE f32 rsqrt(f32 x) noexcept
    {
        return 1.0f / squareRoot(x);
    }

    // 0 when x < edge, otherwise 1 (HLSL step semantics).
    WE_FORCEINLINE constexpr f32 step(f32 edge, f32 x) noexcept
    {
        return x < edge ? 0.0f : 1.0f;
    }

    // Smooth Hermite interpolation across the [edge0, edge1] band (HLSL
    // smoothstep semantics).
    WE_FORCEINLINE constexpr f32 smoothstep(f32 edge0, f32 edge1, f32 x) noexcept
    {
        f32 const t = saturate((x - edge0) / (edge1 - edge0));
        return t * t * (3.0f - 2.0f * t);
    }

    // True when lhs and rhs differ by no more than EPSILON.
    WE_FORCEINLINE constexpr bool approxEqual(f32 lhs, f32 rhs) noexcept
    {
        return abs(lhs - rhs) <= EPSILON;
    }

    WE_FORCEINLINE bool isNaN(f32 x) noexcept
    {
        return std::isnan(x);
    }

    WE_FORCEINLINE bool isInf(f32 x) noexcept
    {
        return std::isinf(x);
    }

    WE_FORCEINLINE bool isFinite(f32 x) noexcept
    {
        return std::isfinite(x);
    }

} // namespace worse::core::math