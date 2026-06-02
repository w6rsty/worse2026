module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <initializer_list>

export module worse.core.container.map;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.iterator;
import worse.core.container.rb_tree;

// Ordered unique map: a thin adapter over the red-black `RBTree` engine keyed by `.first`, the
// ordered counterpart to `UnorderedMap`. O(log n) insert/find/erase, sorted iteration, stable
// element addresses. Iterators are mutable but ONLY `.second` may change -- mutating a key
// breaks the tree ordering (documented contract, not enforced; change a key via erase+insert),
// the same rule as `FlatMap`/`UnorderedMap` (R16). GAME-PERF: prefer `FlatMap`/`UnorderedMap`;
// use `Map` only for ordered iteration with stable refs + O(log n) mutation.
export namespace worse::core::container
{
    template <typename Key, typename T>
    struct MapKeyOfValue
    {
        WE_NODISCARD constexpr Key const& operator()(Pair<Key, T> const& value) const noexcept { return value.first; }
    };

    // Default Compare is the TRANSPARENT `Less<>` (like FlatMap) so heterogeneous lookup works
    // out of the box (find/count/lowerBound by a view/projection with no temporary Key). (R45)
    template <typename Key, typename T, typename Compare = Less<>, typename Allocator = WE_DEFAULT_ALLOCATOR>
    class Map
    {
        using Table = RBTree<Pair<Key, T>, Key, MapKeyOfValue<Key, T>, Compare, Allocator>;

    public:
        using KeyType        = Key;
        using MappedType     = T;
        using ValueType      = Pair<Key, T>;
        using CompareType    = Compare;
        using AllocatorType  = Allocator;
        using SizeType       = typename Table::SizeType;
        using Reference      = ValueType&;
        using ConstReference = ValueType const&;

        // Mutable iterators -- but only `.second` may change (see the class contract).
        using Iterator         = typename Table::Iterator;
        using ConstIterator    = typename Table::ConstIterator;
        using ReverseIter      = ReverseIterator<Iterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        // --- construction ------------------------------------------------------

        Map() = default;
        explicit Map(AllocatorType const& allocator) : mTree(allocator) {}
        explicit Map(Compare const& comp, AllocatorType const& allocator = AllocatorType{}) : mTree(comp, allocator) {}

        template <typename InIt>
            requires InputIterator<InIt>
        Map(InIt first, InIt last)
        {
            mTree.insert(first, last);
        }

        Map(std::initializer_list<ValueType> init) { mTree.insert(init.begin(), init.end()); }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mTree.empty(); }
        WE_NODISCARD SizeType size() const noexcept { return mTree.size(); }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return mTree.begin(); }
        WE_NODISCARD ConstIterator begin() const noexcept { return mTree.begin(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mTree.cbegin(); }
        WE_NODISCARD Iterator end() noexcept { return mTree.end(); }
        WE_NODISCARD ConstIterator end() const noexcept { return mTree.end(); }
        WE_NODISCARD ConstIterator cend() const noexcept { return mTree.cend(); }
        WE_NODISCARD ReverseIter rbegin() noexcept { return ReverseIter(end()); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(cend()); }
        WE_NODISCARD ReverseIter rend() noexcept { return ReverseIter(begin()); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(cbegin()); }

        // --- lookup ------------------------------------------------------------

        // Templated on the query type for transparent (heterogeneous) lookup; see RBTree (R45).
        // operator[]/at/insertOrAssign/tryEmplace stay Key-typed (they construct with the key).
        template <typename K>
        WE_NODISCARD Iterator find(K const& key) noexcept { return mTree.find(key); }
        template <typename K>
        WE_NODISCARD ConstIterator find(K const& key) const noexcept { return mTree.find(key); }
        template <typename K>
        WE_NODISCARD bool contains(K const& key) const noexcept { return mTree.contains(key); }
        template <typename K>
        WE_NODISCARD SizeType count(K const& key) const noexcept { return mTree.count(key); }
        template <typename K>
        WE_NODISCARD Iterator lowerBound(K const& key) noexcept { return mTree.lowerBound(key); }
        template <typename K>
        WE_NODISCARD ConstIterator lowerBound(K const& key) const noexcept { return mTree.lowerBound(key); }
        template <typename K>
        WE_NODISCARD Iterator upperBound(K const& key) noexcept { return mTree.upperBound(key); }
        template <typename K>
        WE_NODISCARD ConstIterator upperBound(K const& key) const noexcept { return mTree.upperBound(key); }
        template <typename K>
        WE_NODISCARD Pair<Iterator, Iterator> equalRange(K const& key) noexcept
        {
            return {mTree.lowerBound(key), mTree.upperBound(key)};
        }

        // --- element access ----------------------------------------------------

        // Return the mapped value for `key`, inserting a value-initialized one if absent.
        // Single tree descent (R45): findOrInsertWith locates the slot once and only constructs
        // the value when the key is absent.
        T& operator[](Key const& key)
        {
            return mTree.findOrInsertWith(key, [&]
                                          { return ValueType(key, T{}); })
                .first->second;
        }
        T& operator[](Key&& key)
        {
            return mTree.findOrInsertWith(key, [&]
                                          { return ValueType(worse::core::move(key), T{}); })
                .first->second;
        }

        WE_NODISCARD T& at(Key const& key) noexcept
        {
            Iterator it = mTree.find(key);
            WE_ASSERT(it != mTree.end());
            return it->second;
        }
        WE_NODISCARD T const& at(Key const& key) const noexcept
        {
            ConstIterator it = mTree.find(key);
            WE_ASSERT(it != mTree.end());
            return it->second;
        }

        // --- modifiers ---------------------------------------------------------

        Pair<Iterator, bool> insert(ValueType const& value) { return mTree.insertUnique(value); }
        Pair<Iterator, bool> insert(ValueType&& value) { return mTree.insertUnique(worse::core::move(value)); }

        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            mTree.insert(first, last);
        }

        // Single descent (R45): the lambda runs only on insert; on the found path `mapped` is
        // forwarded into the assignment instead. Exactly one of the two forwards executes.
        template <typename M>
        Pair<Iterator, bool> insertOrAssign(Key const& key, M&& mapped)
        {
            Pair<Iterator, bool> r =
                mTree.findOrInsertWith(key, [&]
                                       { return ValueType(key, worse::core::forward<M>(mapped)); });
            if (!r.second)
            {
                r.first->second = worse::core::forward<M>(mapped);
            }
            return r;
        }

        // Insert {key, T(args...)} only if the key is absent; builds the mapped value ONLY when
        // inserting (the win over operator[]/insert for an expensive T), in ONE tree descent (R45).
        template <typename... Args>
        Pair<Iterator, bool> tryEmplace(Key const& key, Args&&... args)
        {
            return mTree.findOrInsertWith(key, [&]
                                          { return ValueType(key, T(worse::core::forward<Args>(args)...)); });
        }

        SizeType erase(Key const& key) { return mTree.erase(key); } // see RBTree: erase stays Key-typed
        Iterator erase(ConstIterator pos) { return mTree.erase(pos); }

        void clear() noexcept { mTree.clear(); }
        void swap(Map& other) noexcept(noexcept(mTree.swap(other.mTree))) { mTree.swap(other.mTree); }

    private:
        Table mTree{};
    };

    template <typename Key, typename T, typename Compare, typename Allocator>
    void swap(Map<Key, T, Compare, Allocator>& a, Map<Key, T, Compare, Allocator>& b) noexcept(noexcept(a.swap(b)))
    {
        a.swap(b);
    }
} // namespace worse::core::container
