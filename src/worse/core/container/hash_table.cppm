module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

export module worse.core.container.hash_table;
import worse.core.basic_type;
import worse.core.memory;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.allocator_traits;
import worse.core.container.iterator;

// Open-addressing Robin Hood hash table -- the shared engine behind UnorderedSet and
// UnorderedMap (DECISIONS D2/R8/R9). Parameterized EASTL-style on a stored `Value`, the
// `Key` it is looked up by, and a `KeyOfValue` extractor (identity for a set, `.first`
// for a map) so one engine serves both.
//
// Layout (R9, AoS): a power-of-two `Value` slot array + a parallel `InfoType` array.
// `info == 0` marks an empty slot; an occupied slot stores `displacement + 1`, where the
// displacement (DIB -- distance from ideal bucket) is how far the element sits from
// `hash & mask`. One extra guard byte past the end (set non-zero) terminates the
// iterator's skip-empties scan without a bounds check.
//
// Robin Hood (R8): on insertion the "poor" incoming element (larger displacement) steals
// a "rich" resident's slot and the resident is re-homed -- bounding the variance of probe
// lengths. Deletion is BACKWARD-SHIFT (no tombstones): the run after the hole slides back
// one slot until an empty or ideal-position element is reached. INVARIANT for lookup: walk
// from the ideal bucket; the key cannot be present once a resident's displacement is
// smaller than the current probe distance, so the search stops early.
//
// `InfoType` is `u16` (not `u8`): a degenerate hash (every key colliding) makes the
// displacement grow with the element count, which a single byte cannot represent; two
// bytes keep heavy-collision workloads correct at negligible metadata cost, and the
// (now effectively unreachable) overflow aborts under the no-exceptions contract.
//
// CONTRACT: any rehash (grow / reserve) OR any erase invalidates ALL iterators and
// references -- open addressing relocates elements. Never hold a slot pointer across a
// mutation. Helpers live in this namespace, left out of the `export` block (module
// linkage hides them) -- no `_detail` sub-namespace, per DECISIONS.
namespace worse::core::container
{
    // Forward iterator over occupied slots. Wraps a slot cursor + a parallel info cursor;
    // `operator++` skips empty slots, stopping at the guard sentinel (a non-zero info byte
    // one past the table) so `end()` needs no separate bounds check. Exposes the five
    // nested typedefs so `IteratorTraits` / the iterator concepts recognise it.
    template <typename Value, typename Info, bool IsConst>
    class HashTableIterator
    {
    public:
        using IteratorCategory = ForwardIteratorTag;
        using ValueType        = Value;
        using DifferenceType   = isize;
        using Pointer          = Conditional<IsConst, Value const*, Value*>;
        using Reference        = Conditional<IsConst, Value const&, Value&>;

        HashTableIterator() noexcept = default;
        HashTableIterator(Pointer slot, Info const* info) noexcept : mpSlot(slot), mpInfo(info) {}

        // Non-const -> const conversion (never the reverse).
        template <bool OtherConst, typename = EnableIf<IsConst && !OtherConst>>
        HashTableIterator(HashTableIterator<Value, Info, OtherConst> const& other) noexcept
            : mpSlot(other.slotPointer()), mpInfo(other.infoCursor())
        {
        }

        WE_NODISCARD Reference operator*() const noexcept { return *mpSlot; }
        WE_NODISCARD Pointer operator->() const noexcept { return mpSlot; }

        HashTableIterator& operator++() noexcept
        {
            do
            {
                ++mpSlot;
                ++mpInfo;
            } while (*mpInfo == 0);
            return *this;
        }
        HashTableIterator operator++(int) noexcept
        {
            HashTableIterator tmp = *this;
            ++*this;
            return tmp;
        }

        WE_NODISCARD friend bool operator==(HashTableIterator const& a, HashTableIterator const& b) noexcept
        {
            return a.mpSlot == b.mpSlot;
        }
        WE_NODISCARD friend bool operator!=(HashTableIterator const& a, HashTableIterator const& b) noexcept
        {
            return a.mpSlot != b.mpSlot;
        }

