module;

#include "worse/core/macro.hpp"

#include <new> // placement new for the stableSort scratch buffer

export module worse.core.algorithm.sort;
import worse.core.basic_type;
import worse.core.intrinsics;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.memory; // runtime scratch buffer for buffered stableSort (R41)
import worse.core.container.iterator;
import worse.core.algorithm.heap;
import worse.core.algorithm.binary_search; // lowerBound / upperBound (stable in-place merge)
import worse.core.algorithm.modifying;     // rotate (stable in-place merge)

// Comparison sorts over a random-access range, iterator-pair API with an optional
// comparator (default `Less<>` => ascending). The headline `sort` is an INTROSORT:
// median-of-3 quicksort that switches to heapsort once recursion gets too deep (the
// O(n log n) worst-case guard) and leaves sub-threshold runs to a single final
// insertion-sort pass (which is ~O(n) on the mostly-sorted tail introsort produces).
// Also: `partialSort` (heap-based top-k), `nthElement` (introselect), and the
// `isSorted`/`isSortedUntil` predicates. `stableSort` (R41) is a BUFFERED O(n log n) merge
// sort: at runtime it grabs an `n/2` scratch buffer from the default allocator and merges
// with it (std::stable_sort speed); during constant evaluation (or if the allocation fails)
// it falls back to the ALLOCATION-FREE in-place rotation merge (O(n log^2 n), R36). The
// in-place merge stays as that fallback, so the algorithm module still works allocator-free
// where it must (constexpr / freestanding); only the hot runtime path takes the buffer.
//
// Lives in the flat `worse::core` namespace; internal move/swap calls are fully
// qualified to avoid the std:: ADL clash.

// Internal machinery: NOT in the `export` blocks below, so module linkage already hides
// it from importers -- no `_detail` sub-namespace needed (matches the allocator_traits
// convention of non-exported helpers living in the main namespace).
namespace worse::core
{
    // Sub-ranges this size or smaller are left for the final insertion-sort pass.
    inline constexpr isize kInsertionThreshold = 16;

    template <typename Size>
    constexpr Size log2Floor(Size n)
    {
        Size k = 0;
        while (n > 1)
        {
            n >>= 1;
            ++k;
        }
        return k;
    }

    // Guarded insertion sort over the whole range (correct for any size; near-linear
    // when the range is already mostly sorted, which is the introsort tail case).
    template <typename RandomIt, typename Compare>
    constexpr void insertionSortImpl(RandomIt first, RandomIt last, Compare& comp)
    {
        using Value = typename IteratorTraits<RandomIt>::ValueType;
        if (first == last)
        {
            return;
        }
        for (RandomIt i = first + 1; i != last; ++i)
        {
            Value value = worse::core::move(*i);
            RandomIt j  = i;
            while (j != first && comp(value, *(j - 1)))
            {
                *j = worse::core::move(*(j - 1));
                --j;
            }
            *j = worse::core::move(value);
        }
    }

    // Merge two consecutive sorted runs [first,mid) and [mid,last) IN PLACE, no scratch
    // buffer: recursively split the larger run, binary-search the matching cut in the other,
    // `rotate` the middle block so the two halves line up, then recurse on the two sub-merges.
    // STABLE -- lowerBound/upperBound place equal elements so the left run stays before the
    // right (left element's slot via lowerBound, right element's slot via upperBound). The
    // classic SGI __merge_without_buffer; O(n log n) rotates => O(n log^2 n) overall.
    template <typename RandomIt, typename Compare>
    constexpr void inplaceMergeImpl(RandomIt first, RandomIt mid, RandomIt last, Compare& comp)
    {
        if (first == mid || mid == last)
        {
            return;
        }
        isize const len1 = mid - first;
        isize const len2 = last - mid;
        if (len1 + len2 == 2)
        {
            if (comp(*mid, *first))
            {
                worse::core::swap(*first, *mid);
            }
            return;
        }
        RandomIt firstCut  = first;
        RandomIt secondCut = mid;
        if (len1 > len2)
        {
            firstCut += len1 / 2;
            secondCut = lowerBound(mid, last, *firstCut, comp);
        }
        else
        {
            secondCut += len2 / 2;
            firstCut = upperBound(first, mid, *secondCut, comp);
        }
        rotate(firstCut, mid, secondCut);
        RandomIt const newMid = firstCut + (secondCut - mid);
        inplaceMergeImpl(first, firstCut, newMid, comp);
        inplaceMergeImpl(newMid, secondCut, last, comp);
    }

