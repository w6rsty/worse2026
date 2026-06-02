module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <initializer_list>

export module worse.core.container.array;
import worse.core.basic_type;
import worse.core.memory;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.allocator;
import worse.core.container.allocator_traits;
import worse.core.container.iterator;
import worse.core.container.memory_util;
import worse.core.algorithm.modifying;

namespace worse::core::container
{
    // Storage + RAII half of the dynamic array (non-exported). Owns the raw buffer and
    // the allocator (EBO via WE_NO_UNIQUE_ADDRESS) and nothing else, so the whole
    // object is exactly three pointers plus the (often empty) allocator. The base
    // destructor frees the RAW storage only; destroying the live ELEMENTS is the
    // derived Array's responsibility (it runs its destructor first).
    template <typename T, typename Allocator>
    class ArrayBase
    {
    public:
        using ValueType      = T;
        using AllocatorType  = Allocator;
        using AllocTraits    = AllocatorTraits<Allocator>;
        using SizeType       = usize;
        using DifferenceType = isize;

        static constexpr SizeType const NPos         = static_cast<usize>(-1);
        static constexpr SizeType const kMaxSize     = static_cast<usize>(-2);
        static constexpr SizeType const kMaxElements = kMaxSize / sizeof(ValueType);

    public:
        ArrayBase() noexcept = default;

        explicit ArrayBase(AllocatorType const& allocator) noexcept : mAllocator{allocator} {}

        ArrayBase(SizeType n, AllocatorType const& allocator) : mAllocator{allocator}
        {
            if (n > 0)
            {
                mpBegin    = doAllocate(n);
                mpEnd      = mpBegin;
                mpCapacity = mpBegin + n;
            }
        }

        ~ArrayBase() noexcept
        {
            if (mpBegin != nullptr)
            {
                doFree(mpBegin, capacity());
            }
        }

        WE_NODISCARD AllocatorType& getAllocator() noexcept { return mAllocator; }
        WE_NODISCARD AllocatorType const& getAllocator() const noexcept { return mAllocator; }
        void setAllocator(AllocatorType const& allocator) noexcept { mAllocator = allocator; }

    protected:
        T* mpBegin    = nullptr;
        T* mpEnd      = nullptr;
        T* mpCapacity = nullptr;
        WE_NO_UNIQUE_ADDRESS AllocatorType mAllocator{};

        WE_NODISCARD SizeType capacity() const noexcept
        {
            return static_cast<SizeType>(mpCapacity - mpBegin);
        }

        WE_NODISCARD constexpr T* doAllocate(SizeType n) noexcept
        {
            if (n == 0)
            {
                return nullptr;
            }
            WE_VERIFY(n <= kMaxElements); // always-on (R46): overflow must not silently wrap n*sizeof(T)

            void* p = AllocTraits::allocate(mAllocator, n * sizeof(T), alignof(T));
            if (p == nullptr)
            {
                memory::handleAllocationFailure(n * sizeof(T), alignof(T));
            }
            return static_cast<T*>(p);
        }

        void doFree(T* p, SizeType n) noexcept
        {
            if (p != nullptr)
            {
                AllocTraits::deallocate(mAllocator, p, n * sizeof(T), alignof(T));
            }
        }

        WE_NODISCARD SizeType getNewCapacity(SizeType currentCapacity) const noexcept
        {
            if (currentCapacity <= 1)
            {
                return 2;
            }
            // Saturate instead of wrapping SizeType (R46): doubling past kMaxElements/2 would
            // overflow to a small value -> under-allocation -> OOB writes. Clamp to kMaxElements;
            // doAllocate's WE_VERIFY then aborts deterministically if even that can't satisfy.
            if (currentCapacity > kMaxElements / 2)
            {
                return kMaxElements;
            }
            return currentCapacity * 2;
        }
    };