        WE_NODISCARD Pointer slotPointer() const noexcept { return mpSlot; }
        WE_NODISCARD Info const* infoCursor() const noexcept { return mpInfo; }

    private:
        Pointer mpSlot     = nullptr;
        Info const* mpInfo = nullptr;
    };

    // Storage + RAII half (non-exported). Owns the two raw buffers and the allocator (EBO),
    // and nothing else; its destructor frees the BUFFERS only -- destroying the live
    // elements is the derived HashTable's job (it runs its destructor first), mirroring
    // ArrayBase.
    template <typename Value, typename Allocator>
    class HashTableBase
    {
    public:
        using AllocatorType = Allocator;
        using AllocTraits   = AllocatorTraits<Allocator>;
        using SizeType      = usize;
        using InfoType      = u16;

        static constexpr InfoType kInfoEnd   = static_cast<InfoType>(-1); // iterator guard sentinel
        static constexpr usize kInvalidIndex = static_cast<usize>(-1);
        static constexpr usize kMinCapacity  = 16;

        HashTableBase() noexcept = default;
        explicit HashTableBase(AllocatorType const& allocator) noexcept : mAllocator{allocator} {}

        ~HashTableBase() noexcept
        {
            if (mpSlots != nullptr)
            {
                doFreeSlots(mpSlots, mCapacity);
            }
            if (mpInfo != nullptr)
            {
                doFreeInfo(mpInfo, mCapacity);
            }
        }

        WE_NODISCARD AllocatorType& getAllocator() noexcept { return mAllocator; }
        WE_NODISCARD AllocatorType const& getAllocator() const noexcept { return mAllocator; }

    protected:
        Value* mpSlots   = nullptr;
        InfoType* mpInfo = nullptr;
        usize mCapacity  = 0; // power of two, or 0 when empty
        usize mMask      = 0; // mCapacity - 1 (valid only when mCapacity != 0)
        usize mSize      = 0; // live element count
        WE_NO_UNIQUE_ADDRESS AllocatorType mAllocator{};

        WE_NODISCARD Value* doAllocateSlots(usize cap) noexcept
        {
            void* p = AllocTraits::allocate(mAllocator, cap * sizeof(Value), alignof(Value));
            if (p == nullptr)
            {
                memory::handleAllocationFailure(cap * sizeof(Value), alignof(Value));
            }
            return static_cast<Value*>(p);
        }

        void doFreeSlots(Value* p, usize cap) noexcept
        {
            if (p != nullptr)
            {
                AllocTraits::deallocate(mAllocator, p, cap * sizeof(Value), alignof(Value));
            }
        }

        // Allocate cap + 1 info entries (the +1 is the guard), zero the table region, and
        // set the guard sentinel so iterator scans terminate.
        WE_NODISCARD InfoType* doAllocateInfo(usize cap) noexcept
        {
            usize const count = cap + 1;
            void* p           = AllocTraits::allocate(mAllocator, count * sizeof(InfoType), alignof(InfoType));
            if (p == nullptr)
            {
                memory::handleAllocationFailure(count * sizeof(InfoType), alignof(InfoType));
            }
            InfoType* info = static_cast<InfoType*>(p);
            __builtin_memset(static_cast<void*>(info), 0, cap * sizeof(InfoType));
            info[cap] = kInfoEnd;
            return info;
        }

        void doFreeInfo(InfoType* p, usize cap) noexcept
        {
            if (p != nullptr)
            {
                AllocTraits::deallocate(mAllocator, p, (cap + 1) * sizeof(InfoType), alignof(InfoType));
            }
        }
    };

    export template <
        typename Value,
        typename Key,
        typename KeyOfValue,
        typename Hasher,
        typename KeyEqual,
        typename Allocator = WE_DEFAULT_ALLOCATOR>
    class HashTable : public HashTableBase<Value, Allocator>
    {
        using BaseType = HashTableBase<Value, Allocator>;
        using ThisType = HashTable<Value, Key, KeyOfValue, Hasher, KeyEqual, Allocator>;

