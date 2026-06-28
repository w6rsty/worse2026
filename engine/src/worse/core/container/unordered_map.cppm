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

export namespace worse::core::container
{
    /**
     * \brief Key extractor: pulls the key (`.first`) from a stored `Pair<Key, T>`.
     *        Non-exported helper.
     */
    template <typename Key, typename T>
    struct UnorderedMapKeyOfValue
    {
        WE_NODISCARD constexpr Key const& operator()(Pair<Key, T> const& value) const noexcept { return value.first; }
    };

    /**
     * \brief Hash map of key/value pairs: a thin adapter over the open-addressing Robin Hood
     *        HashTable storing `Pair<Key, T>`, keyed by `.first` (DECISIONS D2).
     * \ingroup ctr_hash
     * \tparam Key key type.
     * \tparam T mapped value type.
     * \tparam Hasher hash functor for Key.
     * \tparam KeyEqual equality comparator for Key.
     * \tparam Allocator allocator type.
     * \note Average O(1) insert / find / erase -- the hash counterpart to FlatMap's sorted
     *       array.
     * \note Unlike std::unordered_map's `pair<const Key, T>`, the stored key is NON-const
     *       (R15): the table relocates elements on rehash / backward-shift erase, which a
     *       const key would forbid. The contract therefore mirrors FlatMap (R16): iterators
     *       are mutable, but you may modify ONLY `.second`. Mutating `.first` strands the
     *       element in the wrong bucket -- change a key only via erase + insert.
     * \note Any insert that rehashes, and any erase, invalidates all iterators and references.
     * \note Iteration order is unspecified and changes on rehash -- never rely on it for
     *       deterministic output (replay/netcode); sort into a buffer if you need a stable
     *       order (R45).
     * \note Lookups are keyed on the exact `Key` this iteration; heterogeneous/transparent
     *       hashing is deferred. Because lookups are non-templated, `erase(Key const&)` and
     *       `erase(ConstIterator)` resolve unambiguously -- no overload-trap (the trap needs
     *       a templated key erase).
     */
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
        /**
         * \brief Ensure room for at least \p n elements without exceeding the max load factor.
         * \param n target element count.
         * \note May rehash, invalidating all iterators and references.
         */
        void reserve(SizeType n) { mTable.reserve(n); }
        /**
         * \brief Set the bucket count to at least \p n (rounded up to a power of two).
         * \param n target bucket count; clamped up to what the current size needs.
         * \note Invalidates all iterators and references.
         */
        void rehash(SizeType n) { mTable.rehash(n); }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return mTable.begin(); }
        WE_NODISCARD ConstIterator begin() const noexcept { return mTable.begin(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mTable.begin(); }
        WE_NODISCARD Iterator end() noexcept { return mTable.end(); }
        WE_NODISCARD ConstIterator end() const noexcept { return mTable.end(); }
        WE_NODISCARD ConstIterator cend() const noexcept { return mTable.end(); }

        // --- lookup ------------------------------------------------------------

        /**
         * \brief Find the element whose key equals \p key.
         * \param key lookup key.
         * \return iterator to the element, or end() if absent.
         * \note Average O(1). Only `.second` of the result may be mutated.
         */
        WE_NODISCARD Iterator find(Key const& key) noexcept { return mTable.find(key); }
        /**
         * \brief Find the element whose key equals \p key.
         * \param key lookup key.
         * \return const iterator to the element, or end() if absent.
         * \note Average O(1).
         */
        WE_NODISCARD ConstIterator find(Key const& key) const noexcept { return mTable.find(key); }
        /**
         * \brief Test whether \p key is present.
         * \param key lookup key.
         * \return true iff an element with key equal to \p key exists.
         */
        WE_NODISCARD bool contains(Key const& key) const noexcept { return mTable.contains(key); }
        /**
         * \brief Count the elements whose key equals \p key.
         * \param key lookup key.
         * \return 0 or 1 (keys are unique).
         */
        WE_NODISCARD SizeType count(Key const& key) const noexcept { return mTable.count(key); }

        // --- element access ----------------------------------------------------

        /**
         * \brief Return a reference to the mapped value for \p key, inserting a
         *        value-initialized one if absent.
         * \param key lookup/insert key (copied).
         * \return mutable reference to the mapped value.
         * \note The reference is read AFTER the insert completes -- a rehash may have moved
         *       the slot, so a pre-insert pointer would dangle. Not nodiscard: `m[k];` is an
         *       idiom. May rehash, invalidating all iterators and references.
         */
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

        /**
         * \brief Return a reference to the mapped value for \p key, inserting a
         *        value-initialized one if absent.
         * \param key lookup/insert key (moved on insertion).
         * \return mutable reference to the mapped value.
         * \note See the const-key overload; may rehash, invalidating all iterators and
         *       references.
         */
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

        /**
         * \brief Access the mapped value of an existing element.
         * \param key key of the element.
         * \return mutable reference to the mapped value.
         * \pre \p key is present.
         * \note Undefined behavior if the key is absent (debug WE_ASSERT).
         */
        WE_NODISCARD T& at(Key const& key) noexcept
        {
            Iterator it = mTable.find(key);
            WE_ASSERT(it != mTable.end());
            return it->second;
        }
        /**
         * \brief Access the mapped value of an existing element.
         * \param key key of the element.
         * \return const reference to the mapped value.
         * \pre \p key is present.
         * \note Undefined behavior if the key is absent (debug WE_ASSERT).
         */
        WE_NODISCARD T const& at(Key const& key) const noexcept
        {
            ConstIterator it = mTable.find(key);
            WE_ASSERT(it != mTable.end());
            return it->second;
        }

        // --- modifiers ---------------------------------------------------------

        /**
         * \brief Insert \p value if its key is absent.
         * \param value key/value pair to copy in.
         * \return {iterator to the element, true if insertion happened}.
         * \note Average O(1); may rehash, invalidating all iterators and references.
         */
        Pair<Iterator, bool> insert(ValueType const& value) { return mTable.insertUnique(value); }
        /**
         * \brief Insert \p value if its key is absent.
         * \param value key/value pair to move in.
         * \return {iterator to the element, true if insertion happened}.
         * \note Average O(1); may rehash, invalidating all iterators and references.
         */
        Pair<Iterator, bool> insert(ValueType&& value) { return mTable.insertUnique(worse::core::move(value)); }

        /**
         * \brief Insert {key, mapped} if \p key is absent, else overwrite the existing mapped
         *        value.
         * \tparam M mapped-value argument type.
         * \param key lookup/insert key.
         * \param mapped value forwarded into `.second`.
         * \return {iterator to the element, false if an existing value was overwritten}.
         * \note May rehash, invalidating all iterators and references.
         */
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

        /**
         * \brief Insert {key, T(args...)} only if \p key is absent.
         * \tparam Args constructor argument types for the mapped value T.
         * \param key lookup/insert key.
         * \param args arguments forwarded to the T constructor.
         * \return {iterator to the element, true if insertion happened}.
         * \note The mapped value is built ONLY when inserting -- the win over operator[] /
         *       insert for an expensive T. May rehash, invalidating all iterators and
         *       references.
         */
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

        /**
         * \brief Insert every pair of the range [first, last), skipping duplicate keys.
         * \tparam InIt input iterator type.
         * \param first range begin.
         * \param last range end.
         * \note May rehash one or more times.
         */
        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            mTable.insert(first, last);
        }

        /**
         * \brief Erase the element with \p key, if present.
         * \param key key to remove.
         * \return 1 if an element was erased, else 0.
         * \note Backward-shift delete; invalidates all iterators and references.
         */
        SizeType erase(Key const& key) { return mTable.eraseByKey(key); }
        /**
         * \brief Erase the element at \p pos.
         * \param pos const iterator to a valid element (must not be end()).
         * \return iterator to the next element after the erased one.
         * \pre \p pos refers to a live element of this map.
         * \note Backward-shift delete; invalidates all iterators and references.
         */
        Iterator erase(ConstIterator pos) { return mTable.eraseByIterator(pos); }

        /**
         * \brief Destroy all elements, keeping the allocated buckets.
         * \note Capacity is retained; size becomes 0.
         */
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