    // Dynamic, contiguous, allocator-aware array (the engine's vector). Growth relocates
    // the live range into a fresh buffer via memory_util::relocate, so a trivially
    // relocatable element type grows with a single memcpy instead of a move+destroy
    // loop. Move steals the buffer unconditionally (the allocator is always-equal, so
    // moves/swaps are noexcept). Reference/iterator invalidation: any growth (push past
    // capacity, reserve, insert causing realloc, shrinkToFit) invalidates all; erase
    // invalidates from the erased position onward.
    export template <typename T, typename Allocator = WE_DEFAULT_ALLOCATOR>
    class Array : public ArrayBase<T, Allocator>
    {
        using BaseType = ArrayBase<T, Allocator>;
        using ThisType = Array<T, Allocator>;

        using BaseType::capacity;
        using BaseType::doAllocate;
        using BaseType::doFree;
        using BaseType::getNewCapacity;
        using BaseType::mAllocator;
        using BaseType::mpBegin;
        using BaseType::mpCapacity;
        using BaseType::mpEnd;

        using AllocTraits = AllocatorTraits<Allocator>;

    public:
        using ValueType      = T;
        using SizeType       = typename BaseType::SizeType;
        using DifferenceType = typename BaseType::DifferenceType;
        using AllocatorType  = typename BaseType::AllocatorType;

        using Pointer        = T*;
        using ConstPointer   = T const*;
        using Reference      = T&;
        using ConstReference = T const&;

        using Iterator         = T*;
        using ConstIterator    = T const*;
        using ReverseIter      = ReverseIterator<Iterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        static constexpr SizeType const NPos = BaseType::NPos;

        static_assert(!IsConst<ValueType>, "Array<T>::ValueType must be non-const");
        static_assert(!IsVolatile<ValueType>, "Array<T>::ValueType must be non-volatile");

        // --- construction / destruction ---------------------------------------

        Array() noexcept(IsNothrowDefaultConstructible<AllocatorType>) : BaseType{} {}

        explicit Array(AllocatorType const& allocator) noexcept : BaseType{allocator} {}

        // n value-initialized elements (zeroes trivial T).
        explicit Array(SizeType n, AllocatorType const& allocator = AllocatorType{}) : BaseType{n, allocator}
        {
            uninitializedValueConstruct(mAllocator, mpBegin, mpBegin + n);
            mpEnd = mpBegin + n;
        }

        // n copies of value.
        Array(SizeType n, ConstReference value, AllocatorType const& allocator = AllocatorType{})
            : BaseType{n, allocator}
        {
            uninitializedFillN(mAllocator, mpBegin, n, value);
            mpEnd = mpBegin + n;
        }

        Array(std::initializer_list<T> init, AllocatorType const& allocator = AllocatorType{})
            : BaseType{init.size(), allocator}
        {
            mpEnd = uninitializedCopy(mAllocator, init.begin(), init.end(), mpBegin);
        }

        Array(ThisType const& other)
            : BaseType{AllocTraits::selectOnContainerCopyConstruction(other.mAllocator)}
        {
            SizeType const n = other.size();
            if (n > 0)
            {
                mpBegin    = doAllocate(n);
                mpCapacity = mpBegin + n;
                mpEnd      = uninitializedCopy(mAllocator, other.mpBegin, other.mpEnd, mpBegin);
            }
        }

        Array(ThisType&& other) noexcept : BaseType{}
        {
            mpBegin       = other.mpBegin;
            mpEnd         = other.mpEnd;
            mpCapacity    = other.mpCapacity;
            mAllocator    = other.mAllocator;
            other.mpBegin = other.mpEnd = other.mpCapacity = nullptr;
        }

        ~Array() noexcept { destroyRange(mAllocator, mpBegin, mpEnd); }

        ThisType& operator=(ThisType const& other)
        {
            if (this != &other)
            {
                destroyRange(mAllocator, mpBegin, mpEnd);
                mpEnd = mpBegin;
                if constexpr (AllocTraits::propagateOnContainerCopyAssignment)
                {
                    mAllocator = other.mAllocator;
                }
                SizeType const n = other.size();
                if (n > capacity())
                {
                    if (mpBegin)
                    {
                        doFree(mpBegin, capacity());
                    }
                    mpBegin    = doAllocate(n);
                    mpCapacity = mpBegin + n;
                }
                mpEnd = uninitializedCopy(mAllocator, other.mpBegin, other.mpEnd, mpBegin);
            }
            return *this;
        }