        using BaseType::doAllocateInfo;
        using BaseType::doAllocateSlots;
        using BaseType::doFreeInfo;
        using BaseType::doFreeSlots;
        using BaseType::mAllocator;
        using BaseType::mCapacity;
        using BaseType::mMask;
        using BaseType::mpInfo;
        using BaseType::mpSlots;
        using BaseType::mSize;

    public:
        using ValueType     = Value;
        using KeyType       = Key;
        using SizeType      = usize;
        using AllocatorType = Allocator;
        using AllocTraits   = AllocatorTraits<Allocator>;
        using InfoType      = typename BaseType::InfoType;

        using Iterator      = HashTableIterator<Value, InfoType, false>;
        using ConstIterator = HashTableIterator<Value, InfoType, true>;

        using BaseType::kInfoEnd;
        using BaseType::kInvalidIndex;
        using BaseType::kMinCapacity;

        static constexpr f32 kMaxLoadFactor = 0.875f; // 7/8

        // --- construction / destruction ---------------------------------------

        HashTable() noexcept = default;

        explicit HashTable(AllocatorType const& allocator) noexcept : BaseType{allocator} {}

        HashTable(ThisType const& other)
            : BaseType{AllocTraits::selectOnContainerCopyConstruction(other.getAllocator())}
        {
            mHash     = other.mHash;
            mKeyEqual = other.mKeyEqual;
            mKeyOf    = other.mKeyOf;
            if (other.mCapacity != 0)
            {
                mpSlots = doAllocateSlots(other.mCapacity);
                mpInfo  = doAllocateInfo(other.mCapacity);
                // Copy the exact Robin Hood layout: same info bytes, same slot indices.
                for (usize i = 0; i < other.mCapacity; ++i)
                {
                    mpInfo[i] = other.mpInfo[i];
                    if (other.mpInfo[i] != 0)
                    {
                        AllocTraits::construct(mAllocator, mpSlots + i, other.mpSlots[i]);
                    }
                }
                mCapacity = other.mCapacity;
                mMask     = other.mMask;
                mSize     = other.mSize;
            }
        }

        HashTable(ThisType&& other) noexcept : BaseType{}
        {
            stealFrom(other);
        }

        ~HashTable() noexcept { destroyAllSlots(); }

        ThisType& operator=(ThisType const& other)
        {
            if (this != &other)
            {
                ThisType tmp(other);
                swap(tmp);
            }
            return *this;
        }

        ThisType& operator=(ThisType&& other) noexcept
        {
            if (this != &other)
            {
                destroyAllSlots();
                if (mpSlots != nullptr)
                {
                    doFreeSlots(mpSlots, mCapacity);
                }
                if (mpInfo != nullptr)
                {
                    doFreeInfo(mpInfo, mCapacity);
                }
                mpSlots    = nullptr;
                mpInfo     = nullptr;
                mCapacity  = 0;
                mMask      = 0;
                mSize      = 0;
                mAllocator = other.mAllocator;
                stealFrom(other);
            }
            return *this;
        }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD bool empty() const noexcept { return mSize == 0; }
        WE_NODISCARD SizeType size() const noexcept { return mSize; }
        WE_NODISCARD SizeType bucketCount() const noexcept { return mCapacity; }
        WE_NODISCARD f32 maxLoadFactor() const noexcept { return kMaxLoadFactor; }
        WE_NODISCARD f32 loadFactor() const noexcept
        {
            return mCapacity == 0 ? 0.0f : static_cast<f32>(mSize) / static_cast<f32>(mCapacity);
        }

        // Ensure room for at least n elements without exceeding the max load factor.
        void reserve(SizeType n)
        {
            SizeType const need = capacityForElements(n);
            if (need > mCapacity)
            {
                rehashToCapacity(need);
            }
        }

