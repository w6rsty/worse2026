#pragma once

#if defined(__clang__)
    #define WE_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define WE_FORCEINLINE __forceinline
#else
    #define WE_FORCEINLINE inline __atttribute__((always_inline))
#endif