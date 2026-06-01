module;

#include "worse/core/macro.hpp"

#include <initializer_list>

export module worse.core.container.flat_map;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.array;
import worse.core.algorithm.binary_search;
import worse.core.algorithm.sort;
import worse.core.algorithm.modifying;

// Non-exported helper: adapts a KEY comparator (orders `Key`s) into a comparator over
// the stored `Pair<Key, T>` elements. Provides the three forms the algorithm layer
// needs -- element vs element (sort), element vs key (lowerBound's `comp(*mid, value)`),
// key vs element (upperBound's `comp(value, *mid)`) -- all keyed on `.first`. The
// heterogeneous overloads are disabled when the query type IS the element type so the
// non-template element/element overload wins unambiguously.
//
// Per DECISIONS: helpers live in the module's main namespace, simply left out of the
// `export` block (module linkage hides them) -- no `_detail` sub-namespace.
namespace worse::core::container
{
    template <typename Key, typename T, typename Compare>
    struct FlatMapValueCompare
    {
        using ValueType = Pair<Key, T>;
        WE_NO_UNIQUE_ADDRESS Compare comp{};

        WE_NODISCARD constexpr bool operator()(ValueType const& a, ValueType const& b) const
        {
            return comp(a.first, b.first);
        }
        template <typename K>
            requires(!IsSame<RemoveCvRef<K>, ValueType>)
        WE_NODISCARD constexpr bool operator()(ValueType const& a, K const& k) const
        {
            return comp(a.first, k);
        }
        template <typename K>
            requires(!IsSame<RemoveCvRef<K>, ValueType>)
        WE_NODISCARD constexpr bool operator()(K const& k, ValueType const& b) const
        {
            return comp(k, b.first);
        }
    };
} // namespace worse::core::container

// Flat ordered map: key/value pairs held in one sorted, contiguous buffer (default
// `Array<Pair<Key, T>>`) ordered by key, located by binary search (DECISIONS D3). Same
// trade-off as FlatSet: O(log n) cache-friendly lookup, O(n) insert/erase.
//
// Unlike std::map's `pair<const Key, T>`, the stored key is NON-const: a contiguous
// container must move/relocate elements on insert/erase, which a const key would forbid.
// The contract is therefore: iterators are mutable, but you may modify only `.second`
// (the mapped value). Mutating `.first` (the key) silently breaks the sorted invariant
// -- change a key only via erase + insert. Any insert/erase invalidates iterators.
//
// Lookups are templated on the query type, so a TRANSPARENT key comparator (e.g.
// `Less<>`) can probe with a cheaper key type without building a full `Key`.
export namespace worse::core::container
{
    template <typename Key, typename T, typename Compare = Less<>, typename Container = Array<Pair<Key, T>>>
    class FlatMap
    {
    public:
        using KeyType        = Key;
        using MappedType     = T;
        using ValueType      = Pair<Key, T>;
        using KeyCompare     = Compare;
        using ContainerType  = Container;
        using SizeType       = typename Container::SizeType;
        using DifferenceType = typename Container::DifferenceType;
        using Reference      = ValueType&;
        using ConstReference = ValueType const&;
        using Pointer        = ValueType*;
        using ConstPointer   = ValueType const*;

        using Iterator         = typename Container::Iterator;
        using ConstIterator    = typename Container::ConstIterator;
        using ReverseIter      = ReverseIterator<Iterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        // --- construction ------------------------------------------------------

        FlatMap() = default;

        explicit FlatMap(Compare const& comp) : mComp(comp) {}

        template <typename InIt>
            requires InputIterator<InIt>
        FlatMap(InIt first, InIt last, Compare const& comp = Compare{}) : mComp(comp)
        {
            bulkAppendSortUnique(first, last);
        }

        FlatMap(std::initializer_list<ValueType> init, Compare const& comp = Compare{}) : mComp(comp)
        {
            bulkAppendSortUnique(init.begin(), init.end());
        }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mData.empty(); }
        WE_NODISCARD SizeType size() const noexcept { return mData.size(); }
        WE_NODISCARD SizeType capacity() const noexcept { return mData.capacity(); }
        void reserve(SizeType n) { mData.reserve(n); }

        // --- iterators ---------------------------------------------------------
        //
        // Mutable, but only `.second` may be changed -- see the class contract.

