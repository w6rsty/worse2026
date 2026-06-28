module;

#include "worse/core/macro.hpp"

export module worse.core.container.memory_util;
import worse.core.basic_type;
import worse.core.intrinsics;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator_traits;

/**
 * \file
 * \brief Raw-storage lifetime primitives: construct objects into uninitialized memory,
 *        destroy a range, and -- the centerpiece -- RELOCATE a range (move-construct at
 *        the destination then destroy the source), with a `memcpy` fast path whenever the
 *        element type is trivially relocatable.
 * \note The single most-reused module: every dynamic container's grow / shrink / assign
 *       routes through `relocate` + the `uninitialized*` helpers, and the
 *       trivial-relocation fast path is where the game-perf win lands (no per-element
 *       move+dtor loop on grow).
 * \note Lives in `worse::core::container` (not the parent `worse::core`): these are
 *       container-storage machinery built on `AllocatorTraits`, not universal vocabulary.
 *       All element lifetime routes through `AllocatorTraits<Allocator>` (never raw
 *       placement-new / destroy_at), honouring any member construct/destroy an allocator
 *       provides. Internal calls to `move` are FULLY QUALIFIED (worse::core::move) to dodge
 *       the std:: ADL clash. Construction helpers assume non-throwing element ctors (true
 *       under the engine's -fno-exceptions contract) and so do no commit-or-rollback.
 */
export namespace worse::core::container
{
    // --- destroy --------------------------------------------------------------

    /** \brief Destroy the single object at \p p via the allocator. */
    template <typename Allocator, typename T>
    constexpr void destroyAt(Allocator& allocator, T* p) noexcept
    {
        AllocatorTraits<Allocator>::destroy(allocator, p);
    }

    /**
     * \brief Destroy `[first, last)`.
     * \note Compiled out entirely for trivially-destructible types.
     */
    template <typename Allocator, typename T>
    constexpr void destroyRange(Allocator& allocator, T* first, T* last) noexcept
    {
        if constexpr (!IsTriviallyDestructible<T>)
        {
            for (; first != last; ++first)
            {
                AllocatorTraits<Allocator>::destroy(allocator, first);
            }
        }
        else
        {
            (void)allocator;
            (void)first;
            (void)last;
        }
    }

    // --- uninitialized construction -------------------------------------------

    /**
     * \brief Default-construct `[first, last)` into raw storage.
     * \note Trivially-default-constructible types are LEFT UNINITIALIZED (the game-perf
     *       choice: no needless zeroing of POD storage); use uninitializedValueConstruct
     *       when zeroing is wanted.
     */
    template <typename Allocator, typename T>
    void uninitializedDefaultConstruct(Allocator& allocator, T* first, T* last)
    {
        if constexpr (IsTriviallyDefaultConstructible<T>)
        {
            (void)allocator;
            (void)first;
            (void)last;
        }
        else
        {
            for (; first != last; ++first)
            {
                AllocatorTraits<Allocator>::construct(allocator, first);
            }
        }
    }

    /** \brief Value-construct `[first, last)` into raw storage (zero-initializes trivial types). */
    template <typename Allocator, typename T>
    void uninitializedValueConstruct(Allocator& allocator, T* first, T* last)
    {
        for (; first != last; ++first)
        {
            AllocatorTraits<Allocator>::construct(allocator, first);
        }
    }

    /** \brief Copy-construct \p value into every slot of `[first, last)`. */
    template <typename Allocator, typename T, typename U>
    void uninitializedFill(Allocator& allocator, T* first, T* last, U const& value)
    {
        for (; first != last; ++first)
        {
            AllocatorTraits<Allocator>::construct(allocator, first, value);
        }
    }

    /**
     * \brief Copy-construct \p value into \p n slots starting at \p first.
     * \return One past the last constructed slot.
     */
    template <typename Allocator, typename T, typename U>
    T* uninitializedFillN(Allocator& allocator, T* first, usize n, U const& value)
    {
        for (usize i = 0; i < n; ++i, ++first)
        {
            AllocatorTraits<Allocator>::construct(allocator, first, value);
        }
        return first;
    }

