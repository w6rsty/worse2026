module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <initializer_list>

export module worse.core.container.unordered_set;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.iterator;
import worse.core.container.hash;
import worse.core.container.hash_table;

// Unordered set: unique keys in an open-addressing Robin Hood table (DECISIONS D2). A thin
// adapter over HashTable where the stored value IS the key (identity extractor). Average
// O(1) insert / find / erase, cache-friendly contiguous slots -- the hash counterpart to
// FlatSet's sorted array.
//
// Iterators are CONSTANT (dereference to `Key const&`): a key is its own hash position, so
// mutating it in place would strand it in the wrong bucket. Change membership only via
// insert / erase. ANY insert that rehashes, and ANY erase, invalidates all iterators and
// references (open addressing relocates elements) -- the HashTable contract.
//
// Lookups are keyed on the exact `Key` this iteration; heterogeneous/transparent hashing
// is deferred (it needs a transparent Hash + KeyEqual, landing with string types later).
export namespace worse::core::container
{
    // Identity extractor: in a set the stored value is the key. Non-exported helper.
    template <typename Key>
    struct UnorderedSetKeyOfValue
    {
        WE_NODISCARD constexpr Key const& operator()(Key const& value) const noexcept { return value; }
    };

    template <
        typename Key,
        typename Hasher    = Hash<Key>,
        typename KeyEqual  = EqualTo<Key>,
        typename Allocator = WE_DEFAULT_ALLOCATOR>
    class UnorderedSet
    {
        using Table = HashTable<Key, Key, UnorderedSetKeyOfValue<Key>, Hasher, KeyEqual, Allocator>;

    public:
        using KeyType        = Key;
        using ValueType      = Key;
        using HasherType     = Hasher;
        using KeyEqualType   = KeyEqual;
        using AllocatorType  = Allocator;
        using SizeType       = typename Table::SizeType;
        using ConstReference = Key const&;

        // Both alias the table's CONST iterator: keys are immutable through the set.
        using Iterator      = typename Table::ConstIterator;
        using ConstIterator = typename Table::ConstIterator;

        // --- construction ------------------------------------------------------

        UnorderedSet() = default;

        explicit UnorderedSet(AllocatorType const& allocator) : mTable(allocator) {}

        template <typename InIt>
            requires InputIterator<InIt>
        UnorderedSet(InIt first, InIt last)
        {
            mTable.insert(first, last);
        }

        UnorderedSet(std::initializer_list<Key> init) { mTable.insert(init.begin(), init.end()); }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mTable.empty(); }
        WE_NODISCARD SizeType size() const noexcept { return mTable.size(); }
        WE_NODISCARD SizeType bucketCount() const noexcept { return mTable.bucketCount(); }
        WE_NODISCARD f32 loadFactor() const noexcept { return mTable.loadFactor(); }
        WE_NODISCARD f32 maxLoadFactor() const noexcept { return mTable.maxLoadFactor(); }
        void reserve(SizeType n) { mTable.reserve(n); }
        void rehash(SizeType n) { mTable.rehash(n); }

        // --- iterators (const only) --------------------------------------------

        WE_NODISCARD ConstIterator begin() const noexcept { return mTable.begin(); }
        WE_NODISCARD ConstIterator end() const noexcept { return mTable.end(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mTable.begin(); }
        WE_NODISCARD ConstIterator cend() const noexcept { return mTable.end(); }

        // --- lookup ------------------------------------------------------------

        WE_NODISCARD ConstIterator find(Key const& key) const noexcept { return mTable.find(key); }
        WE_NODISCARD bool contains(Key const& key) const noexcept { return mTable.contains(key); }
        WE_NODISCARD SizeType count(Key const& key) const noexcept { return mTable.count(key); }

        // --- modifiers ---------------------------------------------------------

        Pair<Iterator, bool> insert(Key const& value)
        {
            auto const r = mTable.insertUnique(value);
            return makePair(ConstIterator(r.first), r.second);
        }
        Pair<Iterator, bool> insert(Key&& value)
        {
            auto const r = mTable.insertUnique(worse::core::move(value));
            return makePair(ConstIterator(r.first), r.second);
        }

        template <typename... Args>
        Pair<Iterator, bool> emplace(Args&&... args)
        {
            auto const r = mTable.emplace(worse::core::forward<Args>(args)...);
            return makePair(ConstIterator(r.first), r.second);
        }

        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            mTable.insert(first, last);
        }

        // erase by key vs by iterator: Iterator == ConstIterator here (keys immutable), so a
        // plain non-template `erase(Key const&)` resolves unambiguously against
        // `erase(ConstIterator)` (no overload-trap -- see PITFALLS / the map adapter).
        SizeType erase(Key const& key) { return mTable.eraseByKey(key); }
        Iterator erase(ConstIterator pos) { return ConstIterator(mTable.eraseByIterator(pos)); }

        void clear() noexcept { mTable.clear(); }

        void swap(UnorderedSet& other) noexcept { mTable.swap(other.mTable); }

    private:
        Table mTable{};
    };

    template <typename Key, typename Hasher, typename KeyEqual, typename Allocator>
    void swap(UnorderedSet<Key, Hasher, KeyEqual, Allocator>& a, UnorderedSet<Key, Hasher, KeyEqual, Allocator>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
