module;

#include "worse/core/macro.hpp"

#include <cstddef>          // std::byte
#include <initializer_list> //
#include <memory>           // std::construct_at, std::destroy_at (no allocator here)

export module worse.core.container.fixed_array;
import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.algorithm.modifying;

// Fixed-capacity array with a RUNTIME size (the engine's fixed_vector): up to N elements
// live in an inline, properly-aligned byte buffer -- zero heap allocation, the whole
// point. Overflow past N is fail-closed: the plain push/emplace/insert/resize/ctor paths
// abort via WE_VERIFY in EVERY build (R46, no heap spill -- spill is a deferred opt-in);
// callers that expect overflow use the non-aborting tryPushBack/tryEmplaceBack (return
// nullptr/false) and can pre-check full()/remaining(). Size is held as `usize mSize`, NOT a pointer into the buffer, so the
// object is byte-relocatable: when T is trivially relocatable, FixedArray<T,N> is too
// (declared below), and an Array<FixedArray<...>> grows by memcpy. Being allocator-less,
// element lifetime is managed directly with std::construct_at/destroy_at.
export namespace worse::core::container
{
    template <typename T, usize N>
    class FixedArray
    {
    public:
        using ValueType        = T;
        using SizeType         = usize;
        using DifferenceType   = isize;
        using Reference        = T&;
        using ConstReference   = T const&;
        using Pointer          = T*;
        using ConstPointer     = T const*;
        using Iterator         = T*;
        using ConstIterator    = T const*;
        using ReverseIter      = ReverseIterator<Iterator>;
        using ConstReverseIter = ReverseIterator<ConstIterator>;

        static constexpr SizeType kCapacity = N;

        static_assert(N > 0, "FixedArray requires N > 0");
        static_assert(!IsConst<T>, "FixedArray<T>: T must be non-const");

        // --- construction / destruction ---------------------------------------

        FixedArray() noexcept = default;

        explicit FixedArray(SizeType n)
        {
            WE_VERIFY(n <= N);
            for (SizeType i = 0; i < n; ++i)
            {
                std::construct_at(ptr() + i);
            }
            mSize = n;
        }

        FixedArray(SizeType n, ConstReference value)
        {
            WE_VERIFY(n <= N);
            for (SizeType i = 0; i < n; ++i)
            {
                std::construct_at(ptr() + i, value);
            }
            mSize = n;
        }

        FixedArray(std::initializer_list<T> init)
        {
            WE_VERIFY(init.size() <= N);
            SizeType i = 0;
            for (T const& v : init)
            {
                std::construct_at(ptr() + i, v);
                ++i;
            }
            mSize = init.size();
        }

        FixedArray(FixedArray const& other)
        {
            for (SizeType i = 0; i < other.mSize; ++i)
            {
                std::construct_at(ptr() + i, other.ptr()[i]);
            }
            mSize = other.mSize;
        }

        FixedArray(FixedArray&& other) noexcept
        {
            relocateFrom(other);
        }

        ~FixedArray() { destroyAll(); }

        FixedArray& operator=(FixedArray const& other)
        {
            if (this != &other)
            {
                destroyAll();
                for (SizeType i = 0; i < other.mSize; ++i)
                {
                    std::construct_at(ptr() + i, other.ptr()[i]);
                }
                mSize = other.mSize;
            }
            return *this;
        }

        FixedArray& operator=(FixedArray&& other) noexcept
        {
            if (this != &other)
            {
                destroyAll();
                relocateFrom(other);
            }
            return *this;
        }

        FixedArray& operator=(std::initializer_list<T> init)
        {
            WE_VERIFY(init.size() <= N);
            destroyAll();
            SizeType i = 0;
            for (T const& v : init)
            {
                std::construct_at(ptr() + i, v);
                ++i;
            }
            mSize = init.size();
            return *this;
        }

        // --- capacity ----------------------------------------------------------

        WE_NODISCARD SizeType size() const noexcept { return mSize; }
        WE_NODISCARD bool empty() const noexcept { return mSize == 0; }
        WE_NODISCARD bool full() const noexcept { return mSize == N; }
        WE_NODISCARD constexpr SizeType capacity() const noexcept { return N; }
        WE_NODISCARD SizeType remaining() const noexcept { return N - mSize; } // free slots (R46 pre-check)

        // --- element access ----------------------------------------------------

