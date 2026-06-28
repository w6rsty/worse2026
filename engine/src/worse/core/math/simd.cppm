#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-WTU-local-entity-exposure"

module;

#include "worse/core/macro.hpp"

#if defined(WE_ARCH_AMD64)
    #include "immintrin.h"
#elif defined(WE_ARCH_AARCH64)
    #include "arm_neon.h"
#endif

export module worse.core.math.simd;
import worse.core.basic_type;
import worse.core.math;

#if defined(WE_FORCE_SCALAR_SIMD)
    #define WE_SIMD_SCALAR 1
#elif defined(WE_ARCH_AMD64)
    #define WE_SIMD_SSE 1
#elif defined(WE_ARCH_AARCH64)
    #define WE_SIMD_NEON 1
#else
    #define WE_SIMD_SCALAR 1
#endif

export namespace worse::core::math::simd
{

#if defined(WE_SIMD_SCALAR)

    struct f32x4
    {
        f32 v[4];
    };

    WE_FORCEINLINE f32x4 loadu(f32 const* pSrc) noexcept
    {
        return f32x4{{pSrc[0], pSrc[1], pSrc[2], pSrc[3]}};
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

    WE_FORCEINLINE f32x4 sqrt(f32x4 value) noexcept
    {
        return f32x4{{squareRoot(value.v[0]), squareRoot(value.v[1]), squareRoot(value.v[2]), squareRoot(value.v[3])}};
    }
    WE_FORCEINLINE f32x4 rsqrt(f32x4 value) noexcept
    {
        return f32x4{{1.0f / squareRoot(value.v[0]), 1.0f / squareRoot(value.v[1]), 1.0f / squareRoot(value.v[2]), 1.0f / squareRoot(value.v[3])}};
    }

    WE_FORCEINLINE f32x4 min(f32x4 lhs, f32x4 rhs) noexcept
    {
        return f32x4{{lhs.v[0] < rhs.v[0] ? lhs.v[0] : rhs.v[0], lhs.v[1] < rhs.v[1] ? lhs.v[1] : rhs.v[1], lhs.v[2] < rhs.v[2] ? lhs.v[2] : rhs.v[2], lhs.v[3] < rhs.v[3] ? lhs.v[3] : rhs.v[3]}};
    }
    WE_FORCEINLINE f32x4 max(f32x4 lhs, f32x4 rhs) noexcept
    {
        return f32x4{{lhs.v[0] > rhs.v[0] ? lhs.v[0] : rhs.v[0], lhs.v[1] > rhs.v[1] ? lhs.v[1] : rhs.v[1], lhs.v[2] > rhs.v[2] ? lhs.v[2] : rhs.v[2], lhs.v[3] > rhs.v[3] ? lhs.v[3] : rhs.v[3]}};
    }
    WE_FORCEINLINE f32x4 abs(f32x4 value) noexcept
    {
        return f32x4{{value.v[0] < 0.0f ? -value.v[0] : value.v[0], value.v[1] < 0.0f ? -value.v[1] : value.v[1], value.v[2] < 0.0f ? -value.v[2] : value.v[2], value.v[3] < 0.0f ? -value.v[3] : value.v[3]}};
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

    // In-place transpose of a 4x4 matrix held as four row registers.
    WE_FORCEINLINE void transpose4(f32x4& r0, f32x4& r1, f32x4& r2, f32x4& r3) noexcept
    {
        f32x4 const c0 = set(r0.v[0], r1.v[0], r2.v[0], r3.v[0]);
        f32x4 const c1 = set(r0.v[1], r1.v[1], r2.v[1], r3.v[1]);
        f32x4 const c2 = set(r0.v[2], r1.v[2], r2.v[2], r3.v[2]);
        f32x4 const c3 = set(r0.v[3], r1.v[3], r2.v[3], r3.v[3]);
        r0             = c0;
        r1             = c1;
        r2             = c2;
        r3             = c3;
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

    WE_FORCEINLINE f32x4 sqrt(f32x4 value) noexcept
    {
        return vsqrtq_f32(value);
    }
    WE_FORCEINLINE f32x4 rsqrt(f32x4 value) noexcept
    {
        // Accurate reciprocal square root (1 / sqrt); favors precision over the
        // approximate vrsqrteq path since normals depend on it.
        return vdivq_f32(vdupq_n_f32(1.0f), vsqrtq_f32(value));
    }

    WE_FORCEINLINE f32x4 min(f32x4 lhs, f32x4 rhs) noexcept
    {
        return vminq_f32(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 max(f32x4 lhs, f32x4 rhs) noexcept
    {
        return vmaxq_f32(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 abs(f32x4 value) noexcept
    {
        return vabsq_f32(value);
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

    // In-place transpose of a 4x4 matrix held as four row registers. vtrnq_f32
    // transposes adjacent 2x2 blocks; vcombine then swaps the 64-bit halves.
    WE_FORCEINLINE void transpose4(f32x4& r0, f32x4& r1, f32x4& r2, f32x4& r3) noexcept
    {
        float32x4x2_t const t01 = vtrnq_f32(r0, r1);
        float32x4x2_t const t23 = vtrnq_f32(r2, r3);
        r0                      = vcombine_f32(vget_low_f32(t01.val[0]), vget_low_f32(t23.val[0]));
        r1                      = vcombine_f32(vget_low_f32(t01.val[1]), vget_low_f32(t23.val[1]));
        r2                      = vcombine_f32(vget_high_f32(t01.val[0]), vget_high_f32(t23.val[0]));
        r3                      = vcombine_f32(vget_high_f32(t01.val[1]), vget_high_f32(t23.val[1]));
    }

#elif WE_SIMD_SSE

    using f32x4 = __m128;

    WE_FORCEINLINE f32x4 loadu(f32 const* pSrc) noexcept
    {
        return _mm_loadu_ps(pSrc);
    }
    WE_FORCEINLINE void storeu(f32* pDst, f32x4 src) noexcept
    {
        _mm_storeu_ps(pDst, src);
    }

    WE_FORCEINLINE f32x4 set(f32 x, f32 y, f32 z, f32 w) noexcept
    {
        return _mm_setr_ps(x, y, z, w); // setr: lane 0 == x
    }

    WE_FORCEINLINE f32x4 splat(f32 scalar) noexcept
    {
        return _mm_set1_ps(scalar);
    }
    WE_FORCEINLINE f32x4 zero() noexcept
    {
        return _mm_setzero_ps();
    }

    WE_FORCEINLINE f32x4 add(f32x4 lhs, f32x4 rhs) noexcept
    {
        return _mm_add_ps(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 sub(f32x4 lhs, f32x4 rhs) noexcept
    {
        return _mm_sub_ps(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 mul(f32x4 lhs, f32x4 rhs) noexcept
    {
        return _mm_mul_ps(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 div(f32x4 lhs, f32x4 rhs) noexcept
    {
        return _mm_div_ps(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 negate(f32x4 value) noexcept
    {
        return _mm_xor_ps(value, _mm_set1_ps(-0.0f)); // flip the sign bit
    }

    WE_FORCEINLINE f32x4 sqrt(f32x4 value) noexcept
    {
        return _mm_sqrt_ps(value);
    }
    WE_FORCEINLINE f32x4 rsqrt(f32x4 value) noexcept
    {
        // Accurate reciprocal square root (1 / sqrt); favors precision over the
        // approximate _mm_rsqrt_ps path since normals depend on it.
        return _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(value));
    }

    WE_FORCEINLINE f32x4 min(f32x4 lhs, f32x4 rhs) noexcept
    {
        return _mm_min_ps(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 max(f32x4 lhs, f32x4 rhs) noexcept
    {
        return _mm_max_ps(lhs, rhs);
    }
    WE_FORCEINLINE f32x4 abs(f32x4 value) noexcept
    {
        return _mm_andnot_ps(_mm_set1_ps(-0.0f), value); // clear the sign bit
    }

    WE_FORCEINLINE f32x4 fmadd(f32x4 a, f32x4 b, f32x4 c) noexcept
    {
    #if defined(__FMA__)
        return _mm_fmadd_ps(a, b, c); // c + a * b
    #else
        return _mm_add_ps(c, _mm_mul_ps(a, b));
    #endif
    }

    WE_FORCEINLINE f32 getLane0(f32x4 value) noexcept
    {
        return _mm_cvtss_f32(value);
    }
    WE_FORCEINLINE f32 hadd4(f32x4 value) noexcept
    {
        // SSE2 horizontal sum (no SSE3 _mm_hadd_ps dependency).
        __m128 shuf = _mm_movehl_ps(value, value); // [z, w, z, w]
        __m128 sums = _mm_add_ps(value, shuf);     // [x+z, y+w, ..]
        shuf        = _mm_shuffle_ps(sums, sums, _MM_SHUFFLE(1, 1, 1, 1));
        sums        = _mm_add_ss(sums, shuf); // (x+z) + (y+w)
        return _mm_cvtss_f32(sums);
    }

    template <u32 X, u32 Y, u32 Z, u32 W>
    WE_FORCEINLINE f32x4 shuffle(f32x4 value) noexcept
    {
        // result lane i == value[{X,Y,Z,W}[i]]; the params are compile-time
        // constants so _MM_SHUFFLE folds to an immediate.
        return _mm_shuffle_ps(value, value, _MM_SHUFFLE(W, Z, Y, X));
    }

    // In-place transpose of a 4x4 matrix held as four row registers.
    WE_FORCEINLINE void transpose4(f32x4& r0, f32x4& r1, f32x4& r2, f32x4& r3) noexcept
    {
        _MM_TRANSPOSE4_PS(r0, r1, r2, r3);
    }
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