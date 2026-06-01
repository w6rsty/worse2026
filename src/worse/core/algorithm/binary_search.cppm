module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.binary_search;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;

// Binary search over a range already sorted with respect to `comp` (default `Less<>`).
// Iterator-pair API; works on any forward iterator (O(log n) comparisons, O(n)
// increments on non-random-access ranges) but is O(log n) overall on the contiguous
// ranges the flat containers use. The `value` parameter is its own type `T` so a
// transparent comparator can look up a key without materializing a full element.
//
// Lives in the flat `worse::core` namespace alongside the other algorithms.
export namespace worse::core
{
    // First position where the element is NOT ordered before `value` (i.e. the leftmost
    // insertion point that keeps the range sorted). == last if every element precedes value.
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

    // First position ordered strictly AFTER `value` (the rightmost insertion point).
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

    // Whether an element equivalent to `value` exists (equivalence = neither orders
    // before the other under `comp`).
    template <typename It, typename T, typename Compare = Less<>>
        requires ForwardIterator<It>
    WE_NODISCARD constexpr bool binarySearch(It first, It last, T const& value, Compare comp = Compare{})
    {
        It const it = lowerBound(first, last, value, comp);
        return it != last && !comp(value, *it);
    }

    // The subrange [lowerBound, upperBound) of all elements equivalent to `value`.
    template <typename It, typename T, typename Compare = Less<>>
        requires ForwardIterator<It>
    WE_NODISCARD constexpr Pair<It, It> equalRange(It first, It last, T const& value, Compare comp = Compare{})
    {
        return makePair(lowerBound(first, last, value, comp), upperBound(first, last, value, comp));
    }
} // namespace worse::core
