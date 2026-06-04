module;

#include "worse/core/macro.hpp"

export module worse.core.container.hash;
import worse.core.basic_type;
import worse.core.type_traits;

/**
 * \file
 * \brief Default hashing vocabulary for the open-addressing hash containers (DECISIONS D2/R8).
 *
 * Three free primitives plus a `Hash<T>` functor in the std::hash spirit:
 *   - hashFinalize(x): murmur3 fmix64 avalanche mixer turning a raw integer key into a
 *     well-spread hash so the low bits (used as the bucket index via `h & mask`) are not
 *     just the key's low bits. The single most important step for open addressing on
 *     power-of-two tables.
 *   - hashCombine(s,v): order-sensitive fold of `v` into seed `s`, for multi-field keys.
 *   - hashBytes(p,n): FNV-1a over a raw byte range, for future String/StringView keys.
 *
 * `Hash<T>` ships specializations for integral, enum, and pointer types (the keys a game
 * engine reaches for first). The PRIMARY template is declared-but-undefined: an
 * unsupported `T` gives `Hash<T>` no `operator()`, so `HashFor<Hash<T>, T>` is `false`
 * and the user can drop in their own `Hash<MyType>` specialization.
 *
 * \note Floating-point keys are deliberately NOT shipped here -- they need -0.0/NaN
 *       canonicalization before a bit hash is sound -- and are deferred to a later iteration.
 * \note Helpers live in this module's namespace, simply left out of the `export` block
 *       (module linkage hides them) -- no `_detail` sub-namespace, per DECISIONS.
 */
export namespace worse::core::container
{
    // --- integer mixing primitives --------------------------------------------

    /**
     * \brief murmur3 fmix64 full-avalanche finalizer.
     * \param x raw integer key to mix.
     * \return well-spread hash of \p x.
     * \note Every input bit flips ~half the output bits, so `hashFinalize(key) & mask`
     *       distributes even sequential integer keys.
     */
    WE_NODISCARD constexpr u64 hashFinalize(u64 x) noexcept
    {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ULL;
        x ^= x >> 33;
        return x;
    }

    /**
     * \brief Fold \p value into \p seed (boost-style, order sensitive).
     * \param seed running hash accumulator.
     * \param value component to fold in.
     * \return updated seed.
     * \note \p value is mixed first so a poorly-distributed component still perturbs the
     *       whole seed.
     */
    WE_NODISCARD constexpr usize hashCombine(usize seed, usize value) noexcept
    {
        u64 const mixed = hashFinalize(static_cast<u64>(value));
        seed ^= static_cast<usize>(mixed) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        return seed;
    }

    /**
     * \brief FNV-1a hash over the byte range [data, data + len).
     * \param data start of the byte range.
     * \param len number of bytes to hash.
     * \return hash of the byte sequence.
     * \note Not constexpr (reads through `void const*`); the integer mixers above are.
     *       Intended for raw byte sequences such as future string keys.
     */
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

    /**
     * \brief Hash functor for key type \p T, in the std::hash spirit.
     * \ingroup ctr_hash
     * \tparam T key type to hash.
     * \tparam Enable std::hash-style SFINAE hook the integral/enum specialization uses.
     * \note Primary template: declared, never defined. An unsupported key type therefore
     *       has no usable `Hash` (HashFor is false) and the user may specialize it.
     * \note A `Hash<T>` must be DETERMINISTIC within a single run: equal keys hash equally
     *       and a key's hash does not vary across calls.
     */
    template <typename T, typename Enable = void>
    struct Hash;

    /**
     * \brief Hash specialization for integral (incl. bool / char family) and enum keys.
     * \note Widens to u64 (enums via their underlying type), then avalanches. A signed key
     *       widens by the usual modular cast -- deterministic, which is all a hash needs.
     */
    template <typename T>
    struct Hash<T, EnableIf<IsIntegral<T> || IsEnum<T>>>
    {
        /**
         * \brief Hash an integral or enum key.
         * \param value key to hash.
         * \return avalanche-mixed hash of \p value.
         */
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

    /**
     * \brief Hash specialization for pointer keys: hashes the address bits.
     * \note Identity-by-address, like std::hash<T*>.
     */
    template <typename T>
    struct Hash<T*>
    {
        /**
         * \brief Hash a pointer key by its address.
         * \param pointer key to hash.
         * \return avalanche-mixed hash of the address bits.
         */
        WE_NODISCARD usize operator()(T* pointer) const noexcept
        {
            return static_cast<usize>(hashFinalize(static_cast<u64>(reinterpret_cast<usize>(pointer))));
        }
    };
} // namespace worse::core::container