    // Recursive stable merge sort; small runs go to the (stable) insertion sort.
    template <typename RandomIt, typename Compare>
    constexpr void stableSortImpl(RandomIt first, RandomIt last, Compare& comp)
    {
        isize const n = last - first;
        if (n <= kInsertionThreshold)
        {
            insertionSortImpl(first, last, comp);
            return;
        }
        RandomIt const mid = first + n / 2;
        stableSortImpl(first, mid, comp);
        stableSortImpl(mid, last, comp);
        inplaceMergeImpl(first, mid, last, comp);
    }

    // Buffered stable merge of two consecutive sorted runs [first,mid) and [mid,last).
    // Moves the LEFT run into `buf` (raw uninitialized storage, capacity >= mid-first), then
    // merges it back with the in-place right run. Buffering only the left half is enough: the
    // output cursor `k` never overtakes the right cursor `j` (k advances once per element, j
    // only when a right element is taken, so k-first <= j-mid+#left-consumed <= mid), so an
    // unconsumed right element is never overwritten and its tail is already in position.
    // STABLE: take the right element only when it is STRICTLY less than the left, so equal
    // keys keep left-before-right. Not constexpr (placement new / raw buffer); runtime only.
    template <typename RandomIt, typename Compare>
    void bufferedMergeImpl(RandomIt first, RandomIt mid, RandomIt last, Compare& comp,
                           typename IteratorTraits<RandomIt>::ValueType* buf)
    {
        using Value   = typename IteratorTraits<RandomIt>::ValueType;
        Value* bufEnd = buf;
        for (RandomIt p = first; p != mid; ++p, ++bufEnd)
        {
            ::new (static_cast<void*>(bufEnd)) Value(worse::core::move(*p));
        }
        Value* i   = buf;   // left cursor (in scratch)
        RandomIt j = mid;   // right cursor (in place)
        RandomIt k = first; // output cursor (in place)
        while (i != bufEnd && j != last)
        {
            if (comp(*j, *i))
            {
                *k = worse::core::move(*j);
                ++j;
            }
            else
            {
                *k = worse::core::move(*i);
                i->~Value();
                ++i;
            }
            ++k;
        }
        // Drain the remaining left scratch; any right tail already sits in place at [k,last).
        while (i != bufEnd)
        {
            *k = worse::core::move(*i);
            i->~Value();
            ++i;
            ++k;
        }
    }

    // Top-down stable merge sort using a shared scratch buffer (capacity >= (n+1)/2). Small
    // runs go to the stable insertion sort; merges route through bufferedMergeImpl. The buffer
    // only ever needs to hold one run's left half (the top merge's, (n+1)/2), reused across the
    // sequential recursion. Runtime only.
    template <typename RandomIt, typename Compare>
    void stableSortBufferedImpl(RandomIt first, RandomIt last, Compare& comp,
                                typename IteratorTraits<RandomIt>::ValueType* buf)
    {
        isize const n = last - first;
        if (n <= kInsertionThreshold)
        {
            insertionSortImpl(first, last, comp);
            return;
        }
        RandomIt const mid = first + n / 2;
        stableSortBufferedImpl(first, mid, comp, buf);
        stableSortBufferedImpl(mid, last, comp, buf);
        bufferedMergeImpl(first, mid, last, comp, buf);
    }

    // Pick the median of *a, *b, *c and swap it into *result (pivot selection).
    template <typename RandomIt, typename Compare>
    constexpr void moveMedianToFirst(RandomIt result, RandomIt a, RandomIt b, RandomIt c, Compare& comp)
    {
        if (comp(*a, *b))
        {
            if (comp(*b, *c))
            {
                worse::core::swap(*result, *b);
            }
            else if (comp(*a, *c))
            {
                worse::core::swap(*result, *c);
            }
            else
            {
                worse::core::swap(*result, *a);
            }
        }
        else if (comp(*a, *c))
        {
            worse::core::swap(*result, *a);
        }
        else if (comp(*b, *c))
        {
            worse::core::swap(*result, *c);
        }
        else
        {
            worse::core::swap(*result, *b);
        }
    }

    // Hoare partition around the pivot at *pivot (which is NOT min/max of the range,
    // thanks to median-of-3, so the inner scans need no bounds guard).
    template <typename RandomIt, typename Compare>
    constexpr RandomIt unguardedPartition(RandomIt first, RandomIt last, RandomIt pivot, Compare& comp)
    {
        while (true)
        {
            while (comp(*first, *pivot))
            {
                ++first;
            }
            --last;
            while (comp(*pivot, *last))
            {
                --last;
            }
            if (!(first < last))
            {
                return first;
            }
            worse::core::swap(*first, *last);
            ++first;
        }
    }