        // Set the bucket count to at least n (rounded up to a power of two), never below
        // what the current size needs.
        void rehash(SizeType n)
        {
            SizeType cap = kMinCapacity;
            while (cap < n)
            {
                cap <<= 1;
            }
            SizeType const minForSize = capacityForElements(mSize);
            if (cap < minForSize)
            {
                cap = minForSize;
            }
            if (cap != mCapacity)
            {
                rehashToCapacity(cap);
            }
        }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept
        {
            usize const i = firstOccupied();
            return Iterator(mpSlots + i, mpInfo + i);
        }
        WE_NODISCARD ConstIterator begin() const noexcept
        {
            usize const i = firstOccupied();
            return ConstIterator(mpSlots + i, mpInfo + i);
        }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return begin(); }

        WE_NODISCARD Iterator end() noexcept { return Iterator(mpSlots + mCapacity, mpInfo + mCapacity); }
        WE_NODISCARD ConstIterator end() const noexcept { return ConstIterator(mpSlots + mCapacity, mpInfo + mCapacity); }
        WE_NODISCARD ConstIterator cend() const noexcept { return end(); }

        // --- lookup ------------------------------------------------------------

        WE_NODISCARD Iterator find(Key const& key) noexcept
        {
            usize const idx = findIndex(key);
            return idx == kInvalidIndex ? end() : Iterator(mpSlots + idx, mpInfo + idx);
        }
        WE_NODISCARD ConstIterator find(Key const& key) const noexcept
        {
            usize const idx = findIndex(key);
            return idx == kInvalidIndex ? end() : ConstIterator(mpSlots + idx, mpInfo + idx);
        }
        WE_NODISCARD bool contains(Key const& key) const noexcept { return findIndex(key) != kInvalidIndex; }
        WE_NODISCARD SizeType count(Key const& key) const noexcept
        {
            return findIndex(key) != kInvalidIndex ? SizeType{1} : SizeType{0};
        }

        // --- modifiers ---------------------------------------------------------

        Pair<Iterator, bool> insertUnique(Value const& value)
        {
            ensureCapacityForOne();
            Value tmp(value);
            Pair<usize, bool> const r = insertNoGrow(worse::core::move(tmp));
            return makePair(Iterator(mpSlots + r.first, mpInfo + r.first), r.second);
        }

        Pair<Iterator, bool> insertUnique(Value&& value)
        {
            ensureCapacityForOne();
            Pair<usize, bool> const r = insertNoGrow(worse::core::move(value));
            return makePair(Iterator(mpSlots + r.first, mpInfo + r.first), r.second);
        }

        template <typename... Args>
        Pair<Iterator, bool> emplace(Args&&... args)
        {
            Value tmp(worse::core::forward<Args>(args)...);
            ensureCapacityForOne();
            Pair<usize, bool> const r = insertNoGrow(worse::core::move(tmp));
            return makePair(Iterator(mpSlots + r.first, mpInfo + r.first), r.second);
        }

        template <typename InIt>
            requires InputIterator<InIt>
        void insert(InIt first, InIt last)
        {
            for (; first != last; ++first)
            {
                insertUnique(*first);
            }
        }

        SizeType eraseByKey(Key const& key)
        {
            usize const idx = findIndex(key);
            if (idx == kInvalidIndex)
            {
                return SizeType{0};
            }
            eraseAt(idx);
            return SizeType{1};
        }

        Iterator eraseByIterator(ConstIterator pos)
        {
            usize const idx = static_cast<usize>(pos.infoCursor() - mpInfo);
            eraseAt(idx);
            return iteratorAtOrAfter(idx);
        }

        void clear() noexcept
        {
            destroyAllSlots();
            if (mCapacity != 0)
            {
                __builtin_memset(static_cast<void*>(mpInfo), 0, mCapacity * sizeof(InfoType));
            }
            mSize = 0;
        }

        // Test/debug hook: verify the Robin Hood invariant holds over the whole table --
        // every occupied slot's stored displacement equals its actual (wrap-aware) distance
        // from its ideal bucket, every key re-probes to its own slot, and the live count
        // matches mSize. Not called in production; cheap to leave compiled in.
        WE_NODISCARD bool checkRobinHoodInvariant() const noexcept
        {
            usize live = 0;
            for (usize i = 0; i < mCapacity; ++i)
            {
                if (mpInfo[i] == 0)
                {
                    continue;
                }
                ++live;
                usize const ideal      = mHash(mKeyOf(mpSlots[i])) & mMask;
                usize const actualDisp = (i - ideal) & mMask; // wrap-aware
                if (static_cast<usize>(mpInfo[i] - 1) != actualDisp)
                {
                    return false;
                }
                if (findIndex(mKeyOf(mpSlots[i])) != i)
                {
                    return false;
                }
            }
            return live == mSize;
        }