        ThisType& operator=(ThisType&& other) noexcept
        {
            if (this != &other)
            {
                destroyRange(mAllocator, mpBegin, mpEnd);
                if (mpBegin)
                {
                    doFree(mpBegin, capacity());
                }
                mpBegin    = other.mpBegin;
                mpEnd      = other.mpEnd;
                mpCapacity = other.mpCapacity;
                if constexpr (AllocTraits::propagateOnContainerMoveAssignment)
                {
                    mAllocator = other.mAllocator;
                }
                other.mpBegin = other.mpEnd = other.mpCapacity = nullptr;
            }
            return *this;
        }

        ThisType& operator=(std::initializer_list<T> init)
        {
            assign(init.begin(), init.end());
            return *this;
        }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD SizeType size() const noexcept { return static_cast<SizeType>(mpEnd - mpBegin); }
        WE_NODISCARD bool empty() const noexcept { return mpBegin == mpEnd; }
        WE_NODISCARD SizeType capacity() const noexcept { return BaseType::capacity(); }

        void reserve(SizeType n)
        {
            if (n > capacity())
            {
                reallocate(n);
            }
        }

        void shrinkToFit()
        {
            if (capacity() > size())
            {
                if (size() == 0)
                {
                    doFree(mpBegin, capacity());
                    mpBegin = mpEnd = mpCapacity = nullptr;
                }
                else
                {
                    reallocate(size());
                }
            }
        }

        // --- element access ----------------------------------------------------

        WE_NODISCARD Reference operator[](SizeType i) noexcept
        {
            WE_ASSERT(i < size());
            return mpBegin[i];
        }
        WE_NODISCARD ConstReference operator[](SizeType i) const noexcept
        {
            WE_ASSERT(i < size());
            return mpBegin[i];
        }

        WE_NODISCARD Reference at(SizeType i) noexcept
        {
            WE_ASSERT(i < size());
            return mpBegin[i];
        }
        WE_NODISCARD ConstReference at(SizeType i) const noexcept
        {
            WE_ASSERT(i < size());
            return mpBegin[i];
        }

        WE_NODISCARD Reference front() noexcept
        {
            WE_ASSERT(!empty());
            return *mpBegin;
        }
        WE_NODISCARD ConstReference front() const noexcept
        {
            WE_ASSERT(!empty());
            return *mpBegin;
        }
        WE_NODISCARD Reference back() noexcept
        {
            WE_ASSERT(!empty());
            return *(mpEnd - 1);
        }
        WE_NODISCARD ConstReference back() const noexcept
        {
            WE_ASSERT(!empty());
            return *(mpEnd - 1);
        }