    template <typename RandomIt, typename Compare>
    constexpr RandomIt unguardedPartitionPivot(RandomIt first, RandomIt last, Compare& comp)
    {
        RandomIt const mid = first + (last - first) / 2;
        moveMedianToFirst(first, first + 1, mid, last - 1, comp);
        return unguardedPartition(first + 1, last, first, comp);
    }

    // Branchless Lomuto partition for cheap-to-swap (trivially-copyable) types -- the lever that
    // matches libc++'s std::sort on random data. Median-of-3 pivot is moved out, then every
    // element is UNCONDITIONALLY swapped while the store cursor advances by a BRANCHLESS
    // `store += (element < pivot)`: no data-dependent branch in the hot scan, so random inputs
    // pay no ~50% misprediction penalty (the win) at the cost of more swaps than Hoare (cheap
    // for trivially-copyable). Returns the pivot's FINAL position; [first,p) < pivot <= [p,last).
    // introsortLoop recurses both sides EXCLUDING p, which guarantees progress even when the
    // pivot lands at an end (so no median-of-3 interior-sentinel assumption is needed here).
    template <typename RandomIt, typename Compare>
    constexpr RandomIt partitionPivotBranchless(RandomIt first, RandomIt last, Compare& comp)
    {
        using Value        = typename IteratorTraits<RandomIt>::ValueType;
        RandomIt const mid = first + (last - first) / 2;
        moveMedianToFirst(first, first + 1, mid, last - 1, comp);
        Value pivot    = worse::core::move(*first); // hold the pivot out of the array
        RandomIt store = first + 1;
        for (RandomIt it = first + 1; it != last; ++it)
        {
            bool const smaller = comp(*it, pivot);
            worse::core::swap(*store, *it);
            store += static_cast<isize>(smaller); // branchless advance
        }
        RandomIt const pivotPos = store - 1; // [first+1,store) < pivot; [store,last) >= pivot
        *first                  = worse::core::move(*pivotPos);
        *pivotPos               = worse::core::move(pivot); // drop the pivot into its sorted slot
        return pivotPos;
    }

    // Sift the root of a max-heap [first, first+len) down to its correct position.
    // Used by partialSort (heap-compatible with the heap module's make/sortHeap).
    template <typename RandomIt, typename Distance, typename Compare>
    constexpr void siftDownRoot(RandomIt first, Distance len, Compare& comp)
    {
        using Value   = typename IteratorTraits<RandomIt>::ValueType;
        Value top     = worse::core::move(first[0]);
        Distance hole = 0;
        Distance child;
        while ((child = 2 * hole + 1) < len)
        {
            if (child + 1 < len && comp(first[child], first[child + 1]))
            {
                ++child;
            }
            if (!comp(top, first[child]))
            {
                break;
            }
            first[hole] = worse::core::move(first[child]);
            hole        = child;
        }
        first[hole] = worse::core::move(top);
    }

    // Quicksort with a recursion-depth budget; on exhaustion it heapsorts the
    // sub-range (guaranteeing O(n log n) overall). Sub-threshold runs are left sorted
    // by the caller's final insertion pass.
    template <typename RandomIt, typename Size, typename Compare>
    constexpr void introsortLoop(RandomIt first, RandomIt last, Size depthLimit, Compare& comp)
    {
        using Value = typename IteratorTraits<RandomIt>::ValueType;
        while (last - first > kInsertionThreshold)
        {
            if (depthLimit == 0)
            {
                makeHeap(first, last, comp);
                sortHeap(first, last, comp);
                return;
            }
            --depthLimit;
            if constexpr (IsTriviallyCopyable<Value>)
            {
                // Branchless Lomuto: pivot placed at p, recurse both sides EXCLUDING it.
                RandomIt const p = partitionPivotBranchless(first, last, comp);
                introsortLoop(p + 1, last, depthLimit, comp);
                last = p;
            }
            else
            {
                // Hoare: pivot stays at the boundary, sides share the cut (cheaper for
                // expensive-to-swap types, which branchless Lomuto would over-swap).
                RandomIt const cut = unguardedPartitionPivot(first, last, comp);
                introsortLoop(cut, last, depthLimit, comp);
                last = cut;
            }
        }
    }
} // namespace worse::core

export namespace worse::core
{
    // Standalone insertion sort (small ranges / nearly-sorted data).
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void insertionSort(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        insertionSortImpl(first, last, comp);
    }