        void swap(ThisType& other) noexcept
        {
            worse::core::swap(mpSlots, other.mpSlots);
            worse::core::swap(mpInfo, other.mpInfo);
            worse::core::swap(mCapacity, other.mCapacity);
            worse::core::swap(mMask, other.mMask);
            worse::core::swap(mSize, other.mSize);
            worse::core::swap(mHash, other.mHash);
            worse::core::swap(mKeyEqual, other.mKeyEqual);
            worse::core::swap(mKeyOf, other.mKeyOf);
            if constexpr (AllocTraits::propagateOnContainerSwap)
            {
                worse::core::swap(mAllocator, other.mAllocator);
            }
        }

    private:
        WE_NO_UNIQUE_ADDRESS Hasher mHash{};
        WE_NO_UNIQUE_ADDRESS KeyEqual mKeyEqual{};
        WE_NO_UNIQUE_ADDRESS KeyOfValue mKeyOf{};

        // Smallest power-of-two capacity (>= kMinCapacity) whose 7/8 load holds `count`.
        WE_NODISCARD static usize capacityForElements(usize count) noexcept
        {
            usize cap = kMinCapacity;
            while (count * 8u > cap * 7u)
            {
                cap <<= 1;
            }
            return cap;
        }

        void ensureCapacityForOne()
        {
            if (mCapacity == 0)
            {
                rehashToCapacity(kMinCapacity);
            }
            else if ((mSize + 1u) * 8u > mCapacity * 7u)
            {
                rehashToCapacity(mCapacity * 2u);
            }
        }

        WE_NODISCARD usize firstOccupied() const noexcept
        {
            usize i = 0;
            while (i < mCapacity && mpInfo[i] == 0)
            {
                ++i;
            }
            return i;
        }

        WE_NODISCARD Iterator iteratorAtOrAfter(usize idx) noexcept
        {
            while (idx < mCapacity && mpInfo[idx] == 0)
            {
                ++idx;
            }
            return Iterator(mpSlots + idx, mpInfo + idx);
        }

        // Probe with the Robin Hood early-exit. Returns the slot index or kInvalidIndex.
        WE_NODISCARD usize findIndex(Key const& key) const noexcept
        {
            if (mSize == 0)
            {
                return kInvalidIndex;
            }
            usize idx     = mHash(key) & mMask;
            InfoType dist = 1; // info value an element placed HERE would carry
            for (;;)
            {
                InfoType const info = mpInfo[idx];
                if (info == 0)
                {
                    return kInvalidIndex; // empty slot: not present
                }
                if (info < dist)
                {
                    return kInvalidIndex; // resident is richer than we'd be: not present
                }
                if (info == dist && mKeyEqual(mKeyOf(mpSlots[idx]), key))
                {
                    return idx;
                }
                idx = (idx + 1) & mMask;
                ++dist;
            }
        }

