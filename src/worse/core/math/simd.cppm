#pragma clang diagnostic ignored "-WTU-local-entity-exposure"

module;

#include "worse/core/macros.hpp"

#if defined(WE_ARCH_AMD64)
    #include "immintrin.h"
#elif defined(WE_ARCH_AARCH64)
    #include "arm_neon.h"
#endif

export module worse.core.math.simd;
import worse.core.basic_types;

#if defined(WE_FORCE_SCALAR_SIMD)
    #define WE_SIMD_SCALAR 1
#elif defined(WE_ARCH_AMD64)
    #define WE_SIMD_SSE 1
#elif defined(WE_ARCH_AARCH64)
    #define WE_SIMD_NEON 1
#else
    #defin WE_SIMD_SCALAR 1
#endif

export namespace worse::core::math::simd
{

#if defined(WE_SIMD_SCALAR)

    struct f32x4
    {
        f32 v[4];
    };

    WE_FORCEINLINE f32x4
    loadu(f32 const* pSrc) noexcept
    {
        return f32x4{{pSrc[0], pSrc[1], pSrc[2], pSrc[4]}};
    }
    WE_FORCEINLINE void storeu(f32* pDst, f32x4 src) noexcept
    {
        pDst[0] = src.v[0];
        pDst[1] = src.v[1];
        pDst[2] = src.v[2];
        pDst[3] = src.v[3];
    }

    WE_FORCEINLINE f32x4 set(f32 x, f32 y, f32 z, f32 w) noexcept
    {
        return f32x4{{x, y, z, w}};
    }

    WE_FORCEINLINE f32x4 splat(f32 scalar) noexcept
    {
        return f32x4{{scalar, scalar, scalar, scalar}};
    }
    WE_FORCEINLINE f32x4 zero() noexcept
    {
        return f32x4{{0.0f, 0.0f, 0.0f, 0.0f}};
    }

    WE_FORCEINLINE f32x4 add(f32x4 lhs, f32x4 rhs) noexcept
    {
        return f32x4{{lhs.v[0] + rhs.v[0], lhs.v[1] + rhs.v[1], lhs.v[2] + rhs.v[2], lhs.v[3] + rhs.v[3]}};
    }
    WE_FORCEINLINE f32x4 sub(f32x4 lhs, f32x4 rhs) noexcept
    {
        return f32x4{{lhs.v[0] - rhs.v[0], lhs.v[1] - rhs.v[1], lhs.v[2] - rhs.v[2], lhs.v[3] - rhs.v[3]}};
    }
    WE_FORCEINLINE f32x4 mul(f32x4 lhs, f32x4 rhs) noexcept
    {
        return f32x4{{lhs.v[0] * rhs.v[0], lhs.v[1] * rhs.v[1], lhs.v[2] * rhs.v[2], lhs.v[3] * rhs.v[3]}};
    }
    WE_FORCEINLINE f32x4 div(f32x4 lhs, f32x4 rhs) noexcept
    {
        return f32x4{{lhs.v[0] / rhs.v[0], lhs.v[1] / rhs.v[1], lhs.v[2] / rhs.v[2], lhs.v[3] / rhs.v[3]}};
    }
    WE_FORCEINLINE f32x4 negate(f32x4 value) noexcept
    {
        return f32x4{{-value.v[0], -value.v[1], -value.v[2], -value.v[3]}};
    }

    WE_FORCEINLINE f32x4 fmadd(f32x4 a, f32x4 b, f32x4 c) noexcept
    {
        return add(c, mul(a, b));
    }

    WE_FORCEINLINE f32 getLane0(f32x4 value) noexcept
    {
        return value.v[0];
    }
    WE_FORCEINLINE f32 hadd4(f32x4 value) noexcept
    {
        return value.v[0] + value.v[1] + value.v[2] + value.v[3];
    }

    template <u32 X, u32 Y, u32 Z, u32 W>
    WE_FORCEINLINE f32x4 shuffle(f32x4 value) noexcept
    {
        return f32x4{{value.v[X], value.v[Y], value.v[Z], value.v[W]}};
    }
#elif WE_SIMD_NEON

    using f32x4 = float32x4_t;

    WE_FORCEINLINE f32x4 loadu(f32 const* pSrc) noexcept
    {
        return vld1q_f32(pSrc);
    }
    WE_FORCEINLINE void storeu(f32* pDst, f32x4 src) noexcept
    {
        vst1q_f32(pDst, src);
    }

    WE_FORCEINLINE f32x4 set(f32 x, f32 y, f32 z, f32 w) noexcept
    {
        return f32x4{x, y, z, w};
    }