    // Introsort. O(n log n) worst case, in place, not stable.
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void sort(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        using Distance = typename IteratorTraits<RandomIt>::DifferenceType;
        if (first != last)
        {
            Distance const n = last - first;
            introsortLoop(first, last, Distance(2 * log2Floor(n)), comp);
            insertionSortImpl(first, last, comp);
        }
    }

    // Stable sort: preserves the relative order of equal elements. At runtime it grabs an
    // (n+1)/2 scratch buffer from the default allocator and does a buffered O(n log n) merge
    // (std::stable_sort speed, R41); during constant evaluation -- or if the allocation fails
    // -- it falls back to the allocation-free in-place rotation merge (O(n log^2 n), R36). Use
    // `sort` when stability is not needed (introsort, no allocation).
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void stableSort(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        using Value   = typename IteratorTraits<RandomIt>::ValueType;
        isize const n = last - first;
        if (n <= kInsertionThreshold)
        {
            insertionSortImpl(first, last, comp); // tiny ranges: no buffer worth allocating
            return;
        }
        if (!intrinsics::isConstantEvaluated())
        {
            usize const bufCount = static_cast<usize>((n + 1) / 2);
            void* raw            = memory::allocate(bufCount * sizeof(Value), alignof(Value), memory::AllocInfo{});
            if (raw != nullptr) [[likely]]
            {
                stableSortBufferedImpl(first, last, comp, static_cast<Value*>(raw));
                memory::deallocate(raw, bufCount * sizeof(Value), alignof(Value));
                return;
            }
        }
        stableSortImpl(first, last, comp); // constant-evaluated or allocation failed
    }

    // Reorder so [first, middle) holds the (middle-first) smallest elements sorted;
    // the rest are left in unspecified order. Heap-based, O(n log k).
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void partialSort(RandomIt first, RandomIt middle, RandomIt last, Compare comp = Compare{})
    {
        using Distance = typename IteratorTraits<RandomIt>::DifferenceType;
        if (first == middle)
        {
            return;
        }
        makeHeap(first, middle, comp);
        Distance const len = middle - first;
        for (RandomIt i = middle; i < last; ++i)
        {
            if (comp(*i, *first)) // smaller than the current max of the kept set
            {
                worse::core::swap(*i, *first);
                siftDownRoot(first, len, comp);
            }
        }
        sortHeap(first, middle, comp);
    }

    // First position where order breaks (== last if the whole range is sorted).
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    WE_NODISCARD constexpr RandomIt isSortedUntil(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        if (first == last)
        {
            return last;
        }
        RandomIt next = first;
        for (++next; next != last; ++next, ++first)
        {
            if (comp(*next, *first))
            {
                return next;
            }
        }
        return last;
    }

    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    WE_NODISCARD constexpr bool isSorted(RandomIt first, RandomIt last, Compare comp = Compare{})
    {
        return isSortedUntil(first, last, comp) == last;
    }
} // namespace worse::core

// introselect needs partialSort/insertionSortImpl/partition, all declared above; like the
// machinery above it is non-exported (placed after the export block only for ordering).
namespace worse::core
{
    template <typename RandomIt, typename Size, typename Compare>
    constexpr void introselectLoop(RandomIt first, RandomIt nth, RandomIt last, Size depthLimit, Compare& comp)
    {
        while (last - first > 3)
        {
            if (depthLimit == 0)
            {
                // Worst-case fallback: partial-sort enough to fix nth's position.
                partialSort(first, nth + 1, last, comp);
                return;
            }
            --depthLimit;
            RandomIt const cut = unguardedPartitionPivot(first, last, comp);
            if (cut <= nth)
            {
                first = cut;
            }
            else
            {
                last = cut;
            }
        }
        insertionSortImpl(first, last, comp);
    }
} // namespace worse::core

export namespace worse::core
{
    // Place the element that would be at `nth` in the fully sorted range at `nth`,
    // with everything before it <= and everything after >= (introselect, avg O(n)).
    template <typename RandomIt, typename Compare = Less<>>
        requires RandomAccessIterator<RandomIt>
    constexpr void nthElement(RandomIt first, RandomIt nth, RandomIt last, Compare comp = Compare{})
    {
        using Distance = typename IteratorTraits<RandomIt>::DifferenceType;
        if (first == last || nth == last)
        {
            return;
        }
        introselectLoop(first, nth, last, Distance(2 * log2Floor(last - first)), comp);
    }
} // namespace worse::core
