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

} // namespace worse::core::math