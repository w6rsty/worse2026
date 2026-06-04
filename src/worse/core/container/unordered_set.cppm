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

export namespace worse::core::container
{
    /**
     * \brief Identity key extractor: in a set the stored value IS the key. Non-exported helper.
     */
    template <typename Key>
    struct UnorderedSetKeyOfValue
    {
        WE_NODISCARD constexpr Key const& operator()(Key const& value) const noexcept { return value; }
    };

    /**
     * \brief Hash set of unique keys: a thin adapter over the open-addressing Robin Hood
     *        HashTable with an identity key extractor and const iterators (DECISIONS D2).
     * \ingroup ctr_hash
     * \tparam Key element type, which is also the key.
     * \tparam Hasher hash functor for Key.
     * \tparam KeyEqual equality comparator for Key.
     * \tparam Allocator allocator type.
     * \note Average O(1) insert / find / erase over cache-friendly contiguous slots -- the
     *       hash counterpart to FlatSet's sorted array.
     * \note Iterators are CONSTANT (dereference to `Key const&`): a key is its own hash
     *       position, so mutating it in place would strand it in the wrong bucket. Change
     *       membership only via insert / erase.
     * \note Any insert that rehashes, and any erase, invalidates all iterators and references
     *       (open addressing relocates elements) -- the HashTable contract.
     * \note Iteration order is unspecified and changes on rehash -- never rely on it for
     *       deterministic output (replay/netcode); sort into a buffer if you need a stable
     *       order (R45).
     * \note Lookups are keyed on the exact `Key` this iteration; heterogeneous/transparent
     *       hashing is deferred (it needs a transparent Hash + KeyEqual, landing with string
     *       types later).
     */
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

        // --- iterators (const only) --------------------------------------------

        WE_NODISCARD ConstIterator begin() const noexcept { return mTable.begin(); }
        WE_NODISCARD ConstIterator end() const noexcept { return mTable.end(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mTable.begin(); }
        WE_NODISCARD ConstIterator cend() const noexcept { return mTable.end(); }

        // --- lookup ------------------------------------------------------------

        /**
         * \brief Find the element equal to \p key.
         * \param key lookup key.
         * \return const iterator to the element, or end() if absent.
         * \note Average O(1).
         */
        WE_NODISCARD ConstIterator find(Key const& key) const noexcept { return mTable.find(key); }
        /**
         * \brief Test whether \p key is present.
         * \param key lookup key.
         * \return true iff an element equal to \p key exists.
         */
        WE_NODISCARD bool contains(Key const& key) const noexcept { return mTable.contains(key); }
        /**
         * \brief Count the elements equal to \p key.
         * \param key lookup key.
         * \return 0 or 1 (keys are unique).
         */
        WE_NODISCARD SizeType count(Key const& key) const noexcept { return mTable.count(key); }

        // --- modifiers ---------------------------------------------------------

        /**
         * \brief Insert \p value if no equivalent key is present.
         * \param value element to copy in.
         * \return {iterator to the element, true if insertion happened}.
         * \note Average O(1); may rehash, invalidating all iterators and references.
         */
        Pair<Iterator, bool> insert(Key const& value)
        {
            auto const r = mTable.insertUnique(value);
            return makePair(ConstIterator(r.first), r.second);
        }
        /**
         * \brief Insert \p value if no equivalent key is present.
         * \param value element to move in.
         * \return {iterator to the element, true if insertion happened}.
         * \note Average O(1); may rehash, invalidating all iterators and references.
         */
        Pair<Iterator, bool> insert(Key&& value)
        {
            auto const r = mTable.insertUnique(worse::core::move(value));
            return makePair(ConstIterator(r.first), r.second);
        }

        /**
         * \brief Construct a key in place and insert it if no equivalent key is present.
         * \tparam Args constructor argument types for Key.
         * \param args arguments forwarded to the Key constructor.
         * \return {iterator to the element, true if insertion happened}.
         * \note The key is built before the existence check; a duplicate discards it.
         */
        template <typename... Args>
        Pair<Iterator, bool> emplace(Args&&... args)
        {
            auto const r = mTable.emplace(worse::core::forward<Args>(args)...);
            return makePair(ConstIterator(r.first), r.second);
        }

        /**
         * \brief Insert every element of the range [first, last), skipping duplicates.
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
         * \note Iterator == ConstIterator here (keys immutable), so this plain non-template
         *       overload resolves unambiguously against `erase(ConstIterator)` -- no
         *       overload-trap (see PITFALLS / the map adapter).
         */
        SizeType erase(Key const& key) { return mTable.eraseByKey(key); }
        /**
         * \brief Erase the element at \p pos.
         * \param pos const iterator to a valid element (must not be end()).
         * \return iterator to the next element after the erased one.
         * \pre \p pos refers to a live element of this set.
         * \note Backward-shift delete; invalidates all iterators and references.
         */
        Iterator erase(ConstIterator pos) { return ConstIterator(mTable.eraseByIterator(pos)); }

        /**
         * \brief Destroy all elements, keeping the allocated buckets.
         * \note Capacity is retained; size becomes 0.
         */
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
