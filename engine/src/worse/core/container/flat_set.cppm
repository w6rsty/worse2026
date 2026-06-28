module;

#include "worse/core/macro.hpp"

#include <initializer_list>

export module worse.core.container.flat_set;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.array;
import worse.core.algorithm.binary_search;
import worse.core.algorithm.sort;
import worse.core.algorithm.modifying;

export namespace worse::core::container
{
    /**
     * \brief Flat ordered set: unique keys in one sorted contiguous buffer, found by binary search.
     * \ingroup ctr_ordered
     *
     * The cache-friendly "flat" set (DECISIONS D3): lookups are O(log n) over one contiguous run
     * -- far fewer cache misses than a node-based red-black tree -- at the cost of O(n)
     * insert/erase (a shift in the underlying array, default `Array<Key>`). Ideal for read-mostly
     * sets and small-to-medium sizes.
     * \tparam Key the element/key type.
     * \tparam Compare strict-weak-ordering key comparator.
     * \tparam Container the contiguous backing container.
     * \note Iterators are CONSTANT (they dereference to `Key const&`): mutating a key in place
     *       would silently break the sorted invariant, so the only sanctioned way to change the
     *       contents is insert/erase. Any insert/erase invalidates iterators (the array may shift
     *       or reallocate) -- the same contract as the underlying Array.
     * \note Lookups are templated on the query type so a TRANSPARENT comparator (e.g. `Less<>`)
     *       can probe with a cheaper key type without materializing a full `Key`.
     * \note CONTRACT: `Compare` must be a STRICT WEAK ORDERING. A broken comparator silently
     *       corrupts the sorted invariant (binary search returns wrong slots, `unique`
     *       mis-dedupes) -- not enforced in release (same posture as std).
     */
    template <typename Key, typename Compare = Less<>, typename Container = Array<Key>>
    class FlatSet
    {
    public:
        using KeyType        = Key;
        using ValueType      = Key;
        using KeyCompare     = Compare;
        using ValueCompare   = Compare;
        using ContainerType  = Container;
        using SizeType       = typename Container::SizeType;
        using DifferenceType = typename Container::DifferenceType;
        using ConstReference = Key const&;
        using ConstPointer   = Key const*;

        // Both alias the container's CONST iterator: keys are immutable through the set.
        using Iterator         = typename Container::ConstIterator;
        using ConstIterator    = typename Container::ConstIterator;
        using ReverseIter      = ReverseIterator<ConstIterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        // --- construction ------------------------------------------------------

        FlatSet() = default;

        explicit FlatSet(Compare const& comp) : mComp(comp) {}

        template <typename InIt>
            requires InputIterator<InIt>
        FlatSet(InIt first, InIt last, Compare const& comp = Compare{}) : mComp(comp)
        {
            bulkAppendSortUnique(first, last);
        }

        FlatSet(std::initializer_list<Key> init, Compare const& comp = Compare{}) : mComp(comp)
        {
            bulkAppendSortUnique(init.begin(), init.end());
        }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mData.empty(); }
        WE_NODISCARD SizeType size() const noexcept { return mData.size(); }
        WE_NODISCARD SizeType capacity() const noexcept { return mData.capacity(); }
        void reserve(SizeType n) { mData.reserve(n); }

        // --- iterators (const only) --------------------------------------------