        WE_NODISCARD Pointer data() noexcept { return mpBegin; }
        WE_NODISCARD ConstPointer data() const noexcept { return mpBegin; }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return mpBegin; }
        WE_NODISCARD ConstIterator begin() const noexcept { return mpBegin; }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return mpBegin; }
        WE_NODISCARD Iterator end() noexcept { return mpEnd; }
        WE_NODISCARD ConstIterator end() const noexcept { return mpEnd; }
        WE_NODISCARD ConstIterator cend() const noexcept { return mpEnd; }

        WE_NODISCARD ReverseIter rbegin() noexcept { return ReverseIter(mpEnd); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(mpEnd); }
        WE_NODISCARD ConstReverseIter crbegin() const noexcept { return ConstReverseIter(mpEnd); }
        WE_NODISCARD ReverseIter rend() noexcept { return ReverseIter(mpBegin); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(mpBegin); }
        WE_NODISCARD ConstReverseIter crend() const noexcept { return ConstReverseIter(mpBegin); }

        // --- modifiers: back ---------------------------------------------------

        template <typename... Args>
        WE_FORCEINLINE Reference emplaceBack(Args&&... args)
        {
            // Force-inlined, grow path INLINE (not an out-of-line &this call): that lets the
            // optimizer promote mpBegin/mpEnd/mpCapacity out of the Array object into registers
            // across a caller's push loop, matching std::vector. An out-of-line grow call would
            // pin those members in memory and force a per-element reload/spill (R42).
            if (mpEnd == mpCapacity) [[unlikely]]
            {
                SizeType const oldSize = size();
                SizeType const newCap  = getNewCapacity(capacity());
                T* const newBegin      = doAllocate(newCap);
                // Construct the new element FIRST (args may alias an existing element; the old
                // buffer is still live here), then relocate the old range.
                AllocTraits::construct(mAllocator, newBegin + oldSize, worse::core::forward<Args>(args)...);
                relocate(mAllocator, mpBegin, mpEnd, newBegin);
                if (mpBegin)
                {
                    doFree(mpBegin, capacity());
                }
                mpBegin    = newBegin;
                mpEnd      = newBegin + oldSize + 1;
                mpCapacity = newBegin + newCap;
                return mpBegin[oldSize];
            }
            AllocTraits::construct(mAllocator, mpEnd, worse::core::forward<Args>(args)...);
            return *mpEnd++;
        }

        WE_FORCEINLINE void pushBack(ConstReference value) { emplaceBack(value); }
        WE_FORCEINLINE void pushBack(T&& value) { emplaceBack(worse::core::move(value)); }

        void popBack() noexcept
        {
            WE_ASSERT(!empty());
            --mpEnd;
            destroyAt(mAllocator, mpEnd);
        }

        // --- modifiers: arbitrary position -------------------------------------

        template <typename... Args>
        Iterator emplace(ConstIterator pos, Args&&... args)
        {
            SizeType const index = static_cast<SizeType>(pos - mpBegin);
            if (mpEnd == mpCapacity)
            {
                insertWithRealloc(index, worse::core::forward<Args>(args)...);
            }
            else if (index == size())
            {
                AllocTraits::construct(mAllocator, mpEnd, worse::core::forward<Args>(args)...);
                ++mpEnd;
            }
            else
            {
                // Build the new value first (handles aliasing + arbitrary args), then
                // open a one-slot gap by relocating the tail right by one.
                T temp(worse::core::forward<Args>(args)...);
                AllocTraits::construct(mAllocator, mpEnd, worse::core::move(*(mpEnd - 1)));
                worse::core::moveBackward(mpBegin + index, mpEnd - 1, mpEnd);
                ++mpEnd;
                mpBegin[index] = worse::core::move(temp);
            }
            return mpBegin + index;
        }

        Iterator insert(ConstIterator pos, ConstReference value) { return emplace(pos, value); }
        Iterator insert(ConstIterator pos, T&& value) { return emplace(pos, worse::core::move(value)); }

        Iterator erase(ConstIterator pos) noexcept
        {
            WE_ASSERT(pos >= mpBegin && pos < mpEnd); // in-range, not end() (R45)
            SizeType const index = static_cast<SizeType>(pos - mpBegin);
            worse::core::move(mpBegin + index + 1, mpEnd, mpBegin + index);
            --mpEnd;
            destroyAt(mAllocator, mpEnd);
            return mpBegin + index;
        }

        Iterator erase(ConstIterator first, ConstIterator last) noexcept
        {
            WE_ASSERT(first >= mpBegin && last <= mpEnd && first <= last); // valid subrange (R45)
            SizeType const i = static_cast<SizeType>(first - mpBegin);
            SizeType const j = static_cast<SizeType>(last - mpBegin);
            if (i != j)
            {
                T* const newEnd = worse::core::move(mpBegin + j, mpEnd, mpBegin + i);
                destroyRange(mAllocator, newEnd, mpEnd);
                mpEnd = newEnd;
            }
            return mpBegin + i;
        }

        // O(1) erase that does NOT preserve order: overwrite the slot with the last
        // element and pop. The game-idiom erase for unordered bags.
        Iterator eraseUnsorted(ConstIterator pos) noexcept
        {
            WE_ASSERT(pos >= mpBegin && pos < mpEnd); // in-range, not end() (R45)
            SizeType const index = static_cast<SizeType>(pos - mpBegin);
            T* const last        = mpEnd - 1;
            if (mpBegin + index != last)
            {
                mpBegin[index] = worse::core::move(*last);
            }
            destroyAt(mAllocator, last);
            --mpEnd;
            return mpBegin + index;
        }

        // --- modifiers: bulk ---------------------------------------------------

        void clear() noexcept
        {
            destroyRange(mAllocator, mpBegin, mpEnd);
            mpEnd = mpBegin;
        }

        void resize(SizeType n)
        {
            SizeType const s = size();
            if (n < s)
            {
                destroyRange(mAllocator, mpBegin + n, mpEnd);
                mpEnd = mpBegin + n;
            }
            else if (n > s)
            {
                reserve(n);
                uninitializedValueConstruct(mAllocator, mpEnd, mpBegin + n);
                mpEnd = mpBegin + n;
            }
        }

        void resize(SizeType n, ConstReference value)
        {
            SizeType const s = size();
            if (n < s)
            {
                destroyRange(mAllocator, mpBegin + n, mpEnd);
                mpEnd = mpBegin + n;
            }
            else if (n > s)
            {
                reserve(n);
                uninitializedFill(mAllocator, mpEnd, mpBegin + n, value);
                mpEnd = mpBegin + n;
            }
        }

        void assign(SizeType n, ConstReference value)
        {
            clear();
            if (n > capacity())
            {
                reallocate(n);
            }
            uninitializedFillN(mAllocator, mpBegin, n, value);
            mpEnd = mpBegin + n;
        }

        template <typename InIt>
            requires InputIterator<InIt>
        void assign(InIt first, InIt last)
        {
            clear();
            for (; first != last; ++first)
            {
                emplaceBack(*first);
            }
        }

        void swap(ThisType& other) noexcept
        {
            worse::core::swap(mpBegin, other.mpBegin);
            worse::core::swap(mpEnd, other.mpEnd);
            worse::core::swap(mpCapacity, other.mpCapacity);
            if constexpr (AllocTraits::propagateOnContainerSwap)
            {
                worse::core::swap(mAllocator, other.mAllocator);
            }
        }

    private:
        // Reallocate to exactly newCapacity (>= size), relocating the live range.
        void reallocate(SizeType newCapacity)
        {
            SizeType const oldSize = size();
            T* const newBegin      = doAllocate(newCapacity);
            relocate(mAllocator, mpBegin, mpEnd, newBegin);
            if (mpBegin)
            {
                doFree(mpBegin, capacity());
            }
            mpBegin    = newBegin;
            mpEnd      = newBegin + oldSize;
            mpCapacity = newBegin + newCapacity;
        }

        // Insert at index when a grow is required: allocate, construct the new element
        // in place, relocate the two surrounding segments into the fresh buffer.
        template <typename... Args>
        void insertWithRealloc(SizeType index, Args&&... args)
        {
            SizeType const oldSize = size();
            SizeType newCap        = getNewCapacity(capacity());
            if (newCap < oldSize + 1)
            {
                newCap = oldSize + 1;
            }
            T* const newBegin = doAllocate(newCap);
            AllocTraits::construct(mAllocator, newBegin + index, worse::core::forward<Args>(args)...);
            relocate(mAllocator, mpBegin, mpBegin + index, newBegin);
            relocate(mAllocator, mpBegin + index, mpEnd, newBegin + index + 1);
            if (mpBegin)
            {
                doFree(mpBegin, capacity());
            }
            mpBegin    = newBegin;
            mpEnd      = newBegin + oldSize + 1;
            mpCapacity = newBegin + newCap;
        }
    };

    export template <typename T, typename Allocator>
    void swap(Array<T, Allocator>& a, Array<T, Allocator>& b) noexcept
    {
        a.swap(b);
    }

} // namespace worse::core::container
