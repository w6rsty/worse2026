module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

export module worse.core.container.swiss_table;
import worse.core.basic_type;
import worse.core.intrinsics;
import worse.core.memory;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.allocator_traits;
import worse.core.container.iterator;
import worse.core.container.hash; // Hash<T>

/**
 * \file
 * \brief SwissTable: a control-byte open-addressing hash engine (the abseil flat_hash /
 *        EASTL2 design), an alternative to the Robin Hood `hash_table` behind the same
 *        `<Value, Key, KeyOfValue, Hasher, KeyEqual, Allocator>` facade (R8).
 *
 * Each slot has a 1-byte control tag: a 7-bit hash fragment H2 for occupied slots, or
 * empty/deleted. Lookups scan a GROUP of 8 control bytes at once with branch-free SWAR
 * (no SIMD intrinsics -> portable), so a probe step rejects up to 8 slots with a couple of
 * integer ops -- the cache-and-branch win that makes SwissTable the modern game-grade hash.
 *
 * \note Refs/iterators invalidate on rehash/erase (open addressing, per D2).
 * \note Layout: two allocations (control bytes + value slots, per R23). Capacity is a power
 *       of two; the first GroupWidth control bytes are MIRRORED at the end so an 8-byte
 *       group load near the wrap edge reads the wrapped-around slots without a branch (no
 *       sentinel -> iteration uses an index bound, sidestepping the sentinel/mirror
 *       interplay). Max load 7/8 (R23). Tombstones on erase, reclaimed on rehash.
 * \note PREDICTABILITY (R45): erase leaves a tombstone; an insert whose fresh slot would
 *       exceed the 7/8 load COUNTING tombstones rehashes -- and when most of the load is
 *       tombstones it rehashes IN PLACE (same capacity) to reclaim them. So steady-state
 *       insert/erase churn can trigger a rehash mid-frame even without growth. The Robin
 *       Hood `hash_table` is tombstone-free (backward-shift erase), so prefer it for
 *       per-frame churn-heavy maps. To stay alloc-free here, call `reserve(n)` after a batch
 *       erase to reclaim tombstones up front, and `wouldRehashOnInsert()` to pre-pay the
 *       rehash at a safe point.
 * \note SWAR byte order assumes little-endian (x86/ARM); a big-endian port would byte-swap
 *       the group load. Internal move/forward fully qualified (ADL).
 */
namespace worse::core::container
{
    using CtrlT = u8;

    inline constexpr CtrlT kEmpty       = 0x80; // 1000_0000
    inline constexpr CtrlT kDeleted     = 0xFE; // 1111_1110
    inline constexpr usize kGroupWidth  = 8;
    inline constexpr usize kSwissMinCap = 16; // power of two, >= kGroupWidth

    WE_NODISCARD WE_FORCEINLINE bool ctrlIsFull(CtrlT c) noexcept
    {
        return (c & 0x80u) == 0;
    }

    // 8 control bytes packed into a u64 (little-endian: byte i == slot offset i), scanned with
    // SWAR. Each "mask" returns a u64 with bit 7 of byte i set iff slot i matches.
    struct SwissGroup
    {
        u64 mCtrl;

        static constexpr u64 kLSBs = 0x0101010101010101ull;
        static constexpr u64 kMSBs = 0x8080808080808080ull;

        explicit SwissGroup(CtrlT const* p) noexcept { intrinsics::memCopy(&mCtrl, p, kGroupWidth); }

        // Slots whose control byte equals h (a full H2 in [0,0x7F]).
        WE_NODISCARD WE_FORCEINLINE u64 match(CtrlT h) const noexcept
        {
            u64 const x = mCtrl ^ (kLSBs * h);
            return (x - kLSBs) & ~x & kMSBs;
        }
        // Slots that are empty (0x80) -- distinguishes empty from deleted (0xFE).
        WE_NODISCARD WE_FORCEINLINE u64 maskEmpty() const noexcept { return mCtrl & ~(mCtrl << 6) & kMSBs; }
        // Slots that are empty OR deleted (any control with bit7 set; full have bit7 clear).
        WE_NODISCARD WE_FORCEINLINE u64 maskEmptyOrDeleted() const noexcept { return mCtrl & kMSBs; }
    };