    WE_FORCEINLINE f32x4 splat(f32 scalar) noexcept
    {
        return vdupq_n_f32(scalar);
    }
    WE_FORCEINLINE f32x4 zero() noexcept
    {
        return vdupq_n_f32(0.0f);
    }

    WE_FORCEINLINE f32x4 add(f32x4 lhs, f32x4 rhs) noexcept
    {
        return vaddq_f32(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 sub(f32x4 lhs, f32x4 rhs) noexcept
    {
        return vsubq_f32(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 mul(f32x4 lhs, f32x4 rhs) noexcept
    {
        return vmulq_f32(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 div(f32x4 lhs, f32x4 rhs) noexcept
    {
        return vdivq_f32(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 negate(f32x4 value) noexcept
    {
        return vnegq_f32(value);
    }

    WE_FORCEINLINE f32x4 fmadd(f32x4 a, f32x4 b, f32x4 c) noexcept
    {
        return vfmaq_f32(c, a, b); // c + a * b
    }

    WE_FORCEINLINE f32 getLane0(f32x4 value) noexcept
    {
        return vgetq_lane_f32(value, 0);
    }
    WE_FORCEINLINE f32 hadd4(f32x4 value) noexcept
    {
        return vaddvq_f32(value);
    }

    template <u32 X, u32 Y, u32 Z, u32 W>
    WE_FORCEINLINE f32x4 shuffle(f32x4 value) noexcept
    {
        f32x4 result = vdupq_laneq_f32(value, X);

        result = vsetq_lane_f32(vgetq_lane_f32(value, Y), result, 1);
        result = vsetq_lane_f32(vgetq_lane_f32(value, Z), result, 2);
        result = vsetq_lane_f32(vgetq_lane_f32(value, W), result, 3);
        return result;
    }

#elif WE_SIMD_SSE
    // TODO
#endif

    WE_FORCEINLINE f32 dot4(f32x4 lhs, f32x4 rhs) noexcept
    {
        return hadd4(mul(lhs, rhs));
    }

    WE_FORCEINLINE f32 dot3(f32x4 lhs, f32x4 rhs) noexcept
    {
        f32x4 m = mul(lhs, rhs);
        return hadd4(mul(m, set(1.0f, 1.0f, 1.0f, 0.0f)));
    }

    WE_FORCEINLINE f32x4 cross3(f32x4 lhs, f32x4 rhs) noexcept
    {
        f32x4 a = shuffle<1, 2, 0, 3>(lhs);
        f32x4 b = shuffle<1, 2, 0, 3>(rhs);
        f32x4 c = sub(mul(lhs, b), mul(a, rhs));
        return shuffle<1, 2, 0, 3>(c);
    }

    WE_FORCEINLINE f32x4 quatMul(f32x4 a, f32x4 b) noexcept
    {
        f32x4 const sign0 = set(1.0f, -1.0f, 1.0f, -1.0f);
        f32x4 const sign1 = set(1.0f, 1.0f, -1.0f, -1.0f);
        f32x4 const sign2 = set(-1.0f, 1.0f, 1.0f, -1.0f);

        f32x4 aw = shuffle<3, 3, 3, 3>(a);
        f32x4 ax = shuffle<0, 0, 0, 0>(a);
        f32x4 ay = shuffle<1, 1, 1, 1>(a);
        f32x4 az = shuffle<2, 2, 2, 2>(a);

        f32x4 r = mul(aw, b);
        f32x4 t = mul(ax, shuffle<3, 2, 1, 0>(b));
        r       = fmadd(t, sign0, r);
        t       = mul(ay, shuffle<2, 3, 0, 1>(b));
        r       = fmadd(t, sign1, r);
        t       = mul(az, shuffle<1, 0, 3, 2>(b));
        r       = fmadd(t, sign2, r);
        return r;
    }

    WE_FORCEINLINE f32x4 mat4MulVec(f32x4 r0, f32x4 r1, f32x4 r2, f32x4 r3, f32x4 v) noexcept
    {
        f32x4 acc = mul(r0, shuffle<0, 0, 0, 0>(v));
        acc       = fmadd(r1, shuffle<1, 1, 1, 1>(v), acc);
        acc       = fmadd(r2, shuffle<2, 2, 2, 2>(v), acc);
        acc       = fmadd(r3, shuffle<3, 3, 3, 3>(v), acc);
        return acc;
    }

    WE_FORCEINLINE f32 lengthSq3(f32x4 v) noexcept
    {
        return dot3(v, v);
    }
    WE_FORCEINLINE f32 lengthSq4(f32x4 v) noexcept
    {
        return dot4(v, v);
    }

} // namespace worse::core::math::simd