        WE_NODISCARD Reference operator[](SizeType i) noexcept
        {
            WE_ASSERT(i < mSize);
            return ptr()[i];
        }
        WE_NODISCARD ConstReference operator[](SizeType i) const noexcept
        {
            WE_ASSERT(i < mSize);
            return ptr()[i];
        }
        WE_NODISCARD Reference at(SizeType i) noexcept
        {
            WE_ASSERT(i < mSize);
            return ptr()[i];
        }
        WE_NODISCARD ConstReference at(SizeType i) const noexcept
        {
            WE_ASSERT(i < mSize);
            return ptr()[i];
        }

        WE_NODISCARD Reference front() noexcept
        {
            WE_ASSERT(!empty());
            return ptr()[0];
        }
        WE_NODISCARD ConstReference front() const noexcept
        {
            WE_ASSERT(!empty());
            return ptr()[0];
        }
        WE_NODISCARD Reference back() noexcept
        {
            WE_ASSERT(!empty());
            return ptr()[mSize - 1];
        }
        WE_NODISCARD ConstReference back() const noexcept
        {
            WE_ASSERT(!empty());
            return ptr()[mSize - 1];
        }

        WE_NODISCARD Pointer data() noexcept { return ptr(); }
        WE_NODISCARD ConstPointer data() const noexcept { return ptr(); }

        // --- iterators ---------------------------------------------------------

        WE_NODISCARD Iterator begin() noexcept { return ptr(); }
        WE_NODISCARD ConstIterator begin() const noexcept { return ptr(); }
        WE_NODISCARD ConstIterator cbegin() const noexcept { return ptr(); }
        WE_NODISCARD Iterator end() noexcept { return ptr() + mSize; }
        WE_NODISCARD ConstIterator end() const noexcept { return ptr() + mSize; }
        WE_NODISCARD ConstIterator cend() const noexcept { return ptr() + mSize; }

        WE_NODISCARD ReverseIter rbegin() noexcept { return ReverseIter(ptr() + mSize); }
        WE_NODISCARD ConstReverseIter rbegin() const noexcept { return ConstReverseIter(ptr() + mSize); }
        WE_NODISCARD ReverseIter rend() noexcept { return ReverseIter(ptr()); }
        WE_NODISCARD ConstReverseIter rend() const noexcept { return ConstReverseIter(ptr()); }

        // --- modifiers ---------------------------------------------------------

        template <typename... Args>
        Reference emplaceBack(Args&&... args)
        {
            WE_VERIFY(mSize < N); // hard cap -- always-on abort (R46), no heap spill
            T* const slot = ptr() + mSize;
            std::construct_at(slot, worse::core::forward<Args>(args)...);
            ++mSize;
            return *slot;
        }

        void pushBack(ConstReference value) { emplaceBack(value); }
        void pushBack(T&& value) { emplaceBack(worse::core::move(value)); }

        // Non-aborting overflow path (R46): returns the constructed element, or nullptr/false
        // when full -- for callers that treat overflow as an expected, handled outcome rather
        // than a precondition violation. The plain emplaceBack/pushBack above abort (WE_VERIFY).
        template <typename... Args>
        WE_NODISCARD Pointer tryEmplaceBack(Args&&... args)
        {
            if (mSize == N)
            {
                return nullptr;
            }
            T* const slot = ptr() + mSize;
            std::construct_at(slot, worse::core::forward<Args>(args)...);
            ++mSize;
            return slot;
        }

        WE_NODISCARD bool tryPushBack(ConstReference value) { return tryEmplaceBack(value) != nullptr; }
        WE_NODISCARD bool tryPushBack(T&& value) { return tryEmplaceBack(worse::core::move(value)) != nullptr; }

        void popBack() noexcept
        {
            WE_ASSERT(!empty());
            --mSize;
            std::destroy_at(ptr() + mSize);
        }

        template <typename... Args>
        Iterator emplace(ConstIterator pos, Args&&... args)
        {
            WE_VERIFY(mSize < N); // hard cap (R46)
            SizeType const index = static_cast<SizeType>(pos - ptr());
            if (index == mSize)
            {
                emplaceBack(worse::core::forward<Args>(args)...);
                return ptr() + index;
            }
            T temp(worse::core::forward<Args>(args)...);
            std::construct_at(ptr() + mSize, worse::core::move(ptr()[mSize - 1]));
            worse::core::moveBackward(ptr() + index, ptr() + mSize - 1, ptr() + mSize);
            ++mSize;
            ptr()[index] = worse::core::move(temp);
            return ptr() + index;
        }

        Iterator insert(ConstIterator pos, ConstReference value) { return emplace(pos, value); }
        Iterator insert(ConstIterator pos, T&& value) { return emplace(pos, worse::core::move(value)); }

