#pragma once

#include <cstdlib>

#if defined(__clang__)
    #define WE_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define WE_FORCEINLINE __forceinline
#else
    #define WE_FORCEINLINE inline __attribute__((always_inline))
#endif

// --- Assertions -----------------------------------------------------------
//
// WE_VERIFY  — always-on; checks a critical invariant in every build.
// WE_ASSERT  — debug-only; compiled out when NDEBUG is defined.
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

#if defined(NDEBUG)
    #define WE_ASSERT(cond) ((void)0)
#else
    #define WE_ASSERT(cond) WE_VERIFY(cond)
#endif
