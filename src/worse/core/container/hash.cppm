module;

#include "worse/core/macro.hpp"

export module worse.core.container.hash;
import worse.core.basic_type;
import worse.core.type_traits;

// Default hashing vocabulary for the open-addressing hash containers (DECISIONS D2/R8).
// Three free primitives plus a `Hash<T>` functor in the std::hash spirit:
//
//   hashFinalize(x) -- murmur3 fmix64 avalanche mixer: turns a raw integer key into a
//                      well-spread hash so the low bits (used as the bucket index via
//                      `h & mask`) are not just the key's low bits. The single most
//                      important step for open addressing on power-of-two tables.
//   hashCombine(s,v) -- order-sensitive fold of `v` into seed `s`, for multi-field keys.
//   hashBytes(p,n)   -- FNV-1a over a raw byte range, for future String/StringView keys.
//
// `Hash<T>` ships specializations for integral, enum, and pointer types (the keys a game
// engine reaches for first). The PRIMARY template is declared-but-undefined: an
// unsupported `T` gives `Hash<T>` no `operator()`, so `HashFor<Hash<T>, T>` is `false`
// and the user can drop in their own `Hash<MyType>` specialization. Floating-point keys
// are deliberately NOT shipped here -- they need -0.0/NaN canonicalization before a bit
// hash is sound -- and are deferred to a later iteration.
//
// Helpers live in this module's namespace, simply left out of the `export` block (module
// linkage hides them) -- no `_detail` sub-namespace, per DECISIONS.
export namespace worse::core::container
{
    // --- integer mixing primitives --------------------------------------------

    // murmur3 fmix64: full-avalanche finalizer. Every input bit flips ~half the output
    // bits, so `hashFinalize(key) & mask` distributes even sequential integer keys.
    WE_NODISCARD constexpr u64 hashFinalize(u64 x) noexcept
    {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ULL;
        x ^= x >> 33;
        return x;
    }

    // Fold `value` into `seed` (boost-style, order sensitive). Mix `value` first so a
    // poorly-distributed component still perturbs the whole seed.
    WE_NODISCARD constexpr usize hashCombine(usize seed, usize value) noexcept
    {
        u64 const mixed = hashFinalize(static_cast<u64>(value));
        seed ^= static_cast<usize>(mixed) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }

    // FNV-1a over [data, data + len). Not constexpr (reads through void const*); the
    // integer mixers above are. For raw byte sequences such as future string keys.
    WE_NODISCARD inline usize hashBytes(void const* data, usize len) noexcept
    {
        constexpr u64 kOffsetBasis = 1469598103934665603ULL;
        constexpr u64 kPrime       = 1099511628211ULL;

        u64 hash       = kOffsetBasis;
        u8 const* byte = static_cast<u8 const*>(data);
        for (usize i = 0; i < len; ++i)
        {
            hash ^= static_cast<u64>(byte[i]);
            hash *= kPrime;
        }
        return static_cast<usize>(hash);
    }

    // --- Hash<T> functor ------------------------------------------------------
    //
    // Primary template: declared, never defined. An unsupported key type therefore has
    // no usable `Hash` (HashFor is false) and the user may specialize it. The `Enable`
    // parameter is the std::hash-style SFINAE hook the integral/enum specialization uses.
    template <typename T, typename Enable = void>
    struct Hash;

    // Integral (incl. bool / char family) and enum keys: widen to u64 (enums via their
    // underlying type), then avalanche. A signed key widens by the usual modular cast --
    // deterministic, which is all a hash needs.
    template <typename T>
    struct Hash<T, EnableIf<IsIntegral<T> || IsEnum<T>>>
    {
        WE_NODISCARD constexpr usize operator()(T value) const noexcept
        {
            if constexpr (IsEnum<T>)
            {
                return static_cast<usize>(hashFinalize(static_cast<u64>(static_cast<__underlying_type(T)>(value))));
            }
            else
            {
                return static_cast<usize>(hashFinalize(static_cast<u64>(value)));
            }
        }
    };

    // Pointer keys: hash the address bits. (Identity-by-address, like std::hash<T*>.)
    template <typename T>
    struct Hash<T*>
    {
        WE_NODISCARD usize operator()(T* pointer) const noexcept
        {
            return static_cast<usize>(hashFinalize(static_cast<u64>(reinterpret_cast<usize>(pointer))));
        }
    };
} // namespace worse::core::container
