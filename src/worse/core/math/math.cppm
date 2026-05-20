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

} // namespace worse::core::math