module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <initializer_list>

export module worse.core.container.set;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.iterator;
import worse.core.container.rb_tree;

// Ordered unique set: a thin adapter over the red-black `RBTree` engine (identity key
// extractor), the ordered counterpart to `UnorderedSet`. O(log n) insert/find/erase, sorted
// iteration, stable element addresses. Keys are immutable through the set (iterators are
// const). GAME-PERF: prefer `FlatSet` (cache-friendly sorted array) or `UnorderedSet` (hash);
// reach for `Set` only when you need ordered iteration WITH stable refs and O(log n) mutation.
export namespace worse::core::container
{
    template <typename Key>
    struct SetKeyOfValue
    {
        WE_NODISCARD constexpr Key const& operator()(Key const& value) const noexcept { return value; }
    };

    template <typename Key, typename Compare = Less<Key>, typename Allocator = WE_DEFAULT_ALLOCATOR>
    class Set
    {
        using Table = RBTree<Key, Key, SetKeyOfValue<Key>, Compare, Allocator>;

    public:
        using KeyType        = Key;
        using ValueType      = Key;
        using CompareType    = Compare;
        using AllocatorType  = Allocator;
        using SizeType       = typename Table::SizeType;
        using ConstReference = Key const&;

        // Both alias the tree's CONST iterator: keys are immutable through the set.
        using Iterator         = typename Table::ConstIterator;
        using ConstIterator    = typename Table::ConstIterator;
        using ReverseIter      = ReverseIterator<ConstIterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        // --- construction ------------------------------------------------------

        Set() = default;
        explicit Set(AllocatorType const& allocator) : mTree(allocator) {}
        explicit Set(Compare const& comp, AllocatorType const& allocator = AllocatorType{}) : mTree(comp, allocator) {}

        template <typename InIt>
            requires InputIterator<InIt>
        Set(InIt first, InIt last)
        {
            mTree.insert(first, last);
        }

        Set(std::initializer_list<Key> init) { mTree.insert(init.begin(), init.end()); }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mTree.empty(); }
        WE_NODISCARD SizeType size() const noexcept { return mTree.size(); }

        // --- iterators (const only) --------------------------------------------

        WE_NODISCARD ConstIterator begin() const noexcept { return mTree.begin(); }
        WE_NODISCARD ConstIterator end() const noexcept { return mTree.end(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mTree.cbegin(); }
        WE_NODISCARD ConstIterator cend() const noexcept { return mTree.cend(); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(end()); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(begin()); }

        // --- lookup ------------------------------------------------------------

        WE_NODISCARD ConstIterator find(Key const& key) const noexcept { return mTree.find(key); }
        WE_NODISCARD bool contains(Key const& key) const noexcept { return mTree.contains(key); }
        WE_NODISCARD SizeType count(Key const& key) const noexcept { return mTree.count(key); }
        WE_NODISCARD ConstIterator lowerBound(Key const& key) const noexcept { return mTree.lowerBound(key); }
        WE_NODISCARD ConstIterator upperBound(Key const& key) const noexcept { return mTree.upperBound(key); }
        WE_NODISCARD Pair<ConstIterator, ConstIterator> equalRange(Key const& key) const noexcept
        {
            return {mTree.lowerBound(key), mTree.upperBound(key)};
        }

        // --- modifiers ---------------------------------------------------------

        Pair<ConstIterator, bool> insert(Key const& value) { return mTree.insertUnique(value); }
        Pair<ConstIterator, bool> insert(Key&& value) { return mTree.insertUnique(worse::core::move(value)); }

        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            mTree.insert(first, last);
        }

        template <typename... Args>
        Pair<ConstIterator, bool> emplace(Args&&... args)
        {
            return mTree.emplaceUnique(worse::core::forward<Args>(args)...);
        }

        SizeType erase(Key const& key) { return mTree.erase(key); }
        ConstIterator erase(ConstIterator pos) { return mTree.erase(pos); }

        void clear() noexcept { mTree.clear(); }
        void swap(Set& other) noexcept(noexcept(mTree.swap(other.mTree))) { mTree.swap(other.mTree); }

    private:
        Table mTree{};
    };

    template <typename Key, typename Compare, typename Allocator>
    void swap(Set<Key, Compare, Allocator>& a, Set<Key, Compare, Allocator>& b) noexcept(noexcept(a.swap(b)))
    {
        a.swap(b);
    }
} // namespace worse::core::container
