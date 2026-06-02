module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.modifying;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;

// Mutating sequence algorithms over iterator-pair ranges. These operate on
// ALREADY-CONSTRUCTED elements (assignment, not construction -- so no allocator,
// unlike memory_util's uninitialized*). copy/move carry a `memmove` fast path for
// raw-pointer ranges of trivially-copyable types; that path is skipped during
// constant evaluation (memmove is not constexpr) so the algorithms stay usable in
// constexpr. Flat `worse::core` namespace; internal move/swap calls fully qualified.
export namespace worse::core
{
    // --- element swap ---------------------------------------------------------

    template <typename It1, typename It2>
    constexpr void iterSwap(It1 a, It2 b)
    {
        worse::core::swap(*a, *b);
    }

    template <typename It1, typename It2>
        requires ForwardIterator<It1> && ForwardIterator<It2>
    constexpr It2 swapRanges(It1 first1, It1 last1, It2 first2)
    {
        for (; first1 != last1; ++first1, ++first2)
        {
            iterSwap(first1, first2);
        }
        return first2;
    }

    // --- copy / move ----------------------------------------------------------

    template <typename InIt, typename OutIt>
        requires InputIterator<InIt>
    constexpr OutIt copy(InIt first, InIt last, OutIt dest)
    {
        using Value = typename IteratorTraits<InIt>::ValueType;
        if constexpr (IsPointer<InIt> && IsPointer<OutIt> && IsTriviallyCopyable<Value>)
        {
            if (!__builtin_is_constant_evaluated())
            {
                usize const n = static_cast<usize>(last - first);
                if (n != 0)
                {
                    __builtin_memmove(static_cast<void*>(dest), static_cast<void const*>(first), n * sizeof(Value));
                }
                return dest + n;
            }
        }
        for (; first != last; ++first, ++dest)
        {
            *dest = *first;
        }
        return dest;
    }

    template <typename BiIt, typename OutBiIt>
        requires BidirectionalIterator<BiIt>
    constexpr OutBiIt copyBackward(BiIt first, BiIt last, OutBiIt destLast)
    {
        using Value = typename IteratorTraits<BiIt>::ValueType;
        if constexpr (IsPointer<BiIt> && IsPointer<OutBiIt> && IsTriviallyCopyable<Value>)
        {
            if (!__builtin_is_constant_evaluated())
            {
                usize const n           = static_cast<usize>(last - first);
                OutBiIt const destFirst = destLast - n;
                if (n != 0)
                {
                    __builtin_memmove(
                        static_cast<void*>(destFirst),
                        static_cast<void const*>(first),
                        n * sizeof(Value));
                }
                return destFirst;
            }
        }
        while (first != last)
        {
            *(--destLast) = *(--last);
        }
        return destLast;
    }

    template <typename InIt, typename OutIt>
        requires InputIterator<InIt>
    constexpr OutIt move(InIt first, InIt last, OutIt dest)
    {
        using Value = typename IteratorTraits<InIt>::ValueType;
        if constexpr (IsPointer<InIt> && IsPointer<OutIt> && IsTriviallyCopyable<Value>)
        {
            if (!__builtin_is_constant_evaluated())
            {
                usize const n = static_cast<usize>(last - first);
                if (n != 0)
                {
                    __builtin_memmove(static_cast<void*>(dest), static_cast<void const*>(first), n * sizeof(Value));
                }
                return dest + n;
            }
        }
        for (; first != last; ++first, ++dest)
        {
            *dest = worse::core::move(*first);
        }
        return dest;
    }

    template <typename BiIt, typename OutBiIt>
        requires BidirectionalIterator<BiIt>
    constexpr OutBiIt moveBackward(BiIt first, BiIt last, OutBiIt destLast)
    {
        using Value = typename IteratorTraits<BiIt>::ValueType;
        if constexpr (IsPointer<BiIt> && IsPointer<OutBiIt> && IsTriviallyCopyable<Value>)
        {
            if (!__builtin_is_constant_evaluated())
            {
                usize const n           = static_cast<usize>(last - first);
                OutBiIt const destFirst = destLast - n;
                if (n != 0)
                {
                    __builtin_memmove(
                        static_cast<void*>(destFirst),
                        static_cast<void const*>(first),
                        n * sizeof(Value));
                }
                return destFirst;
            }
        }
        while (first != last)
        {
            *(--destLast) = worse::core::move(*(--last));
        }
        return destLast;
    }

    // --- fill -----------------------------------------------------------------