        WE_NODISCARD Iterator begin() noexcept { return mData.begin(); }
        WE_NODISCARD ConstIterator begin() const noexcept { return mData.begin(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mData.begin(); }
        WE_NODISCARD Iterator end() noexcept { return mData.end(); }
        WE_NODISCARD ConstIterator end() const noexcept { return mData.end(); }
        WE_NODISCARD ConstIterator cend() const noexcept { return mData.end(); }
        WE_NODISCARD ReverseIter rbegin() noexcept { return ReverseIter(mData.end()); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(mData.end()); }
        WE_NODISCARD ReverseIter rend() noexcept { return ReverseIter(mData.begin()); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(mData.begin()); }

        WE_NODISCARD Pointer data() noexcept { return mData.data(); }
        WE_NODISCARD ConstPointer data() const noexcept { return mData.data(); }

        // --- lookup (transparent: K may differ from Key) -----------------------

        template <typename K>
        WE_NODISCARD Iterator find(K const& key)
        {
            SizeType const idx = lowerBoundIndex(key);
            return equivalentAt(idx, key) ? mData.begin() + static_cast<DifferenceType>(idx) : mData.end();
        }
        template <typename K>
        WE_NODISCARD ConstIterator find(K const& key) const
        {
            SizeType const idx = lowerBoundIndex(key);
            return equivalentAt(idx, key) ? mData.begin() + static_cast<DifferenceType>(idx) : mData.end();
        }

        template <typename K>
        WE_NODISCARD bool contains(K const& key) const
        {
            return equivalentAt(lowerBoundIndex(key), key);
        }

        template <typename K>
        WE_NODISCARD SizeType count(K const& key) const
        {
            return contains(key) ? SizeType{1} : SizeType{0};
        }

        template <typename K>
        WE_NODISCARD Iterator lowerBound(K const& key)
        {
            return mData.begin() + static_cast<DifferenceType>(lowerBoundIndex(key));
        }
        template <typename K>
        WE_NODISCARD ConstIterator lowerBound(K const& key) const
        {
            return mData.begin() + static_cast<DifferenceType>(lowerBoundIndex(key));
        }

        template <typename K>
        WE_NODISCARD Iterator upperBound(K const& key)
        {
            return mData.begin() + static_cast<DifferenceType>(upperBoundIndex(key));
        }
        template <typename K>
        WE_NODISCARD ConstIterator upperBound(K const& key) const
        {
            return mData.begin() + static_cast<DifferenceType>(upperBoundIndex(key));
        }

        template <typename K>
        WE_NODISCARD Pair<Iterator, Iterator> equalRange(K const& key)
        {
            return makePair(lowerBound(key), upperBound(key));
        }
        template <typename K>
        WE_NODISCARD Pair<ConstIterator, ConstIterator> equalRange(K const& key) const
        {
            return makePair(lowerBound(key), upperBound(key));
        }

        // --- element access ----------------------------------------------------

        // Return the mapped value for `key`, inserting a value-initialized one if absent.
        // Not nodiscard: `m[k];` to default-insert is an idiom (matches std::map).
        T& operator[](Key const& key)
        {
            SizeType const idx = lowerBoundIndex(key);
            if (equivalentAt(idx, key))
            {
                return mData[idx].second;
            }
            mData.emplace(mData.begin() + static_cast<DifferenceType>(idx), key, T{});
            return mData[idx].second;
        }

        T& operator[](Key&& key)
        {
            SizeType const idx = lowerBoundIndex(key);
            if (equivalentAt(idx, key))
            {
                return mData[idx].second;
            }
            mData.emplace(mData.begin() + static_cast<DifferenceType>(idx), worse::core::move(key), T{});
            return mData[idx].second;
        }

        // Access an existing mapped value. UB if the key is absent (debug WE_ASSERT).
        WE_NODISCARD T& at(Key const& key) noexcept
        {
            SizeType const idx = lowerBoundIndex(key);
            WE_ASSERT(equivalentAt(idx, key));
            return mData[idx].second;
        }
        WE_NODISCARD T const& at(Key const& key) const noexcept
        {
            SizeType const idx = lowerBoundIndex(key);
            WE_ASSERT(equivalentAt(idx, key));
            return mData[idx].second;
        }

        // --- modifiers ---------------------------------------------------------

        Pair<Iterator, bool> insert(ValueType const& value)
        {
            SizeType const idx = lowerBoundIndex(value.first);
            if (equivalentAt(idx, value.first))
            {
                return makePair(mData.begin() + static_cast<DifferenceType>(idx), false);
            }
            mData.insert(mData.begin() + static_cast<DifferenceType>(idx), value);
            return makePair(mData.begin() + static_cast<DifferenceType>(idx), true);
        }

        Pair<Iterator, bool> insert(ValueType&& value)
        {
            SizeType const idx = lowerBoundIndex(value.first);
            if (equivalentAt(idx, value.first))
            {
                return makePair(mData.begin() + static_cast<DifferenceType>(idx), false);
            }
            mData.insert(mData.begin() + static_cast<DifferenceType>(idx), worse::core::move(value));
            return makePair(mData.begin() + static_cast<DifferenceType>(idx), true);
        }

        // Insert {key, mapped} if absent, else overwrite the existing mapped value.
        template <typename M>
        Pair<Iterator, bool> insertOrAssign(Key const& key, M&& mapped)
        {
            SizeType const idx = lowerBoundIndex(key);
            if (equivalentAt(idx, key))
            {
                mData[idx].second = worse::core::forward<M>(mapped);
                return makePair(mData.begin() + static_cast<DifferenceType>(idx), false);
            }
            mData.emplace(mData.begin() + static_cast<DifferenceType>(idx), key, worse::core::forward<M>(mapped));
            return makePair(mData.begin() + static_cast<DifferenceType>(idx), true);
        }

        // Insert {key, T(args...)} only if the key is absent. The mapped value is built
        // ONLY when inserting (the win over operator[] / insert for expensive T).
        template <typename... Args>
        Pair<Iterator, bool> tryEmplace(Key const& key, Args&&... args)
        {
            SizeType const idx = lowerBoundIndex(key);
            if (equivalentAt(idx, key))
            {
                return makePair(mData.begin() + static_cast<DifferenceType>(idx), false);
            }
            mData.emplace(mData.begin() + static_cast<DifferenceType>(idx), key, T(worse::core::forward<Args>(args)...));
            return makePair(mData.begin() + static_cast<DifferenceType>(idx), true);
        }

        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            bulkAppendSortUnique(first, last);
        }

