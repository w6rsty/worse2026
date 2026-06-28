module;

#include "worse/core/macro.hpp"

export module worse.core.algorithm.heap;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;

/**
 * \file
 * \brief Binary max-heap operations over a random-access range (iterator-pair API with an
 *        optional comparator; default `Less<>` => max-heap, largest element on top).
 * \note These power introsort's heapsort fallback and PriorityQueue. The sift routines use
 *       the "hole" technique (one move per level instead of a 3-move swap) -- the same shape
 *       as libstdc++'s __adjust_heap/__push_heap, the proven, branch-lean form.
 * \note Algorithms live in the flat `worse::core` namespace (the engine's `std::`-equivalent
 *       vocabulary), so callers use `makeHeap`, `sortHeap`, ... unqualified; every internal
 *       `move` is fully qualified to avoid the std:: ADL clash.
 */

// Internal sift helpers: NOT in the `export` block below, so module linkage already
// hides them from importers -- no `_detail` sub-namespace needed (matches the
// allocator_traits convention of non-exported helpers in the main namespace).
namespace worse::core
{
    // Sift the hole at `holeIndex` UP toward `topIndex`, settling `value` where the
    // heap order is restored. Used after appending (pushHeap) and at the tail of
    // adjustHeap.
    template <typename RandomIt, typename Distance, typename T, typename Compare>
    constexpr void siftUp(RandomIt first, Distance holeIndex, Distance topIndex, T value, Compare& comp)
    {
        Distance parent = (holeIndex - 1) / 2;
        while (holeIndex > topIndex && comp(first[parent], value))
        {
            first[holeIndex] = worse::core::move(first[parent]);
            holeIndex        = parent;
            parent           = (holeIndex - 1) / 2;
        }
        first[holeIndex] = worse::core::move(value);
    }

    // Sift the hole at `holeIndex` DOWN through a heap of length `len` (always taking
    // the larger child), then siftUp `value` into the resulting hole.
    template <typename RandomIt, typename Distance, typename T, typename Compare>
    constexpr void adjustHeap(RandomIt first, Distance holeIndex, Distance len, T value, Compare& comp)
    {
        Distance const topIndex = holeIndex;
        Distance secondChild    = holeIndex;
        while (secondChild < (len - 1) / 2)
        {
            secondChild = 2 * (secondChild + 1);
            if (comp(first[secondChild], first[secondChild - 1]))
            {
                --secondChild;
            }
            first[holeIndex] = worse::core::move(first[secondChild]);
            holeIndex        = secondChild;
        }
        // Odd length: a lone left child may remain.
        if ((len & 1) == 0 && secondChild == (len - 2) / 2)
        {
            secondChild      = 2 * (secondChild + 1);
            first[holeIndex] = worse::core::move(first[secondChild - 1]);
            holeIndex        = secondChild - 1;
        }
        siftUp(first, holeIndex, topIndex, worse::core::move(value), comp);
    }
} // namespace worse::core

export namespace worse::core
{
    /** \addtogroup algo_heap */
    /** @{ */
    /**
     * \brief Restore the heap after the element at (last-1) was just appended.
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam RandomIt random-access iterator.
     * \param comp strict-weak-ordering comparator (default `Less<>` => max-heap).
     * \pre [first, last-1) is a heap with respect to \p comp.
     * \note O(log n).
     */
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void pushHeap(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        using Distance     = typename IteratorTraits<RandomIt>::DifferenceType;
        using Value        = typename IteratorTraits<RandomIt>::ValueType;
        Distance const len = last - first;
        if (len < 2)
        {
            return;
        }
        Value value = worse::core::move(first[len - 1]);
        siftUp(first, Distance(len - 1), Distance(0), worse::core::move(value), comp);
    }

    /**
     * \brief Move the top (max) element to (last-1) and restore the heap on [first, last-1).
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam RandomIt random-access iterator.
     * \param comp strict-weak-ordering comparator (default `Less<>` => max-heap).
     * \pre [first, last) is a heap with respect to \p comp.
     * \note O(log n).
     */
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void popHeap(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        using Distance = typename IteratorTraits<RandomIt>::DifferenceType;
        using Value    = typename IteratorTraits<RandomIt>::ValueType;
        if (last - first < 2)
        {
            return;
        }
        RandomIt const result = last - 1;
        Value value           = worse::core::move(*result);
        *result               = worse::core::move(*first);
        adjustHeap(first, Distance(0), Distance(result - first), worse::core::move(value), comp);
    }

    /**
     * \brief Rearrange [first, last) into a heap with respect to \p comp.
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam RandomIt random-access iterator.
     * \param comp strict-weak-ordering comparator (default `Less<>` => max-heap).
     * \note O(n) via Floyd's bottom-up construction.
     */
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void makeHeap(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        using Distance     = typename IteratorTraits<RandomIt>::DifferenceType;
        using Value        = typename IteratorTraits<RandomIt>::ValueType;
        Distance const len = last - first;
        if (len < 2)
        {
            return;
        }
        for (Distance parent = (len - 2) / 2;; --parent)
        {
            Value value = worse::core::move(first[parent]);
            adjustHeap(first, parent, len, worse::core::move(value), comp);
            if (parent == 0)
            {
                break;
            }
        }
    }

    /**
     * \brief Turn a heap into an ascending sorted range (repeated popHeap).
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam RandomIt random-access iterator.
     * \param comp strict-weak-ordering comparator (default `Less<>` => ascending).
     * \pre [first, last) is a heap with respect to \p comp.
     * \note O(n log n).
     */
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void sortHeap(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        while (last - first > 1)
        {
            popHeap(first, last, comp);
            --last;
        }
    }

    /**
     * \brief First position where the heap property breaks.
     * \param first iterator to the first element of the range.
     * \param last iterator one past the last element of the range.
     * \tparam RandomIt random-access iterator.
     * \param comp strict-weak-ordering comparator (default `Less<>` => max-heap).
     * \return iterator to the first element out of heap order, or \p last if [first, last) is a heap.
     * \note O(n).
     */
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    WE_NODISCARD constexpr RandomIt isHeapUntil(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        using Distance     = typename IteratorTraits<RandomIt>::DifferenceType;
        Distance const len = last - first;
        Distance parent    = 0;
        for (Distance child = 1; child < len; ++child)
        {
            if (comp(first[parent], first[child]))
            {
                return first + child;
            }
            if ((child & 1) == 0)
            {
                ++parent;
            }
        }
        return last;
    }

    /** \brief True if [first, last) is a heap with respect to \p comp. O(n). */
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    WE_NODISCARD constexpr bool isHeap(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        return isHeapUntil(first, last, comp) == last;
    }
    /** @} */
} // namespace worse::core
