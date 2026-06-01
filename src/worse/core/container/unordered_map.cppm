module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <initializer_list>

export module worse.core.container.unordered_map;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.iterator;
import worse.core.container.hash;
import worse.core.container.hash_table;

// Unordered map: key/value pairs in an open-addressing Robin Hood table (DECISIONS D2). A
// thin adapter over HashTable storing `Pair<Key, T>`, keyed by `.first`. Average O(1)
// insert / find / erase -- the hash counterpart to FlatMap's sorted array.
//
// Unlike std::unordered_map's `pair<const Key, T>`, the stored key is NON-const (R15): the
// table relocates elements on rehash / backward-shift erase, which a const key would
// forbid. The contract therefore mirrors FlatMap (R16): iterators are mutable, but you may
// modify ONLY `.second`. Mutating `.first` strands the element in the wrong bucket -- change
// a key only via erase + insert. ANY insert that rehashes, and ANY erase, invalidates all
// iterators and references.
//
// Lookups are keyed on the exact `Key` this iteration; heterogeneous/transparent hashing is
// deferred. Because lookups are non-templated, `erase(Key const&)` and `erase(ConstIterator)`
// resolve unambiguously -- no overload-trap (the trap needs a templated key erase).
export namespace worse::core::container
{
    // Extracts the key (`.first`) from a stored `Pair<Key, T>`. Non-exported helper.
    template <typename Key, typename T>
    struct UnorderedMapKeyOfValue
    {
        WE_NODISCARD constexpr Key const& operator()(Pair<Key, T> const& value) const noexcept { return value.first; }
    };

    template <
        typename Key,
        typename T,
        typename Hasher    = Hash<Key>,
        typename KeyEqual  = EqualTo<Key>,
        typename Allocator = WE_DEFAULT_ALLOCATOR>
    class UnorderedMap
    {
        using Table =
            HashTable<Pair<Key, T>, Key, UnorderedMapKeyOfValue<Key, T>, Hasher, KeyEqual, Allocator>;

    public:
        using KeyType        = Key;
        using MappedType     = T;
        using ValueType      = Pair<Key, T>;
        using HasherType     = Hasher;
        using KeyEqualType   = KeyEqual;
        using AllocatorType  = Allocator;
        using SizeType       = typename Table::SizeType;
        using Reference      = ValueType&;
        using ConstReference = ValueType const&;

        // Mutable iterators -- but only `.second` may change (see the class contract).
        using Iterator      = typename Table::Iterator;
        using ConstIterator = typename Table::ConstIterator;

        // --- construction ------------------------------------------------------

        UnorderedMap() = default;

        explicit UnorderedMap(AllocatorType const& allocator) : mTable(allocator) {}

        template <typename InIt>
            requires InputIterator<InIt>
        UnorderedMap(InIt first, InIt last)
        {
            mTable.insert(first, last);
        }

        UnorderedMap(std::initializer_list<ValueType> init) { mTable.insert(init.begin(), init.end()); }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mTable.empty(); }
        WE_NODISCARD SizeType size() const noexcept { return mTable.size(); }
        WE_NODISCARD SizeType bucketCount() const noexcept { return mTable.bucketCount(); }
        WE_NODISCARD f32 loadFactor() const noexcept { return mTable.loadFactor(); }
        WE_NODISCARD f32 maxLoadFactor() const noexcept { return mTable.maxLoadFactor(); }
        void reserve(SizeType n) { mTable.reserve(n); }
        void rehash(SizeType n) { mTable.rehash(n); }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return mTable.begin(); }
        WE_NODISCARD ConstIterator begin() const noexcept { return mTable.begin(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mTable.begin(); }
        WE_NODISCARD Iterator end() noexcept { return mTable.end(); }
        WE_NODISCARD ConstIterator end() const noexcept { return mTable.end(); }
        WE_NODISCARD ConstIterator cend() const noexcept { return mTable.end(); }

        // --- lookup ------------------------------------------------------------

        WE_NODISCARD Iterator find(Key const& key) noexcept { return mTable.find(key); }
        WE_NODISCARD ConstIterator find(Key const& key) const noexcept { return mTable.find(key); }
        WE_NODISCARD bool contains(Key const& key) const noexcept { return mTable.contains(key); }
        WE_NODISCARD SizeType count(Key const& key) const noexcept { return mTable.count(key); }

        // --- element access ----------------------------------------------------

        // Return the mapped value for `key`, inserting a value-initialized one if absent.
        // The reference is read AFTER the insert completes -- a rehash may have moved the
        // slot, so a pre-insert pointer would dangle. Not nodiscard: `m[k];` is an idiom.
        T& operator[](Key const& key)
        {
            Iterator it = mTable.find(key);
            if (it != mTable.end())
            {
                return it->second;
            }
            auto const r = mTable.insertUnique(ValueType(key, T{}));
            return r.first->second;
        }

        T& operator[](Key&& key)
        {
            Iterator it = mTable.find(key);
            if (it != mTable.end())
            {
                return it->second;
            }
            auto const r = mTable.insertUnique(ValueType(worse::core::move(key), T{}));
            return r.first->second;
        }

        // Access an existing mapped value. UB if the key is absent (debug WE_ASSERT).
        WE_NODISCARD T& at(Key const& key) noexcept
        {
            Iterator it = mTable.find(key);
            WE_ASSERT(it != mTable.end());
            return it->second;
        }
        WE_NODISCARD T const& at(Key const& key) const noexcept
        {
            ConstIterator it = mTable.find(key);
            WE_ASSERT(it != mTable.end());
            return it->second;
        }

        // --- modifiers ---------------------------------------------------------

        Pair<Iterator, bool> insert(ValueType const& value) { return mTable.insertUnique(value); }
        Pair<Iterator, bool> insert(ValueType&& value) { return mTable.insertUnique(worse::core::move(value)); }

        // Insert {key, mapped} if absent, else overwrite the existing mapped value.
        template <typename M>
        Pair<Iterator, bool> insertOrAssign(Key const& key, M&& mapped)
        {
            Iterator it = mTable.find(key);
            if (it != mTable.end())
            {
                it->second = worse::core::forward<M>(mapped);
                return makePair(it, false);
            }
            return mTable.insertUnique(ValueType(key, worse::core::forward<M>(mapped)));
        }

        // Insert {key, T(args...)} only if the key is absent. The mapped value is built ONLY
        // when inserting (the win over operator[] / insert for an expensive T).
        template <typename... Args>
        Pair<Iterator, bool> tryEmplace(Key const& key, Args&&... args)
        {
            Iterator it = mTable.find(key);
            if (it != mTable.end())
            {
                return makePair(it, false);
            }
            return mTable.insertUnique(ValueType(key, T(worse::core::forward<Args>(args)...)));
        }

        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            mTable.insert(first, last);
        }

        SizeType erase(Key const& key) { return mTable.eraseByKey(key); }
        Iterator erase(ConstIterator pos) { return mTable.eraseByIterator(pos); }

        void clear() noexcept { mTable.clear(); }

        void swap(UnorderedMap& other) noexcept { mTable.swap(other.mTable); }

    private:
        Table mTable{};
    };

    template <typename Key, typename T, typename Hasher, typename KeyEqual, typename Allocator>
    void swap(
        UnorderedMap<Key, T, Hasher, KeyEqual, Allocator>& a,
        UnorderedMap<Key, T, Hasher, KeyEqual, Allocator>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
