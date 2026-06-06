module;

#include <cstdint>
#include <cstddef>
#include <limits>

export module worse.core.basic_type;

/**
 * \file
 * \brief Fixed-width scalar aliases (`u8`/`i32`/`f32`/`usize`/...), their numeric
 *        limits, and the matching `_u8`/`_i32`/`_f32`/... user-defined literals.
 */
export namespace worse
{

    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using f32 = float;
    using f64 = double;

    using usize = std::size_t;
    using isize = std::ptrdiff_t;

    // --- Numeric limits ---------------------------------------------------

    inline constexpr u8 U8_MIN = std::numeric_limits<u8>::min();
    inline constexpr u8 U8_MAX = std::numeric_limits<u8>::max();

    inline constexpr u16 U16_MIN = std::numeric_limits<u16>::min();
    inline constexpr u16 U16_MAX = std::numeric_limits<u16>::max();

    inline constexpr u32 U32_MIN = std::numeric_limits<u32>::min();
    inline constexpr u32 U32_MAX = std::numeric_limits<u32>::max();

    inline constexpr u64 U64_MIN = std::numeric_limits<u64>::min();
    inline constexpr u64 U64_MAX = std::numeric_limits<u64>::max();

    inline constexpr i8 I8_MIN = std::numeric_limits<i8>::min();
    inline constexpr i8 I8_MAX = std::numeric_limits<i8>::max();

    inline constexpr i16 I16_MIN = std::numeric_limits<i16>::min();
    inline constexpr i16 I16_MAX = std::numeric_limits<i16>::max();

    inline constexpr i32 I32_MIN = std::numeric_limits<i32>::min();
    inline constexpr i32 I32_MAX = std::numeric_limits<i32>::max();

    inline constexpr i64 I64_MIN = std::numeric_limits<i64>::min();
    inline constexpr i64 I64_MAX = std::numeric_limits<i64>::max();

    inline constexpr usize USIZE_MIN = std::numeric_limits<usize>::min();
    inline constexpr usize USIZE_MAX = std::numeric_limits<usize>::max();

    inline constexpr isize ISIZE_MIN = std::numeric_limits<isize>::min();
    inline constexpr isize ISIZE_MAX = std::numeric_limits<isize>::max();

    /**
     * \brief `F32_MIN` is the smallest positive normalized value; `F32_LOWEST` is the
     *        most negative finite value.
     * \note `F32_EPSILON` is the machine epsilon (1 ulp at 1.0) — distinct from
     *       math::EPSILON, which is a comparison tolerance.
     */
    inline constexpr f32 F32_MIN      = std::numeric_limits<f32>::min();
    inline constexpr f32 F32_MAX      = std::numeric_limits<f32>::max();
    inline constexpr f32 F32_LOWEST   = std::numeric_limits<f32>::lowest();
    inline constexpr f32 F32_EPSILON  = std::numeric_limits<f32>::epsilon();
    inline constexpr f32 F32_INFINITY = std::numeric_limits<f32>::infinity();
    inline constexpr f32 F32_NAN      = std::numeric_limits<f32>::quiet_NaN();

    inline constexpr f64 F64_MIN      = std::numeric_limits<f64>::min();
    inline constexpr f64 F64_MAX      = std::numeric_limits<f64>::max();
    inline constexpr f64 F64_LOWEST   = std::numeric_limits<f64>::lowest();
    inline constexpr f64 F64_EPSILON  = std::numeric_limits<f64>::epsilon();
    inline constexpr f64 F64_INFINITY = std::numeric_limits<f64>::infinity();
    inline constexpr f64 F64_NAN      = std::numeric_limits<f64>::quiet_NaN();

    constexpr u8 operator""_u8(unsigned long long v) noexcept
    {
        return static_cast<u8>(v);
    }
    constexpr u16 operator""_u16(unsigned long long v) noexcept
    {
        return static_cast<u16>(v);
    }
    constexpr u32 operator""_u32(unsigned long long v) noexcept
    {
        return static_cast<u32>(v);
    }
    constexpr u64 operator""_u64(unsigned long long v) noexcept
    {
        return static_cast<u64>(v);
    }

    constexpr i8 operator""_i8(unsigned long long v) noexcept
    {
        return static_cast<i8>(v);
    }
    constexpr i16 operator""_i16(unsigned long long v) noexcept
    {
        return static_cast<i16>(v);
    }
    constexpr i32 operator""_i32(unsigned long long v) noexcept
    {
        return static_cast<i32>(v);
    }
    constexpr i64 operator""_i64(unsigned long long v) noexcept
    {
        return static_cast<i64>(v);
    }

    constexpr usize operator""_usize(unsigned long long v) noexcept
    {
        return static_cast<usize>(v);
    }
    constexpr isize operator""_isize(unsigned long long v) noexcept
    {
        return static_cast<isize>(v);
    }

    constexpr f32 operator""_f32(long double v) noexcept
    {
        return static_cast<f32>(v);
    }
    constexpr f64 operator""_f64(long double v) noexcept
    {
        return static_cast<f64>(v);
    }

} // namespace worse