        Iterator erase(ConstIterator pos) noexcept
        {
            SizeType const index = static_cast<SizeType>(pos - ptr());
            worse::core::move(ptr() + index + 1, ptr() + mSize, ptr() + index);
            --mSize;
            std::destroy_at(ptr() + mSize);
            return ptr() + index;
        }

        // O(1) order-not-preserved erase: overwrite with the last element and pop.
        Iterator eraseUnsorted(ConstIterator pos) noexcept
        {
            SizeType const index = static_cast<SizeType>(pos - ptr());
            T* const last        = ptr() + mSize - 1;
            if (ptr() + index != last)
            {
                ptr()[index] = worse::core::move(*last);
            }
            std::destroy_at(last);
            --mSize;
            return ptr() + index;
        }

        void clear() noexcept { destroyAll(); }

        void resize(SizeType n)
        {
            WE_VERIFY(n <= N);
            if (n < mSize)
            {
                for (SizeType i = n; i < mSize; ++i)
                {
                    std::destroy_at(ptr() + i);
                }
            }
            else
            {
                for (SizeType i = mSize; i < n; ++i)
                {
                    std::construct_at(ptr() + i);
                }
            }
            mSize = n;
        }

        void resize(SizeType n, ConstReference value)
        {
            WE_VERIFY(n <= N);
            if (n < mSize)
            {
                for (SizeType i = n; i < mSize; ++i)
                {
                    std::destroy_at(ptr() + i);
                }
            }
            else
            {
                for (SizeType i = mSize; i < n; ++i)
                {
                    std::construct_at(ptr() + i, value);
                }
            }
            mSize = n;
        }

        void assign(SizeType n, ConstReference value)
        {
            WE_VERIFY(n <= N);
            destroyAll();
            for (SizeType i = 0; i < n; ++i)
            {
                std::construct_at(ptr() + i, value);
            }
            mSize = n;
        }

        void swap(FixedArray& other) noexcept
        {
            SizeType const common = mSize < other.mSize ? mSize : other.mSize;
            for (SizeType i = 0; i < common; ++i)
            {
                worse::core::swap(ptr()[i], other.ptr()[i]);
            }
            // Relocate the longer container's tail into the shorter one.
            if (mSize < other.mSize)
            {
                for (SizeType i = common; i < other.mSize; ++i)
                {
                    std::construct_at(ptr() + i, worse::core::move(other.ptr()[i]));
                    std::destroy_at(other.ptr() + i);
                }
            }
            else if (other.mSize < mSize)
            {
                for (SizeType i = common; i < mSize; ++i)
                {
                    std::construct_at(other.ptr() + i, worse::core::move(ptr()[i]));
                    std::destroy_at(ptr() + i);
                }
            }
            worse::core::swap(mSize, other.mSize);
        }

    private:
        alignas(T) std::byte mBuffer[sizeof(T) * N];
        usize mSize = 0;

        WE_NODISCARD Pointer ptr() noexcept { return reinterpret_cast<Pointer>(mBuffer); }
        WE_NODISCARD ConstPointer ptr() const noexcept { return reinterpret_cast<ConstPointer>(mBuffer); }

        void destroyAll() noexcept
        {
            if constexpr (!IsTriviallyDestructible<T>)
            {
                for (SizeType i = 0; i < mSize; ++i)
                {
                    std::destroy_at(ptr() + i);
                }
            }
            mSize = 0;
        }

        // Move-construct (or memcpy, when T is trivially relocatable) `other`'s live
        // range into this empty buffer, leaving `other` empty. Assumes this is empty.
        void relocateFrom(FixedArray& other) noexcept
        {
            if constexpr (IsTriviallyRelocatable<T>)
            {
                __builtin_memcpy(
                    static_cast<void*>(mBuffer),
                    static_cast<void const*>(other.mBuffer),
                    other.mSize * sizeof(T));
                mSize       = other.mSize;
                other.mSize = 0; // source bytes abandoned (relocated)
            }
            else
            {
                for (SizeType i = 0; i < other.mSize; ++i)
                {
                    std::construct_at(ptr() + i, worse::core::move(other.ptr()[i]));
                }
                mSize = other.mSize;
                other.destroyAll();
            }
        }
    };

    template <typename T, usize N>
    void swap(FixedArray<T, N>& a, FixedArray<T, N>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container

// FixedArray<T,N> is byte-relocatable exactly when its element type is (size is an index,
// not a self-pointer -- R7). This lets containers memcpy-relocate a FixedArray-of-POD.
namespace worse::core
{
    template <typename T, usize N>
    struct WeIsTriviallyRelocatable<worse::core::container::FixedArray<T, N>>
    {
        static constexpr bool value = IsTriviallyRelocatable<T>;
    };
} // namespace worse::core