    // memset fast path for raw-pointer ranges of 1-byte trivially-copyable elements
    // (clearing/initializing u8/byte/bool buffers -- the per-frame case). Larger element
    // types fall through to the scalar loop, which the optimizer can still lower to a
    // vector store / memset when `value` is a repeatable byte pattern. Skipped during
    // constant evaluation (memset is not constexpr), mirroring copy/move above.
    template <typename It, typename T>
        requires ForwardIterator<It>
    constexpr void fill(It first, It last, T const& value)
    {
        using Value = typename IteratorTraits<It>::ValueType;
        if constexpr (IsPointer<It> && IsTriviallyCopyable<Value> && sizeof(Value) == 1)
        {
            if (!__builtin_is_constant_evaluated())
            {
                usize const n = static_cast<usize>(last - first);
                if (n != 0)
                {
                    Value const tmp = value; // same conversion the scalar store performs
                    unsigned char byte;
                    __builtin_memcpy(&byte, &tmp, 1);
                    __builtin_memset(static_cast<void*>(first), byte, n);
                }
                return;
            }
        }
        for (; first != last; ++first)
        {
            *first = value;
        }
    }

    template <typename It, typename T>
    constexpr It fillN(It first, usize n, T const& value)
    {
        using Value = typename IteratorTraits<It>::ValueType;
        if constexpr (IsPointer<It> && IsTriviallyCopyable<Value> && sizeof(Value) == 1)
        {
            if (!__builtin_is_constant_evaluated())
            {
                if (n != 0)
                {
                    Value const tmp = value;
                    unsigned char byte;
                    __builtin_memcpy(&byte, &tmp, 1);
                    __builtin_memset(static_cast<void*>(first), byte, n);
                }
                return first + n;
            }
        }
        for (usize i = 0; i < n; ++i, ++first)
        {
            *first = value;
        }
        return first;
    }

    // --- reverse / rotate -----------------------------------------------------

    template <typename BiIt>
        requires BidirectionalIterator<BiIt>
    constexpr void reverse(BiIt first, BiIt last)
    {
        while (first != last)
        {
            --last;
            if (first == last)
            {
                break;
            }
            iterSwap(first, last);
            ++first;
        }
    }

    // Rotate so that *middle becomes the first element. Returns the new position of the
    // element that was at *first. Three-reversal method (simple, O(n)).
    template <typename BiIt>
        requires BidirectionalIterator<BiIt>
    constexpr BiIt rotate(BiIt first, BiIt middle, BiIt last)
    {
        if (first == middle)
        {
            return last;
        }
        if (middle == last)
        {
            return first;
        }
        reverse(first, middle);
        reverse(middle, last);
        reverse(first, last);
        BiIt result = first;
        advance(result, distance(middle, last));
        return result;
    }

    // --- remove / unique / replace --------------------------------------------
    //
    // remove/unique are the "erase-remove" front halves: they compact the kept
    // elements toward the front and return the new logical end; the caller trims the
    // tail. They never shrink the underlying range.

    template <typename It, typename T>
        requires ForwardIterator<It>
    constexpr It remove(It first, It last, T const& value)
    {
        It result = first;
        for (; first != last; ++first)
        {
            if (!(*first == value))
            {
                if (result != first)
                {
                    *result = worse::core::move(*first);
                }
                ++result;
            }
        }
        return result;
    }

    template <typename It, typename Pred>
        requires ForwardIterator<It>
    constexpr It removeIf(It first, It last, Pred pred)
    {
        It result = first;
        for (; first != last; ++first)
        {
            if (!pred(*first))
            {
                if (result != first)
                {
                    *result = worse::core::move(*first);
                }
                ++result;
            }
        }
        return result;
    }

    // Collapse consecutive equivalent elements to one; returns the new logical end.
    template <typename It, typename Pred = EqualTo<>>
        requires ForwardIterator<It>
    constexpr It unique(It first, It last, Pred pred = Pred{})
    {
        if (first == last)
        {
            return last;
        }
        It result = first;
        while (++first != last)
        {
            if (!pred(*result, *first))
            {
                ++result;
                if (result != first)
                {
                    *result = worse::core::move(*first);
                }
            }
        }
        return ++result;
    }

    template <typename It, typename T>
        requires ForwardIterator<It>
    constexpr void replace(It first, It last, T const& oldValue, T const& newValue)
    {
        for (; first != last; ++first)
        {
            if (*first == oldValue)
            {
                *first = newValue;
            }
        }
    }

    template <typename It, typename Pred, typename T>
        requires ForwardIterator<It>
    constexpr void replaceIf(It first, It last, Pred pred, T const& newValue)
    {
        for (; first != last; ++first)
        {
            if (pred(*first))
            {
                *first = newValue;
            }
        }
    }
} // namespace worse::core
