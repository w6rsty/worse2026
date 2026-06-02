module;

#include "worse/core/macro.hpp"

export module worse.core.intrinsics;
import worse.core.basic_type;

// Typed, force-inlined wrappers over the raw compiler intrinsics the container and
// algorithm code relies on, so call-sites read intent (`intrinsics::memCopy(...)`)
// instead of a bare `__builtin_*`. Follows the existing `addressOf` wrapper precedent
// (iterator.cppm). The `mem*` wrappers take `void*`/`void const*` so the call-site
// `static_cast<void*>` casts that silence `-Wnontrivial-memcall` on trivially-
// RELOCATABLE-but-not-copyable types keep working unchanged. `WE_FORCEINLINE`
// guarantees these collapse to the underlying builtin with zero codegen change on the
// hot paths (the force-inline that R42/R47 proved load-bearing for push/insert).
//
// Deliberately NOT wrapped: `addressOf` (already wrapped, iterator.cppm),
// `__STDCPP_DEFAULT_NEW_ALIGNMENT__` (a std macro, memory.cppm), and
// `__builtin_unreachable` (lives in macro.hpp, a header that cannot import a module).
export namespace worse::core::intrinsics
{
    // Raw byte copy/move/set. `mem*` mirrors the C library names; the wrappers return
    // the destination pointer like the builtins (every current call-site discards it).
    WE_FORCEINLINE void* memCopy(void* dest, void const* src, usize n) noexcept
    {
        return __builtin_memcpy(dest, src, n);
    }
    WE_FORCEINLINE void* memMove(void* dest, void const* src, usize n) noexcept
    {
        return __builtin_memmove(dest, src, n);
    }
    WE_FORCEINLINE void* memSet(void* dest, int value, usize n) noexcept
    {
        return __builtin_memset(dest, value, n);
    }

    // `true` during constant evaluation. Must stay `constexpr`: it is the guard that
    // routes around the (non-constexpr) `mem*` fast paths in copy/move/fill/sort.
    WE_FORCEINLINE constexpr bool isConstantEvaluated() noexcept
    {
        return __builtin_is_constant_evaluated();
    }

    // Count trailing zero bits of a 64-bit word. PRECONDITION: x != 0 (UB otherwise,
    // same contract as the underlying builtin). Used by the SwissTable SWAR group scan.
    WE_FORCEINLINE i32 countTrailingZeros64(u64 x) noexcept
    {
        WE_ASSERT(x != 0);
        return __builtin_ctzll(x);
    }
} // namespace worse::core::intrinsics