        WE_NODISCARD ConstIterator begin() const noexcept { return mData.begin(); }
        WE_NODISCARD ConstIterator end() const noexcept { return mData.end(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mData.begin(); }
        WE_NODISCARD ConstIterator cend() const noexcept { return mData.end(); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(mData.end()); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(mData.begin()); }

        WE_NODISCARD ConstPointer data() const noexcept { return mData.data(); }

        // --- lookup (transparent: K may differ from Key) -----------------------

        /**
         * \brief Find the element equivalent to \p key.
         * \tparam K query type; with a transparent comparator no temporary `Key` is built.
         * \param key lookup key.
         * \return const iterator to the element, or end() if absent.
         * \note O(log n) binary search.
         */
        template <typename K>
        WE_NODISCARD ConstIterator find(K const& key) const
        {
            SizeType const idx = lowerBoundIndex(key);
            if (idx < mData.size() && !mComp(key, mData[idx]))
            {
                return cbegin() + static_cast<DifferenceType>(idx);
            }
            return cend();
        }

        /**
         * \brief Test whether an element equivalent to \p key is present.
         * \tparam K query type (heterogeneous under a transparent comparator).
         * \param key lookup key.
         * \return true if an equivalent element exists.
         * \note O(log n).
         */
        template <typename K>
        WE_NODISCARD bool contains(K const& key) const
        {
            SizeType const idx = lowerBoundIndex(key);
            return idx < mData.size() && !mComp(key, mData[idx]);
        }

        /**
         * \brief Count elements equivalent to \p key (0 or 1 — keys are unique).
         * \tparam K query type (heterogeneous under a transparent comparator).
         * \param key lookup key.
         * \return 1 if present, else 0.
         */
        template <typename K>
        WE_NODISCARD SizeType count(K const& key) const
        {
            return contains(key) ? SizeType{1} : SizeType{0};
        }

        /**
         * \brief First element not ordered before \p key (the sorted insertion point).
         * \tparam K query type (heterogeneous under a transparent comparator).
         * \param key lookup key.
         * \return const iterator to the lower bound (may be end()).
         * \note O(log n).
         */
        template <typename K>
        WE_NODISCARD ConstIterator lowerBound(K const& key) const
        {
            return cbegin() + static_cast<DifferenceType>(lowerBoundIndex(key));
        }

        /**
         * \brief First element ordered after \p key.
         * \tparam K query type (heterogeneous under a transparent comparator).
         * \param key lookup key.
         * \return const iterator to the upper bound (may be end()).
         * \note O(log n).
         */
        template <typename K>
        WE_NODISCARD ConstIterator upperBound(K const& key) const
        {
            ConstIterator const it = worse::core::upperBound(mData.begin(), mData.end(), key, mComp);
            return it;
        }

        /**
         * \brief The `[lowerBound, upperBound)` range of elements equivalent to \p key.
         * \tparam K query type (heterogeneous under a transparent comparator).
         * \param key lookup key.
         * \return a pair of const iterators bounding the (0-or-1-element) equal range.
         */
        template <typename K>
        WE_NODISCARD Pair<ConstIterator, ConstIterator> equalRange(K const& key) const
        {
            return makePair(lowerBound(key), upperBound(key));
        }

        // --- modifiers ---------------------------------------------------------

        /**
         * \brief Insert \p value if no equivalent key is present.
         * \param value element to copy-insert at its sorted position.
         * \return a (iterator, bool) pair; bool is true if insertion happened, false if the key
         *         already existed (the iterator points at the existing element either way).
         * \note O(n): the underlying array shifts to keep the sorted order.
         */
        Pair<Iterator, bool> insert(Key const& value)
        {
            SizeType const idx = lowerBoundIndex(value);
            if (idx < mData.size() && !mComp(value, mData[idx]))
            {
                return makePair(cbegin() + static_cast<DifferenceType>(idx), false);
            }
            mData.insert(mData.begin() + static_cast<DifferenceType>(idx), value);
            return makePair(cbegin() + static_cast<DifferenceType>(idx), true);
        }

        /** \brief Insert \p value by move if no equivalent key is present; see insert(Key const&). */
        Pair<Iterator, bool> insert(Key&& value)
        {
            SizeType const idx = lowerBoundIndex(value);
            if (idx < mData.size() && !mComp(value, mData[idx]))
            {
                return makePair(cbegin() + static_cast<DifferenceType>(idx), false);
            }
            mData.insert(mData.begin() + static_cast<DifferenceType>(idx), worse::core::move(value));
            return makePair(cbegin() + static_cast<DifferenceType>(idx), true);
        }

        /**
         * \brief Construct a key from \p args and insert it if not already present.
         * \tparam Args constructor argument types for `Key`.
         * \param args arguments forwarded to the `Key` constructor.
         * \return a (iterator, bool) pair as for insert().
         * \note A set element IS its key, so emplace must materialize the key to locate it; it
         *       is then moved in only if not already present.
         */
        template <typename... Args>
        Pair<Iterator, bool> emplace(Args&&... args)
        {
            Key tmp(worse::core::forward<Args>(args)...);
            return insert(worse::core::move(tmp));
        }

        /**
         * \brief Bulk-insert the range [first, last), merging into the sorted-unique invariant.
         * \tparam InIt input iterator type.
         * \param first iterator to the first element of the range.
         * \param last iterator one past the last element of the range.
         * \note One append + sort + unique pass (O(n log n)) — beats n single inserts; for
         *       ForwardIterator+ inputs the buffer is pre-reserved to one growth. (R45)
         */
        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            bulkAppendSortUnique(first, last);
        }