        // Constrained off the iterator types so `erase(iterator)` resolves to the
        // overload below (an Iterator is an exact match for this template but only a
        // qualification-converted match for `erase(ConstIterator)`, which would
        // otherwise lose overload resolution).
        template <typename K>
            requires(!IsSame<RemoveCvRef<K>, Iterator> && !IsSame<RemoveCvRef<K>, ConstIterator>)
        SizeType erase(K const& key)
        {
            SizeType const idx = lowerBoundIndex(key);
            if (equivalentAt(idx, key))
            {
                mData.erase(mData.begin() + static_cast<DifferenceType>(idx));
                return SizeType{1};
            }
            return SizeType{0};
        }

        Iterator erase(ConstIterator pos)
        {
            SizeType const idx = static_cast<SizeType>(pos - cbegin());
            mData.erase(mData.begin() + static_cast<DifferenceType>(idx));
            return mData.begin() + static_cast<DifferenceType>(idx);
        }

        void clear() noexcept { mData.clear(); }

        void swap(FlatMap& other) noexcept
        {
            mData.swap(other.mData);
            worse::core::swap(mComp, other.mComp);
        }

    private:
        Container mData{};
        WE_NO_UNIQUE_ADDRESS Compare mComp{};

        WE_NODISCARD FlatMapValueCompare<Key, T, Compare> valueComp() const { return {mComp}; }

        // First index i where !comp(mData[i].first, key) -- the sorted insertion point.
        template <typename K>
        WE_NODISCARD SizeType lowerBoundIndex(K const& key) const
        {
            ConstIterator const first = mData.begin();
            ConstIterator const it    = worse::core::lowerBound(first, mData.end(), key, valueComp());
            return static_cast<SizeType>(it - first);
        }

        template <typename K>
        WE_NODISCARD SizeType upperBoundIndex(K const& key) const
        {
            ConstIterator const first = mData.begin();
            ConstIterator const it    = worse::core::upperBound(first, mData.end(), key, valueComp());
            return static_cast<SizeType>(it - first);
        }

        // Is the element at `idx` (a lowerBound result) actually equivalent to `key`?
        // lowerBound guarantees !comp(elem, key); equivalence needs also !comp(key, elem).
        template <typename K>
        WE_NODISCARD bool equivalentAt(SizeType idx, K const& key) const
        {
            return idx < mData.size() && !mComp(key, mData[idx].first);
        }

        // Append [first, last), then restore the sorted-unique-by-key invariant with one
        // sort + one unique pass. O(n log n) -- the proper flat bulk build.
        template <typename InIt>
        void bulkAppendSortUnique(InIt first, InIt last)
        {
            for (; first != last; ++first)
            {
                mData.emplaceBack(*first);
            }
            worse::core::sort(mData.begin(), mData.end(), valueComp());
            auto equiv = [this](ValueType const& a, ValueType const& b)
            {
                return !mComp(a.first, b.first) && !mComp(b.first, a.first);
            };
            auto const newEnd = worse::core::unique(mData.begin(), mData.end(), equiv);
            mData.erase(newEnd, mData.end());
        }
    };

    template <typename Key, typename T, typename Compare, typename Container>
    void swap(FlatMap<Key, T, Compare, Container>& a, FlatMap<Key, T, Compare, Container>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
