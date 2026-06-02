module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.binary_search;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;

/**
 * \file
 * \brief Binary search over a range already sorted with respect to `comp` (default `Less<>`).
 * \note Iterator-pair API; works on any forward iterator (O(log n) comparisons, O(n)
 *       increments on non-random-access ranges) but O(log n) overall on the contiguous
 *       ranges the flat containers use. The `value` parameter is its own type `T` so a
 *       transparent comparator can look up a key without materializing a full element.
 *       Lives in the flat `worse::core` namespace alongside the other algorithms.
 */
export namespace worse::core
{
    /** \addtogroup algo_search */
    /** @{ */
    /**
     * \brief First position not ordered before \p value (leftmost insertion point keeping order).
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam It forward iterator.
     * \param value lookup key (own type \p T for transparent comparators).
     * \param comp strict-weak-ordering comparator.
     * \return iterator to that position, or \p last if every element precedes \p value.
     * \pre [first, last) is sorted with respect to \p comp.
     */
    template <typename It, typename T, typename Compare = Less<>>
        requires ForwardIterator<It>
    WE_NODISCARD constexpr It lowerBound(It first, It last, T const& value, Compare comp = Compare{})
    {
        using Distance = typename IteratorTraits<It>::DifferenceType;
        Distance len   = distance(first, last);
        while (len > 0)
        {
            Distance const half = len / 2;
            It mid              = first;
            advance(mid, half);
            if (comp(*mid, value))
            {
                first = mid;
                ++first;
                len -= half + 1;
            }
            else
            {
                len = half;
            }
        }
        return first;
    }

    /**
     * \brief First position ordered strictly after \p value (rightmost insertion point).
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam It forward iterator.
     * \param value lookup key.
     * \param comp strict-weak-ordering comparator.
     * \return iterator to that position, or \p last if no element follows \p value.
     * \pre [first, last) is sorted with respect to \p comp.
     */
    template <typename It, typename T, typename Compare = Less<>>
        requires ForwardIterator<It>
    WE_NODISCARD constexpr It upperBound(It first, It last, T const& value, Compare comp = Compare{})
    {
        using Distance = typename IteratorTraits<It>::DifferenceType;
        Distance len   = distance(first, last);
        while (len > 0)
        {
            Distance const half = len / 2;
            It mid              = first;
            advance(mid, half);
            if (comp(value, *mid))
            {
                len = half;
            }
            else
            {
                first = mid;
                ++first;
                len -= half + 1;
            }
        }
        return first;
    }

    /**
     * \brief Whether an element equivalent to \p value exists.
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam It forward iterator.
     * \param value lookup key.
     * \param comp strict-weak-ordering comparator; equivalence = neither orders before the other.
     * \return true if such an element is present.
     * \pre [first, last) is sorted with respect to \p comp.
     */
    template <typename It, typename T, typename Compare = Less<>>
        requires ForwardIterator<It>
    WE_NODISCARD constexpr bool binarySearch(It first, It last, T const& value, Compare comp = Compare{})
    {
        It const it = lowerBound(first, last, value, comp);
        return it != last && !comp(value, *it);
    }

    /**
     * \brief The subrange of all elements equivalent to \p value.
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam It forward iterator.
     * \param value lookup key.
     * \param comp strict-weak-ordering comparator.
     * \return the pair {lowerBound, upperBound}, an empty range at the insertion point if absent.
     * \pre [first, last) is sorted with respect to \p comp.
     */
    template <typename It, typename T, typename Compare = Less<>>
        requires ForwardIterator<It>
    WE_NODISCARD constexpr Pair<It, It> equalRange(It first, It last, T const& value, Compare comp = Compare{})
    {
        return makePair(lowerBound(first, last, value, comp), upperBound(first, last, value, comp));
    }
    /** @} */
} // namespace worse::core