    // Iterate the set bits of a SWAR mask as slot offsets (0..7), low to high.
    WE_NODISCARD WE_FORCEINLINE u32 lowestMatch(u64 mask) noexcept
    {
        return static_cast<u32>(intrinsics::countTrailingZeros64(mask) >> 3);
    }

    /**
     * \brief Forward iterator over the full (occupied) slots of a SwissTable.
     * \note Advances by index, skipping empty/deleted control bytes up to the capacity bound
     *       (no sentinel); end() is the iterator at index == capacity.
     */
    template <typename Value, bool IsConst>
    class SwissIterator
    {
    public:
        using IteratorCategory = ForwardIteratorTag;
        using ValueType        = Value;
        using DifferenceType   = isize;
        using Pointer          = Conditional<IsConst, Value const*, Value*>;
        using Reference        = Conditional<IsConst, Value const&, Value&>;

        constexpr SwissIterator() = default;
        constexpr SwissIterator(CtrlT const* ctrl, Value* slot, usize index, usize cap) noexcept
            : mpCtrl(ctrl), mpSlot(slot), mIndex(index), mCap(cap)
        {
        }

        // const conversion
        template <bool C, typename = EnableIf<IsConst && !C>>
        constexpr SwissIterator(SwissIterator<Value, C> const& o) noexcept
            : mpCtrl(o.ctrl()), mpSlot(o.slot()), mIndex(o.index()), mCap(o.cap())
        {
        }

        WE_NODISCARD Reference operator*() const noexcept { return mpSlot[mIndex]; }
        WE_NODISCARD Pointer operator->() const noexcept { return mpSlot + mIndex; }

        SwissIterator& operator++() noexcept
        {
            ++mIndex;
            skipToFull();
            return *this;
        }
        SwissIterator operator++(int) noexcept
        {
            SwissIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        WE_NODISCARD constexpr CtrlT const* ctrl() const noexcept { return mpCtrl; }
        WE_NODISCARD constexpr Value* slot() const noexcept { return mpSlot; }
        WE_NODISCARD constexpr usize index() const noexcept { return mIndex; }
        WE_NODISCARD constexpr usize cap() const noexcept { return mCap; }

        WE_NODISCARD friend bool operator==(SwissIterator const& a, SwissIterator const& b) noexcept
        {
            return a.mIndex == b.mIndex;
        }
        WE_NODISCARD friend bool operator!=(SwissIterator const& a, SwissIterator const& b) noexcept
        {
            return a.mIndex != b.mIndex;
        }

        // Advance mIndex to the next full slot (or to mCap == end).
        void skipToFull() noexcept
        {
            while (mIndex < mCap && !ctrlIsFull(mpCtrl[mIndex]))
            {
                ++mIndex;
            }
        }

    private:
        CtrlT const* mpCtrl = nullptr;
        Value* mpSlot       = nullptr;
        usize mIndex        = 0;
        usize mCap          = 0;
    };

    /**
     * \brief Control-byte open-addressing hash table (SwissTable / abseil flat_hash design)
     *        using portable SWAR control-byte groups.
     * \tparam Value stored element type.
     * \tparam Key key the element is looked up by.
     * \tparam KeyOfValue extractor mapping a stored Value to its Key.
     * \tparam Hasher hash functor for Key.
     * \tparam KeyEqual equality comparator for Key.
     * \tparam Allocator allocator type.
     * \note Iteration order is unspecified and changes on rehash. Erase leaves a tombstone;
     *       a rehash (grow or in-place tombstone reclaim) and any erase invalidate all
     *       iterators and references (R45, R23).
     */
    export template <
        typename Value,
        typename Key,
        typename KeyOfValue,
        typename Hasher    = Hash<Key>,
        typename KeyEqual  = EqualTo<Key>,
        typename Allocator = WE_DEFAULT_ALLOCATOR>
    class SwissTable
    {
        using AllocTraits = AllocatorTraits<Allocator>;
        using ThisType    = SwissTable<Value, Key, KeyOfValue, Hasher, KeyEqual, Allocator>;

