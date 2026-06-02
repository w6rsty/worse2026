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

export namespace worse::core::container
{
    template <typename Key>
    struct SetKeyOfValue
    {
        WE_NODISCARD constexpr Key const& operator()(Key const& value) const noexcept { return value; }
    };

    /**
     * \brief Ordered unique set backed by the red-black `RBTree` engine (identity key extractor).
     *
     * The ordered counterpart to `UnorderedSet`: O(log n) insert/find/erase, sorted iteration,
     * stable element addresses. Keys are immutable through the set (iterators are const).
     * \note Default `Compare` is the TRANSPARENT `Less<>` (like FlatSet) so heterogeneous lookup
     *       works out of the box (e.g. find by a view/projection with no temporary Key). (R45)
     * \note GAME-PERF: prefer `FlatSet` (cache-friendly sorted array) or `UnorderedSet` (hash);
     *       reach for `Set` only when you need ordered iteration WITH stable refs and O(log n)
     *       mutation.
     */
    template <typename Key, typename Compare = Less<>, typename Allocator = WE_DEFAULT_ALLOCATOR>
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

        /**
         * \brief Find the element equivalent to \p key.
         * \tparam K query type; transparent (heterogeneous) lookup builds no temporary `Key`,
         *         see RBTree (R45).
         * \return iterator to the element, or end() if absent. O(log n).
         */
        template <typename K>
        WE_NODISCARD ConstIterator find(K const& key) const noexcept { return mTree.find(key); }
        /**
         * \brief Test whether an element equivalent to \p key exists.
         * \tparam K query type; transparent (heterogeneous) lookup, see find(). O(log n).
         */
        template <typename K>
        WE_NODISCARD bool contains(K const& key) const noexcept { return mTree.contains(key); }
        /**
         * \brief Count elements equivalent to \p key (0 or 1).
         * \tparam K query type; transparent (heterogeneous) lookup, see find(). O(log n).
         */
        template <typename K>
        WE_NODISCARD SizeType count(K const& key) const noexcept { return mTree.count(key); }
        /**
         * \brief First element not less than \p key.
         * \tparam K query type; transparent (heterogeneous) lookup, see find(). O(log n).
         */
        template <typename K>
        WE_NODISCARD ConstIterator lowerBound(K const& key) const noexcept { return mTree.lowerBound(key); }
        /**
         * \brief First element greater than \p key.
         * \tparam K query type; transparent (heterogeneous) lookup, see find(). O(log n).
         */
        template <typename K>
        WE_NODISCARD ConstIterator upperBound(K const& key) const noexcept { return mTree.upperBound(key); }
        /**
         * \brief Range [lowerBound, upperBound) of elements equivalent to \p key.
         * \tparam K query type; transparent (heterogeneous) lookup, see find().
         */
        template <typename K>
        WE_NODISCARD Pair<ConstIterator, ConstIterator> equalRange(K const& key) const noexcept
        {
            return {mTree.lowerBound(key), mTree.upperBound(key)};
        }

        // --- modifiers ---------------------------------------------------------

        /**
         * \brief Insert \p value if absent; no-op if an equivalent key already exists.
         * \return a pair {iterator-to-element, inserted?}. O(log n).
         */
        Pair<ConstIterator, bool> insert(Key const& value) { return mTree.insertUnique(value); }
        Pair<ConstIterator, bool> insert(Key&& value) { return mTree.insertUnique(worse::core::move(value)); }

        /**
         * \brief Insert each element of the range [\p first, \p last), skipping duplicates.
         * \tparam InIt input iterator type.
         */
        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            mTree.insert(first, last);
        }

        /**
         * \brief Construct a key in place and insert it if absent.
         * \tparam Args constructor argument types for `Key`.
         * \return a pair {iterator-to-element, inserted?}. O(log n).
         */
        template <typename... Args>
        Pair<ConstIterator, bool> emplace(Args&&... args)
        {
            return mTree.emplaceUnique(worse::core::forward<Args>(args)...);
        }

        /**
         * \brief Erase the element with key \p key, if any.
         * \return the number removed (0 or 1). O(log n).
         * \note erase stays Key-typed (no transparent overload), see RBTree.
         */
        SizeType erase(Key const& key) { return mTree.erase(key); } // see RBTree: erase stays Key-typed
        /**
         * \brief Erase the element at \p pos.
         * \return iterator to the following element.
         */
        ConstIterator erase(ConstIterator pos) { return mTree.erase(pos); }

        /**
         * \brief Remove all elements.
         */
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
