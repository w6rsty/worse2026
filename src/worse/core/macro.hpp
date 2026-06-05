#pragma once

#include <cstdlib>

#if defined(__clang__)
    #define WE_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define WE_FORCEINLINE __forceinline
#else
    #define WE_FORCEINLINE inline __attribute__((always_inline))
#endif

#if defined(_MSC_VER)
    #define WE_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
    #define WE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#define WE_NODISCARD [[nodiscard]]

#define WE_NORETURN [[noreturn]]

#define WE_MAYBE_UNUSED [[maybe_unused]]

// Declare a type as trivially relocatable: "move-construct then destroy source" is
// byte-equivalent to memcpy, so containers may relocate it with memcpy on grow/erase/
// rehash. Trivially-copyable types qualify automatically; use this for types that are
// not (e.g. own a heap pointer with a non-trivial move) but are still relocatable.
//
// Use at namespace scope, only where `import worse.core.type_traits;` is in effect:
//     WE_DECLARE_TRIVIALLY_RELOCATABLE(MyType);
#define WE_DECLARE_TRIVIALLY_RELOCATABLE(TYPE)           \
    template <>                                          \
    struct ::worse::core::WeIsTriviallyRelocatable<TYPE> \
    {                                                    \
        static constexpr bool value = true;              \
    }

// --- Assertions -----------------------------------------------------------
//
// WE_VERIFY  — always-on; checks a critical invariant in every build.
// WE_ASSERT  — debug-only; compiled out when NDEBUG is defined, UNLESS the build forces
//              them on via WE_ENABLE_ASSERTS (the WORSE_ENABLE_ASSERTS CMake option), which
//              keeps assertions in optimized builds without otherwise leaving debug mode.
//
// On failure both abort(). abort() (not throw) keeps these valid once the
// engine is built with exceptions disabled.

#define WE_VERIFY(cond)   \
    do                    \
    {                     \
        if (!(cond))      \
        {                 \
            std::abort(); \
        }                 \
    } while (false)

#if defined(NDEBUG) && !defined(WE_ENABLE_ASSERTS)
    #define WE_ASSERT(cond) ((void)0)
#else
    #define WE_ASSERT(cond) WE_VERIFY(cond)
#endif

#if defined(NDEBUG) && !defined(WE_ENABLE_ASSERTS)
    #define WE_ASSERT_MSG(cond, msg) ((void)0)
#else
    #define WE_ASSERT_MSG(cond, msg) WE_VERIFY(cond)
#endif

namespace worse::core
{
    WE_NORETURN inline void unreachable()
    {
#if defined(__clang__)
        __builtin_unreachable();
#elif defined(_MSC_VER)
        __assume(false);
#else
    #include <cassert>
        assert(false && "Unreachable code executed!");
#endif
    }
} // namespace worse::core