    public:
        using ValueType      = Value;
        using KeyType        = Key;
        using SizeType       = usize;
        using DifferenceType = isize;
        using AllocatorType  = Allocator;
        using Reference      = Value&;
        using ConstReference = Value const&;

        using Iterator      = SwissIterator<Value, false>;
        using ConstIterator = SwissIterator<Value, true>;

        static constexpr SizeType NPos = static_cast<SizeType>(-1);

        SwissTable() noexcept(IsNothrowDefaultConstructible<Allocator>) = default;
        explicit SwissTable(Allocator const& allocator) noexcept : mAllocator(allocator) {}

        SwissTable(ThisType const& other)
            : mAllocator(AllocTraits::selectOnContainerCopyConstruction(other.mAllocator))
        {
            if (other.mSize != 0)
            {
                reserveForInsert(other.mSize);
                for (CtrlT const* c = other.mpCtrl; c != other.mpCtrl + other.mCapacity; ++c)
                {
                    if (ctrlIsFull(*c))
                    {
                        insertUnique(other.mpSlots[static_cast<usize>(c - other.mpCtrl)]);
                    }
                }
            }
        }

        SwissTable(ThisType&& other) noexcept
            : mpCtrl(other.mpCtrl)
            , mpSlots(other.mpSlots)
            , mCapacity(other.mCapacity)
            , mSize(other.mSize)
            , mDeleted(other.mDeleted)
            , mAllocator(worse::core::move(other.mAllocator))
        {
            other.mpCtrl    = nullptr;
            other.mpSlots   = nullptr;
            other.mCapacity = 0;
            other.mSize     = 0;
            other.mDeleted  = 0;
        }

        ~SwissTable() { freeAll(); }

        ThisType& operator=(ThisType const& other)
        {
            if (this != &other)
            {
                clear();
                if constexpr (AllocTraits::propagateOnContainerCopyAssignment)
                {
                    // free under the OLD allocator, then adopt the new one
                    freeAll();
                    mAllocator = other.mAllocator;
                }
                for (CtrlT const* c = other.mpCtrl; c != other.mpCtrl + other.mCapacity; ++c)
                {
                    if (ctrlIsFull(*c))
                    {
                        insertUnique(other.mpSlots[static_cast<usize>(c - other.mpCtrl)]);
                    }
                }
            }
            return *this;
        }

        ThisType& operator=(ThisType&& other) noexcept
        {
            if (this != &other)
            {
                freeAll();
                mpCtrl          = other.mpCtrl;
                mpSlots         = other.mpSlots;
                mCapacity       = other.mCapacity;
                mSize           = other.mSize;
                mDeleted        = other.mDeleted;
                mAllocator      = worse::core::move(other.mAllocator);
                other.mpCtrl    = nullptr;
                other.mpSlots   = nullptr;
                other.mCapacity = 0;
                other.mSize     = 0;
                other.mDeleted  = 0;
            }
            return *this;
        }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mSize == 0; }
        WE_NODISCARD SizeType size() const noexcept { return mSize; }
        WE_NODISCARD SizeType capacity() const noexcept { return mCapacity; }
        WE_NODISCARD f32 loadFactor() const noexcept
        {
            return mCapacity == 0 ? 0.0f : static_cast<f32>(mSize) / static_cast<f32>(mCapacity);
        }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return makeIterator(firstFullIndex()); }
        WE_NODISCARD ConstIterator begin() const noexcept { return makeConstIterator(firstFullIndex()); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return begin(); }
        WE_NODISCARD Iterator end() noexcept { return makeIterator(mCapacity); }
        WE_NODISCARD ConstIterator end() const noexcept { return makeConstIterator(mCapacity); }
        WE_NODISCARD ConstIterator cend() const noexcept { return end(); }

        // --- lookup ------------------------------------------------------------