        /**
         * \brief Erase the element equivalent to \p key, if present.
         * \tparam K query type (heterogeneous under a transparent comparator).
         * \param key key to remove.
         * \return the number of elements removed (0 or 1).
         * \note O(n): the array shifts to close the gap.
         */
        template <typename K>
        SizeType erase(K const& key)
        {
            SizeType const idx = lowerBoundIndex(key);
            if (idx < mData.size() && !mComp(key, mData[idx]))
            {
                mData.erase(mData.begin() + static_cast<DifferenceType>(idx));
                return SizeType{1};
            }
            return SizeType{0};
        }

        /**
         * \brief Erase the element at \p pos.
         * \param pos const iterator in [begin(), end()) to the element to remove.
         * \return iterator to the element following the erased one.
         */
        Iterator erase(ConstIterator pos)
        {
            WE_ASSERT(pos >= cbegin() && pos < cend()); // in-range, not end() (R45)
            SizeType const idx = static_cast<SizeType>(pos - cbegin());
            mData.erase(mData.begin() + static_cast<DifferenceType>(idx));
            return cbegin() + static_cast<DifferenceType>(idx);
        }

        /** \brief Remove all elements (capacity is retained). */
        void clear() noexcept { mData.clear(); }

        void swap(FlatSet& other) noexcept
        {
            mData.swap(other.mData);
            worse::core::swap(mComp, other.mComp);
        }

    private:
        Container mData{};
        WE_NO_UNIQUE_ADDRESS Compare mComp{};

        // First index i where !comp(mData[i], key) -- the sorted insertion point for key.
        template <typename K>
        WE_NODISCARD SizeType lowerBoundIndex(K const& key) const
        {
            ConstIterator const first = mData.begin();
            ConstIterator const it    = worse::core::lowerBound(first, mData.end(), key, mComp);
            return static_cast<SizeType>(it - first);
        }

        // Append [first, last), then restore the sorted-unique invariant over the whole
        // buffer with one sort + one unique pass (equivalence = neither key orders before
        // the other). O(n log n) -- the proper flat bulk build, beating n single inserts.
        template <typename InIt>
        void bulkAppendSortUnique(InIt first, InIt last)
        {
            if constexpr (ForwardIterator<InIt>) // multi-pass: pre-size to one growth (R45)
            {
                mData.reserve(mData.size() + static_cast<SizeType>(worse::core::distance(first, last)));
            }
            for (; first != last; ++first)
            {
                mData.emplaceBack(*first);
            }
            worse::core::sort(mData.begin(), mData.end(), mComp);
            auto equiv = [this](Key const& a, Key const& b)
            {
                return !mComp(a, b) && !mComp(b, a);
            };
            auto const newEnd = worse::core::unique(mData.begin(), mData.end(), equiv);
            mData.erase(newEnd, mData.end());
        }
    };

    template <typename Key, typename Compare, typename Container>
    void swap(FlatSet<Key, Compare, Container>& a, FlatSet<Key, Compare, Container>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