        // Place `value` if its key is absent. Returns {slot index of the inserted element,
        // inserted?}. Precondition: ensureCapacityForOne() has guaranteed a free slot.
        Pair<usize, bool> insertNoGrow(Value&& value)
        {
            usize idx           = mHash(mKeyOf(value)) & mMask;
            InfoType dist       = 1;
            usize insertedIndex = kInvalidIndex;
            for (;;)
            {
                InfoType const info = mpInfo[idx];
                if (info == 0)
                {
                    AllocTraits::construct(mAllocator, mpSlots + idx, worse::core::move(value));
                    mpInfo[idx] = dist;
                    ++mSize;
                    if (insertedIndex == kInvalidIndex)
                    {
                        insertedIndex = idx;
                    }
                    return makePair(insertedIndex, true);
                }
                // The duplicate check is only meaningful before the first steal: afterwards
                // `value` carries a displaced (already-unique) resident being re-homed.
                if (insertedIndex == kInvalidIndex && info == dist && mKeyEqual(mKeyOf(mpSlots[idx]), mKeyOf(value)))
                {
                    return makePair(idx, false);
                }
                if (info < dist)
                {
                    // We are poorer: steal this slot and re-home the displaced resident.
                    worse::core::swap(value, mpSlots[idx]);
                    worse::core::swap(dist, mpInfo[idx]);
                    if (insertedIndex == kInvalidIndex)
                    {
                        insertedIndex = idx;
                    }
                }
                idx = (idx + 1) & mMask;
                ++dist;
                WE_ASSERT(dist != 0); // u16 wrap guard; the load factor keeps this unreachable
            }
        }

        // Backward-shift delete (no tombstones): slide the following run back one slot
        // until an empty or already-ideal (displacement 0) element is reached.
        void eraseAt(usize idx) noexcept
        {
            AllocTraits::destroy(mAllocator, mpSlots + idx);
            usize next = (idx + 1) & mMask;
            while (mpInfo[next] > 1) // occupied AND displaced (displacement > 0)
            {
                AllocTraits::construct(mAllocator, mpSlots + idx, worse::core::move(mpSlots[next]));
                AllocTraits::destroy(mAllocator, mpSlots + next);
                mpInfo[idx]  = static_cast<InfoType>(mpInfo[next] - 1);
                mpInfo[next] = 0;
                idx          = next;
                next         = (next + 1) & mMask;
            }
            mpInfo[idx] = 0;
            --mSize;
        }

        // Reallocate to newCap (power of two, large enough for mSize) and re-home every
        // live element into the fresh table. Frees the old buffers.
        void rehashToCapacity(usize newCap)
        {
            Value* oldSlots    = mpSlots;
            InfoType* oldInfo  = mpInfo;
            usize const oldCap = mCapacity;

            mpSlots   = doAllocateSlots(newCap);
            mpInfo    = doAllocateInfo(newCap);
            mCapacity = newCap;
            mMask     = newCap - 1;
            mSize     = 0;

            for (usize i = 0; i < oldCap; ++i)
            {
                if (oldInfo[i] != 0)
                {
                    insertNoGrow(worse::core::move(oldSlots[i]));
                    AllocTraits::destroy(mAllocator, oldSlots + i);
                }
            }
            if (oldSlots != nullptr)
            {
                doFreeSlots(oldSlots, oldCap);
            }
            if (oldInfo != nullptr)
            {
                doFreeInfo(oldInfo, oldCap);
            }
        }

        void destroyAllSlots() noexcept
        {
            if constexpr (!IsTriviallyDestructible<Value>)
            {
                for (usize i = 0; i < mCapacity; ++i)
                {
                    if (mpInfo[i] != 0)
                    {
                        AllocTraits::destroy(mAllocator, mpSlots + i);
                    }
                }
            }
        }

        // Take ownership of other's buffers, leaving it empty. Assumes *this holds nothing.
        void stealFrom(ThisType& other) noexcept
        {
            mpSlots   = other.mpSlots;
            mpInfo    = other.mpInfo;
            mCapacity = other.mCapacity;
            mMask     = other.mMask;
            mSize     = other.mSize;
            mHash     = worse::core::move(other.mHash);
            mKeyEqual = worse::core::move(other.mKeyEqual);
            mKeyOf    = worse::core::move(other.mKeyOf);

            other.mpSlots   = nullptr;
            other.mpInfo    = nullptr;
            other.mCapacity = 0;
            other.mMask     = 0;
            other.mSize     = 0;
        }
    };

    export template <
        typename Value,
        typename Key,
        typename KeyOfValue,
        typename Hasher,
        typename KeyEqual,
        typename Allocator>
    void swap(
        HashTable<Value, Key, KeyOfValue, Hasher, KeyEqual, Allocator>& a,
        HashTable<Value, Key, KeyOfValue, Hasher, KeyEqual, Allocator>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container