        /**
         * \brief Find the element equal to \p key.
         * \param key lookup key.
         * \return iterator to the element, or end() if absent.
         * \note Average O(1); each probe step rejects up to 8 slots via a SWAR group scan.
         */
        WE_NODISCARD Iterator find(Key const& key) noexcept { return makeIterator(findIndex(key)); }
        /**
         * \brief Find the element equal to \p key.
         * \param key lookup key.
         * \return const iterator to the element, or end() if absent.
         * \note Average O(1); each probe step rejects up to 8 slots via a SWAR group scan.
         */
        WE_NODISCARD ConstIterator find(Key const& key) const noexcept { return makeConstIterator(findIndex(key)); }
        /**
         * \brief Test whether \p key is present.
         * \param key lookup key.
         * \return true iff an element equal to \p key exists.
         */
        WE_NODISCARD bool contains(Key const& key) const noexcept { return findIndex(key) != mCapacity; }
        /**
         * \brief Count the elements equal to \p key.
         * \param key lookup key.
         * \return 0 or 1 (keys are unique).
         */
        WE_NODISCARD SizeType count(Key const& key) const noexcept
        {
            return findIndex(key) != mCapacity ? SizeType{1} : SizeType{0};
        }

        // --- modifiers ---------------------------------------------------------

        /**
         * \brief Insert \p value if its key is absent.
         * \param value element to copy in.
         * \return {iterator to the element, true if inserted, false if the key already existed}.
         * \note May rehash (grow or reclaim tombstones), invalidating all iterators/references.
         */
        Pair<Iterator, bool> insertUnique(Value const& value) { return emplaceImpl(value); }
        /**
         * \brief Insert \p value if its key is absent.
         * \param value element to move in.
         * \return {iterator to the element, true if inserted, false if the key already existed}.
         * \note May rehash (grow or reclaim tombstones), invalidating all iterators/references.
         */
        Pair<Iterator, bool> insertUnique(Value&& value) { return emplaceImpl(worse::core::move(value)); }

        /**
         * \brief Construct an element in place and insert it if its key is absent.
         * \tparam Args constructor argument types for Value.
         * \param args arguments forwarded to the Value constructor.
         * \return {iterator to the element, true if inserted, false if the key already existed}.
         * \note The element is built before the existence check; a duplicate key discards it.
         */
        template <typename... Args>
        Pair<Iterator, bool> emplaceUnique(Args&&... args)
        {
            return emplaceImpl(worse::core::forward<Args>(args)...);
        }

