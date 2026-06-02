module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.modifying;
import worse.core.basic_type;
import worse.core.intrinsics;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;

/**
 * \file
 * \brief Mutating sequence algorithms over iterator-pair ranges (assignment, not
 *        construction -- no allocator, unlike memory_util's uninitialized*).
 * \note copy/move carry a `memmove` fast path for raw-pointer ranges of trivially-copyable
 *       types, skipped during constant evaluation (memmove is not constexpr) so the
 *       algorithms stay usable in constexpr.
 * \note Flat `worse::core` namespace; internal move/swap calls fully qualified.
 */
export namespace worse::core
{
    // --- element swap ---------------------------------------------------------

    /** \brief Swap the elements the two iterators point to. */
    template <typename It1, typename It2>
    constexpr void iterSwap(It1 a, It2 b)
    {
        worse::core::swap(*a, *b);
    }

    /**
     * \brief Swap [first1, last1) element-wise with the range starting at \p first2.
     * \return iterator past the last swapped element of the second range.
     */
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

    /**
     * \brief Copy-assign [first, last) into the range beginning at \p dest.
     * \tparam InIt input iterator.
     * \return iterator past the last copied element of the output range.
     * \pre [dest, dest + (last-first)) does not overlap [first, last) toward the front.
     * \note `memmove` fast path for raw-pointer ranges of trivially-copyable types, skipped
     *       during constant evaluation.
     */
    template <typename InIt, typename OutIt>
        requires InputIterator<InIt>
    constexpr OutIt copy(InIt first, InIt last, OutIt dest)
    {
        using Value = typename IteratorTraits<InIt>::ValueType;
        if constexpr (IsPointer<InIt> && IsPointer<OutIt> && IsTriviallyCopyable<Value>)
        {
            if (!intrinsics::isConstantEvaluated())
            {
                usize const n = static_cast<usize>(last - first);
                if (n != 0)
                {
                    intrinsics::memMove(static_cast<void*>(dest), static_cast<void const*>(first), n * sizeof(Value));
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

    /**
     * \brief Copy-assign [first, last) into the range ending at \p destLast, working backward.
     * \return iterator to the first written element. Use when ranges overlap and \p destLast > \p last.
     * \note Same `memmove` fast path / constexpr skip as copy.
     */
    template <typename BiIt, typename OutBiIt>
        requires BidirectionalIterator<BiIt>
    constexpr OutBiIt copyBackward(BiIt first, BiIt last, OutBiIt destLast)
    {
        using Value = typename IteratorTraits<BiIt>::ValueType;
        if constexpr (IsPointer<BiIt> && IsPointer<OutBiIt> && IsTriviallyCopyable<Value>)
        {
            if (!intrinsics::isConstantEvaluated())
            {
                usize const n           = static_cast<usize>(last - first);
                OutBiIt const destFirst = destLast - n;
                if (n != 0)
                {
                    intrinsics::memMove(
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

    /**
     * \brief Move-assign [first, last) into the range beginning at \p dest.
     * \return iterator past the last moved element. Same `memmove` fast path / constexpr skip as copy.
     */
    template <typename InIt, typename OutIt>
        requires InputIterator<InIt>
    constexpr OutIt move(InIt first, InIt last, OutIt dest)
    {
        using Value = typename IteratorTraits<InIt>::ValueType;
        if constexpr (IsPointer<InIt> && IsPointer<OutIt> && IsTriviallyCopyable<Value>)
        {
            if (!intrinsics::isConstantEvaluated())
            {
                usize const n = static_cast<usize>(last - first);
                if (n != 0)
                {
                    intrinsics::memMove(static_cast<void*>(dest), static_cast<void const*>(first), n * sizeof(Value));
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

    /**
     * \brief Move-assign [first, last) into the range ending at \p destLast, working backward.
     * \return iterator to the first written element. Same `memmove` fast path / constexpr skip as copy.
     */
    template <typename BiIt, typename OutBiIt>
        requires BidirectionalIterator<BiIt>
    constexpr OutBiIt moveBackward(BiIt first, BiIt last, OutBiIt destLast)
    {
        using Value = typename IteratorTraits<BiIt>::ValueType;
        if constexpr (IsPointer<BiIt> && IsPointer<OutBiIt> && IsTriviallyCopyable<Value>)
        {
            if (!intrinsics::isConstantEvaluated())
            {
                usize const n           = static_cast<usize>(last - first);
                OutBiIt const destFirst = destLast - n;
                if (n != 0)
                {
                    intrinsics::memMove(
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

    /**
     * \brief Assign \p value to every element of [first, last).
     * \tparam It forward iterator.
     * \note memset fast path for raw-pointer ranges of 1-byte trivially-copyable elements
     *       (clearing/initializing u8/byte/bool buffers -- the per-frame case); larger element
     *       types fall through to the scalar loop, which the optimizer can still lower to a
     *       vector store / memset for a repeatable byte pattern. Skipped during constant
     *       evaluation (memset is not constexpr), mirroring copy/move. (R45/P8)
     */
    template <typename It, typename T>
        requires ForwardIterator<It>
    constexpr void fill(It first, It last, T const& value)
    {
        using Value = typename IteratorTraits<It>::ValueType;
        if constexpr (IsPointer<It> && IsTriviallyCopyable<Value> && sizeof(Value) == 1)
        {
            if (!intrinsics::isConstantEvaluated())
            {
                usize const n = static_cast<usize>(last - first);
                if (n != 0)
                {
                    Value const tmp = value; // same conversion the scalar store performs
                    unsigned char byte;
                    intrinsics::memCopy(&byte, &tmp, 1);
                    intrinsics::memSet(static_cast<void*>(first), byte, n);
                }
                return;
            }
        }
        for (; first != last; ++first)
        {
            *first = value;
        }
    }

    /**
     * \brief Assign \p value to the first \p n elements from \p first.
     * \return iterator past the last written element.
     * \note Same memset fast path / constexpr skip as fill. (R45/P8)
     */
    template <typename It, typename T>
    constexpr It fillN(It first, usize n, T const& value)
    {
        using Value = typename IteratorTraits<It>::ValueType;
        if constexpr (IsPointer<It> && IsTriviallyCopyable<Value> && sizeof(Value) == 1)
        {
            if (!intrinsics::isConstantEvaluated())
            {
                if (n != 0)
                {
                    Value const tmp = value;
                    unsigned char byte;
                    intrinsics::memCopy(&byte, &tmp, 1);
                    intrinsics::memSet(static_cast<void*>(first), byte, n);
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

    /** \brief Reverse the order of elements in [first, last) in place. O(n). */
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

    /**
     * \brief Rotate [first, last) so \p middle becomes the first element.
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \param middle new first element after the rotation.
     * \return the new position of the element that was at \p first.
     * \note Three-reversal method, O(n).
     */
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

    /**
     * \brief Compact [first, last) keeping only elements not equal to \p value.
     * \tparam It forward iterator.
     * \return the new logical end; elements in [result, last) are unspecified.
     * \note "Erase-remove" front half: compacts kept elements toward the front and never
     *       shrinks the underlying range; the caller trims the tail.
     */
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

    /** \brief Compact [first, last) keeping only elements NOT satisfying \p pred; returns the new logical end. */
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

    /**
     * \brief Collapse each run of consecutive elements equivalent under \p pred to one.
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam It forward iterator.
     * \param pred binary equivalence predicate (default `EqualTo<>`).
     * \return the new logical end; the caller trims the tail (erase-remove front half).
     */
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

    /** \brief Replace every element of [first, last) equal to \p oldValue with \p newValue. */
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

    /** \brief Replace every element of [first, last) satisfying \p pred with \p newValue. */
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