    /**
     * \brief Copy-construct `[first, last)` into \p dest.
     * \return The destination end.
     * \note memcpy fast path when source and destination are the same trivially-copyable type.
     */
    template <typename Allocator, typename T, typename U>
    T* uninitializedCopy(Allocator& allocator, U const* first, U const* last, T* dest)
    {
        if constexpr (IsSame<RemoveCv<T>, RemoveCv<U>> && IsTriviallyCopyable<T>)
        {
            usize const n = static_cast<usize>(last - first);
            if (n != 0)
            {
                intrinsics::memCopy(static_cast<void*>(dest), static_cast<void const*>(first), n * sizeof(T));
            }
            (void)allocator;
            return dest + n;
        }
        else
        {
            for (; first != last; ++first, ++dest)
            {
                AllocatorTraits<Allocator>::construct(allocator, dest, *first);
            }
            return dest;
        }
    }

    /**
     * \brief Move-construct `[first, last)` into \p dest; sources left moved-from, NOT destroyed.
     * \return The destination end.
     * \note memcpy fast path for trivially-copyable types.
     */
    template <typename Allocator, typename T>
    T* uninitializedMove(Allocator& allocator, T* first, T* last, T* dest)
    {
        if constexpr (IsTriviallyCopyable<T>)
        {
            usize const n = static_cast<usize>(last - first);
            if (n != 0)
            {
                intrinsics::memCopy(static_cast<void*>(dest), static_cast<void const*>(first), n * sizeof(T));
            }
            (void)allocator;
            return dest + n;
        }
        else
        {
            for (; first != last; ++first, ++dest)
            {
                AllocatorTraits<Allocator>::construct(allocator, dest, worse::core::move(*first));
            }
            return dest;
        }
    }

    // --- relocate: the grow/shrink primitive ----------------------------------

    /**
     * \brief Relocate `[first, last)` into \p dest: move-construct each at the destination
     *        then destroy the source.
     * \pre Source and destination ranges MUST NOT overlap (the fresh-buffer grow case).
     * \return The destination end.
     * \note For a TRIVIALLY RELOCATABLE type this collapses to a single `memcpy` and the
     *       source bytes are simply abandoned (NO per-element destroy) -- the whole point
     *       of the trait.
     * \note In-place overlapping shifts (vector insert/erase) are the container's
     *       responsibility (memmove fast path + move-assignment loop there), because an
     *       overlapping shift mixes assignment into live slots with construction into raw
     *       ones -- semantics relocate deliberately does not cover.
     */
    template <typename Allocator, typename T>
    T* relocate(Allocator& allocator, T* first, T* last, T* dest)
    {
        if constexpr (IsTriviallyRelocatable<T>)
        {
            usize const n = static_cast<usize>(last - first);
            if (n != 0)
            {
                // memcpy is valid for a type that is trivially RELOCATABLE even when
                // it is not trivially copyable; the void* casts silence the
                // (here-intentional) -Wnontrivial-memcall.
                intrinsics::memCopy(static_cast<void*>(dest), static_cast<void const*>(first), n * sizeof(T));
            }
            (void)allocator;
            return dest + n;
        }
        else
        {
            for (; first != last; ++first, ++dest)
            {
                AllocatorTraits<Allocator>::construct(allocator, dest, worse::core::move(*first));
                AllocatorTraits<Allocator>::destroy(allocator, first);
            }
            return dest;
        }
    }

    /**
     * \brief High->low mirror of relocate: the last source maps to `(destLast - 1)`.
     * \pre Source and destination ranges MUST NOT overlap (same contract as relocate).
     * \return The new begin (`destLast - n`).
     * \note memcpy fast path for trivially-relocatable types.
     */
    template <typename Allocator, typename T>
    T* relocateBackward(Allocator& allocator, T* first, T* last, T* destLast)
    {
        usize const n = static_cast<usize>(last - first);
        if constexpr (IsTriviallyRelocatable<T>)
        {
            T* const destFirst = destLast - n;
            if (n != 0)
            {
                intrinsics::memCopy(static_cast<void*>(destFirst), static_cast<void const*>(first), n * sizeof(T));
            }
            (void)allocator;
            return destFirst;
        }
        else
        {
            while (first != last)
            {
                --last;
                --destLast;
                AllocatorTraits<Allocator>::construct(allocator, destLast, worse::core::move(*last));
                AllocatorTraits<Allocator>::destroy(allocator, last);
            }
            return destLast;
        }
    }
} // namespace worse::core::container