        /**
         * \brief Insert every element of the range [first, last).
         * \tparam InIt input iterator type.
         * \param first range begin.
         * \param last range end.
         * \note Duplicate keys are skipped; may rehash one or more times.
         */
        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            for (; first != last; ++first)
            {
                insertUnique(*first);
            }
        }

        /**
         * \brief Erase the element with \p key, if present.
         * \param key key to remove.
         * \return 1 if an element was erased, else 0.
         * \note Leaves a tombstone (reclaimed on a later rehash); invalidates all
         *       iterators and references.
         */
        SizeType erase(Key const& key) noexcept
        {
            usize const idx = findIndex(key);
            if (idx == mCapacity)
            {
                return 0;
            }
            eraseAt(idx);
            return 1;
        }

        /**
         * \brief Erase the element at \p pos.
         * \param pos const iterator to a valid element (must not be end()).
         * \return iterator to the next full slot after the erased one.
         * \pre \p pos refers to a live element of this table.
         * \note Leaves a tombstone (reclaimed on a later rehash); invalidates all
         *       iterators and references.
         */
        Iterator erase(ConstIterator pos) noexcept
        {
            usize idx = pos.index();
            eraseAt(idx);
            // advance to the next full slot after the erased one
            ++idx;
            while (idx < mCapacity && !ctrlIsFull(mpCtrl[idx]))
            {
                ++idx;
            }
            return makeIterator(idx);
        }

        /**
         * \brief Destroy all elements, keeping the allocated capacity.
         * \note Resets all control bytes to empty (drops tombstones); size becomes 0.
         */
        void clear() noexcept
        {
            if (mpCtrl == nullptr || mSize == 0)
            {
                if (mpCtrl != nullptr)
                {
                    setAllEmpty();
                }
                mSize    = 0;
                mDeleted = 0;
                return;
            }
            destroyAllSlots();
            setAllEmpty();
            mSize    = 0;
            mDeleted = 0;
        }

        /**
         * \brief Ensure inserting up to \p n total elements is rehash-free.
         * \param n target total element count.
         * \note Grows if \p n exceeds the 7/8 load; otherwise, if accumulated tombstones
         *       would force a rehash before reaching \p n live elements, reclaims them IN
         *       PLACE now (R45) -- so a `reserve` after a batch erase restores the alloc-free
         *       steady state. After reserve(n) the next `n - size()` inserts allocate nothing.
         */
        void reserve(SizeType n)
        {
            if (n > maxLoad(mCapacity))
            {
                resize(capacityForSize(n)); // grow to fit n (also drops tombstones)
            }
            else if (mDeleted != 0 && n + mDeleted > maxLoad(mCapacity))
            {
                resize(mCapacity); // capacity fits n, but tombstones would rehash first -> reclaim now
            }
        }

        /**
         * \brief Report whether inserting one NEW key right now would trigger a rehash.
         * \return true if a fresh insert would grow or reclaim tombstones in place.
         * \note Lets frame code pre-pay via reserve() at a safe point rather than eat the
         *       stall mid-insert. Conservative: an insert hitting an existing key, or reusing
         *       a tombstone without crossing the load, does not rehash. (R45)
         */
        WE_NODISCARD bool wouldRehashOnInsert() const noexcept
        {
            return mSize + mDeleted + 1 > maxLoad(mCapacity);
        }

        void swap(ThisType& other) noexcept
        {
            worse::core::swap(mpCtrl, other.mpCtrl);
            worse::core::swap(mpSlots, other.mpSlots);
            worse::core::swap(mCapacity, other.mCapacity);
            worse::core::swap(mSize, other.mSize);
            worse::core::swap(mDeleted, other.mDeleted);
            if constexpr (AllocTraits::propagateOnContainerSwap)
            {
                worse::core::swap(mAllocator, other.mAllocator);
            }
        }

    private:
        CtrlT* mpCtrl   = nullptr; // mCapacity + kGroupWidth bytes (last GroupWidth mirror the first)
        Value* mpSlots  = nullptr; // mCapacity value slots (raw storage; live where ctrl is full)
        usize mCapacity = 0;
        usize mSize     = 0;
        usize mDeleted  = 0;
        WE_NO_UNIQUE_ADDRESS Allocator mAllocator{};

        WE_NODISCARD static usize maxLoad(usize cap) noexcept { return cap - (cap / 8); } // 7/8

        WE_NODISCARD static usize capacityForSize(usize n) noexcept
        {
            usize cap = kSwissMinCap;
            while (maxLoad(cap) < n)
            {
                cap *= 2;
            }
            return cap;
        }

        WE_NODISCARD WE_FORCEINLINE static CtrlT h2Of(usize hash) noexcept { return static_cast<CtrlT>(hash & 0x7Fu); }

        WE_NODISCARD Iterator makeIterator(usize index) noexcept { return Iterator(mpCtrl, mpSlots, index, mCapacity); }
        WE_NODISCARD ConstIterator makeConstIterator(usize index) const noexcept
        {
            return ConstIterator(mpCtrl, mpSlots, index, mCapacity);
        }

        WE_NODISCARD usize firstFullIndex() const noexcept
        {
            usize i = 0;
            while (i < mCapacity && !ctrlIsFull(mpCtrl[i]))
            {
                ++i;
            }
            return i;
        }

        WE_NODISCARD static Key const& keyOf(Value const& v) noexcept { return KeyOfValue{}(v); }

        // Probe for `key`; return its slot index or mCapacity if absent.
        WE_NODISCARD WE_FORCEINLINE usize findIndex(Key const& key) const noexcept
        {
            if (mCapacity == 0)
            {
                return mCapacity;
            }
            usize const hash = Hasher{}(key);
            usize const mask = mCapacity - 1;
            CtrlT const h2   = h2Of(hash);
            usize pos        = (hash >> 7) & mask;
            for (usize probed = 0; probed <= mCapacity; probed += kGroupWidth)
            {
                SwissGroup g(mpCtrl + pos);
                for (u64 m = g.match(h2); m != 0; m &= m - 1)
                {
                    usize const slot = (pos + lowestMatch(m)) & mask;
                    if (KeyEqual{}(keyOf(mpSlots[slot]), key)) [[likely]]
                    {
                        return slot;
                    }
                }
                if (g.maskEmpty() != 0)
                {
                    return mCapacity; // hit an empty -> not present
                }
                pos = (pos + kGroupWidth) & mask;
            }
            return mCapacity;
        }

        template <typename... Args>
        Pair<Iterator, bool> emplaceImpl(Args&&... args)
        {
            // Build the value first so we can hash/compare its key (handles arbitrary args).
            // For the common value-passthrough this is a single move/copy the compiler elides.
            if (mCapacity == 0)
            {
                resize(kSwissMinCap);
            }
            Value tmp(worse::core::forward<Args>(args)...);
            Key const& key   = keyOf(tmp);
            usize const hash = Hasher{}(key);

            // Single probe: locates an existing match, or the first empty/deleted insert slot.
            usize slot;
            if (findOrPrepare(key, hash, slot))
            {
                return {makeIterator(slot), false}; // already present
            }
            // Only an EMPTY slot consumes fresh capacity; a tombstone reuse does not. Rehash
            // (dropping tombstones / growing) only when a fresh slot would exceed the 7/8 load.
            if (mpCtrl[slot] == kEmpty && mSize + mDeleted + 1 > maxLoad(mCapacity))
            {
                rehashAndGrow();
                slot = findInsertSlot(hash); // all tombstones gone -> an empty slot
            }
            bool const wasDeleted = (mpCtrl[slot] == kDeleted);
            AllocTraits::construct(mAllocator, mpSlots + slot, worse::core::move(tmp));
            setCtrl(slot, h2Of(hash));
            ++mSize;
            if (wasDeleted)
            {
                --mDeleted;
            }
            return {makeIterator(slot), true};
        }

        // One probe pass: if `key` is present, set outSlot to it and return true; otherwise set
        // outSlot to the first empty-or-deleted slot in the sequence (tombstone reuse preferred)
        // and return false.
        WE_NODISCARD bool findOrPrepare(Key const& key, usize hash, usize& outSlot) const noexcept
        {
            usize const mask = mCapacity - 1;
            CtrlT const h2   = h2Of(hash);
            usize pos        = (hash >> 7) & mask;
            usize firstAvail = NPos;
            while (true)
            {
                SwissGroup g(mpCtrl + pos);
                for (u64 m = g.match(h2); m != 0; m &= m - 1)
                {
                    usize const s = (pos + lowestMatch(m)) & mask;
                    if (KeyEqual{}(keyOf(mpSlots[s]), key)) [[likely]]
                    {
                        outSlot = s;
                        return true;
                    }
                }
                if (firstAvail == NPos)
                {
                    u64 const avail = g.maskEmptyOrDeleted();
                    if (avail != 0)
                    {
                        firstAvail = (pos + lowestMatch(avail)) & mask;
                    }
                }
                if (g.maskEmpty() != 0)
                {
                    outSlot = firstAvail; // an empty terminates the run -> firstAvail is set
                    return false;
                }
                pos = (pos + kGroupWidth) & mask;
            }
        }

        // First empty-or-deleted slot in the probe sequence for `hash` (capacity guaranteed).
        WE_NODISCARD usize findInsertSlot(usize hash) const noexcept
        {
            usize const mask = mCapacity - 1;
            usize pos        = (hash >> 7) & mask;
            while (true)
            {
                SwissGroup g(mpCtrl + pos);
                u64 const avail = g.maskEmptyOrDeleted();
                if (avail != 0)
                {
                    return (pos + lowestMatch(avail)) & mask;
                }
                pos = (pos + kGroupWidth) & mask;
            }
        }

        void eraseAt(usize idx) noexcept
        {
            // Leaves a tombstone (kDeleted): preserves probe sequences but consumes a slot until
            // the next rehash reclaims it. See reserve()/wouldRehashOnInsert() (R45).
            AllocTraits::destroy(mAllocator, mpSlots + idx);
            setCtrl(idx, kDeleted);
            --mSize;
            ++mDeleted;
        }

        // Write a control byte, keeping the first-group mirror in sync for wraparound reads.
        void setCtrl(usize i, CtrlT h) noexcept
        {
            mpCtrl[i] = h;
            if (i < kGroupWidth)
            {
                mpCtrl[mCapacity + i] = h;
            }
        }

        void setAllEmpty() noexcept { intrinsics::memSet(mpCtrl, kEmpty, mCapacity + kGroupWidth); }

        void rehashAndGrow()
        {
            // Mostly tombstones -> rehash in place (same cap, drop tombstones); else grow 2x.
            if (mSize <= maxLoad(mCapacity) / 2)
            {
                resize(mCapacity);
            }
            else
            {
                resize(mCapacity * 2);
            }
        }

        void reserveForInsert(usize n)
        {
            usize const cap = capacityForSize(n);
            if (cap > mCapacity)
            {
                resize(cap);
            }
        }

        // Allocate fresh arrays of newCap and re-home every live element; frees the old block.
        void resize(usize newCap)
        {
            CtrlT* const oldCtrl  = mpCtrl;
            Value* const oldSlots = mpSlots;
            usize const oldCap    = mCapacity;

            CtrlT* const newCtrl = static_cast<CtrlT*>(
                AllocTraits::allocate(mAllocator, newCap + kGroupWidth, alignof(CtrlT)));
            if (newCtrl == nullptr)
            {
                memory::handleAllocationFailure(newCap + kGroupWidth, alignof(CtrlT));
            }
            Value* const newSlots = static_cast<Value*>(
                AllocTraits::allocate(mAllocator, newCap * sizeof(Value), alignof(Value)));
            if (newSlots == nullptr)
            {
                memory::handleAllocationFailure(newCap * sizeof(Value), alignof(Value));
            }

            mpCtrl    = newCtrl;
            mpSlots   = newSlots;
            mCapacity = newCap;
            mSize     = 0;
            mDeleted  = 0;
            setAllEmpty();

            if (oldCtrl != nullptr)
            {
                for (usize i = 0; i < oldCap; ++i)
                {
                    if (ctrlIsFull(oldCtrl[i]))
                    {
                        rehomeSlot(worse::core::move(oldSlots[i]));
                        AllocTraits::destroy(mAllocator, oldSlots + i);
                    }
                }
                AllocTraits::deallocate(mAllocator, oldCtrl, oldCap + kGroupWidth, alignof(CtrlT));
                AllocTraits::deallocate(mAllocator, oldSlots, oldCap * sizeof(Value), alignof(Value));
            }
        }

        // Insert a known-unique element during resize (no existence check needed).
        void rehomeSlot(Value&& value)
        {
            usize const hash = Hasher{}(keyOf(value));
            usize const slot = findInsertSlot(hash);
            AllocTraits::construct(mAllocator, mpSlots + slot, worse::core::move(value));
            setCtrl(slot, h2Of(hash));
            ++mSize;
        }

        void destroyAllSlots() noexcept
        {
            if constexpr (!IsTriviallyDestructible<Value>)
            {
                for (usize i = 0; i < mCapacity; ++i)
                {
                    if (ctrlIsFull(mpCtrl[i]))
                    {
                        AllocTraits::destroy(mAllocator, mpSlots + i);
                    }
                }
            }
        }

        void freeAll() noexcept
        {
            if (mpCtrl != nullptr)
            {
                destroyAllSlots();
                AllocTraits::deallocate(mAllocator, mpCtrl, mCapacity + kGroupWidth, alignof(CtrlT));
                AllocTraits::deallocate(mAllocator, mpSlots, mCapacity * sizeof(Value), alignof(Value));
                mpCtrl    = nullptr;
                mpSlots   = nullptr;
                mCapacity = 0;
                mSize     = 0;
                mDeleted  = 0;
            }
        }
    };

    /**
     * \brief Swap the contents of two SwissTables.
     * \param a first table.
     * \param b second table.
     */
    export template <
        typename Value, typename Key, typename KeyOfValue, typename Hasher, typename KeyEqual, typename Allocator>
    void swap(
        SwissTable<Value, Key, KeyOfValue, Hasher, KeyEqual, Allocator>& a,
        SwissTable<Value, Key, KeyOfValue, Hasher, KeyEqual, Allocator>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
