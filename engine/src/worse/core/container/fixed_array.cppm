module;

#include "worse/core/macro.hpp"

#include <cstddef>          // std::byte
#include <initializer_list> //
#include <memory>           // std::construct_at, std::destroy_at (no allocator here)

export module worse.core.container.fixed_array;
import worse.core.basic_type;
import worse.core.intrinsics;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.algorithm.modifying;

export namespace worse::core::container
{
    /**
     * \brief Fixed-capacity array with a RUNTIME size (the engine's `fixed_vector`): up to N
     *        elements live in an inline, properly-aligned byte buffer, with zero heap allocation.
     * \ingroup ctr_contiguous
     *
     * Being allocator-less, element lifetime is managed directly with
     * `std::construct_at`/`destroy_at`.
     * \note Overflow past N is fail-closed: the plain push/emplace/insert/resize/ctor paths
     *       abort via `WE_VERIFY` in EVERY build (no heap spill -- spill is a deferred opt-in);
     *       callers that expect overflow use the non-aborting tryPushBack/tryEmplaceBack (return
     *       nullptr/false) and can pre-check full()/remaining(). (R46)
     * \note Size is held as `usize mSize`, NOT a pointer into the buffer, so the object is
     *       byte-relocatable: when T is trivially relocatable, `FixedArray<T,N>` is too (declared
     *       below), and an `Array<FixedArray<...>>` grows by memcpy.
     */
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

        /**
         * \brief Construct with \p n value-initialized elements.
         * \pre \p n <= N (always-on `WE_VERIFY`). (R46)
         */
        explicit FixedArray(SizeType n)
        {
            WE_VERIFY(n <= N);
            for (SizeType i = 0; i < n; ++i)
            {
                std::construct_at(ptr() + i);
            }
            mSize = n;
        }

        /**
         * \brief Construct with \p n copies of \p value.
         * \pre \p n <= N (always-on `WE_VERIFY`). (R46)
         */
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
        /** \brief True when size() == capacity() (no free slots remain). */
        WE_NODISCARD bool full() const noexcept { return mSize == N; }
        WE_NODISCARD constexpr SizeType capacity() const noexcept { return N; }
        /** \brief Number of free slots, i.e. `N - size()`. \note Overflow pre-check seam. (R46) */
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

        /**
         * \brief Construct an element in place at the end.
         * \tparam Args constructor argument types for `T`.
         * \param args forwarded to `T`'s constructor.
         * \return reference to the newly constructed element.
         * \pre `size() < N`; overflow aborts via always-on `WE_VERIFY`, no heap spill. (R46)
         */
        template <typename... Args>
        Reference emplaceBack(Args&&... args)
        {
            WE_VERIFY(mSize < N); // hard cap -- always-on abort (R46), no heap spill
            T* const slot = ptr() + mSize;
            std::construct_at(slot, worse::core::forward<Args>(args)...);
            ++mSize;
            return *slot;
        }

        /** \brief Append \p value at the end (copy). \pre `size() < N` (always-on `WE_VERIFY`). (R46) */
        void pushBack(ConstReference value) { emplaceBack(value); }
        /** \brief Append \p value at the end (move). \pre `size() < N` (always-on `WE_VERIFY`). (R46) */
        void pushBack(T&& value) { emplaceBack(worse::core::move(value)); }

        /**
         * \brief Non-aborting emplace-at-end: construct an element only if room remains.
         * \tparam Args constructor argument types for `T`.
         * \param args forwarded to `T`'s constructor.
         * \return pointer to the constructed element, or nullptr when full().
         * \note For callers that treat overflow as an expected, handled outcome rather than a
         *       precondition violation; the plain emplaceBack/pushBack abort instead. (R46)
         */
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

        /** \brief Non-aborting append (copy). \return true if appended, false when full(). (R46) */
        WE_NODISCARD bool tryPushBack(ConstReference value) { return tryEmplaceBack(value) != nullptr; }
        /** \brief Non-aborting append (move). \return true if appended, false when full(). (R46) */
        WE_NODISCARD bool tryPushBack(T&& value) { return tryEmplaceBack(worse::core::move(value)) != nullptr; }

        /**
         * \brief Remove the last element.
         * \pre `!empty()` (debug `WE_ASSERT`).
         */
        void popBack() noexcept
        {
            WE_ASSERT(!empty());
            --mSize;
            std::destroy_at(ptr() + mSize);
        }

        /**
         * \brief Construct an element in place before \p pos, shifting the tail right by one.
         * \tparam Args constructor argument types for `T`.
         * \param pos iterator in [begin(), end()] marking the insertion point.
         * \param args forwarded to `T`'s constructor.
         * \return iterator to the newly inserted element.
         * \pre `size() < N` (always-on `WE_VERIFY`). (R46)
         * \note O(1) at the end; O(n) mid-sequence (element shift).
         */
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

        /** \brief Insert \p value before \p pos (copy); see emplace(). \return iterator to the inserted element. */
        Iterator insert(ConstIterator pos, ConstReference value) { return emplace(pos, value); }
        /** \brief Insert \p value before \p pos (move); see emplace(). \return iterator to the inserted element. */
        Iterator insert(ConstIterator pos, T&& value) { return emplace(pos, worse::core::move(value)); }

        /**
         * \brief Erase the element at \p pos, shifting the tail left to fill the gap.
         * \param pos iterator to the element to erase.
         * \return iterator to the element that followed \p pos.
         * \pre \p pos in [begin(), end()), i.e. in-range and not end() (debug `WE_ASSERT`). (R45)
         * \note O(n).
         */
        Iterator erase(ConstIterator pos) noexcept
        {
            WE_ASSERT(pos >= ptr() && pos < ptr() + mSize); // in-range, not end() (R45)
            SizeType const index = static_cast<SizeType>(pos - ptr());
            worse::core::move(ptr() + index + 1, ptr() + mSize, ptr() + index);
            --mSize;
            std::destroy_at(ptr() + mSize);
            return ptr() + index;
        }

        /**
         * \brief Erase the element at \p pos without preserving order: overwrite it with the
         *        last element, then pop.
         * \param pos iterator to the element to erase.
         * \return iterator to the slot \p pos occupied (now holding the moved-in last element, or end()).
         * \pre \p pos in [begin(), end()), i.e. in-range and not end() (debug `WE_ASSERT`). (R45)
         * \note O(1).
         */
        Iterator eraseUnsorted(ConstIterator pos) noexcept
        {
            WE_ASSERT(pos >= ptr() && pos < ptr() + mSize); // in-range, not end() (R45)
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

        /** \brief Destroy all elements; size becomes 0. */
        void clear() noexcept { destroyAll(); }

        /**
         * \brief Resize to \p n elements, value-initializing any new ones.
         * \param n new size; trailing elements are destroyed when \p n < size().
         * \pre \p n <= N (always-on `WE_VERIFY`). (R46)
         */
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

        /**
         * \brief Resize to \p n elements, copy-filling any new ones from \p value.
         * \param n new size.
         * \param value element copied into each newly added slot.
         * \pre \p n <= N (always-on `WE_VERIFY`). (R46)
         */
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

        /**
         * \brief Replace the contents with \p n copies of \p value.
         * \param n number of elements after the call.
         * \param value element copied into every slot.
         * \pre \p n <= N (always-on `WE_VERIFY`). (R46)
         */
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

        /**
         * \brief Swap contents with \p other.
         * \param other array to swap with.
         * \note Swaps the common prefix in place, then relocates the longer array's tail.
         */
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
                intrinsics::memCopy(
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

    /** \brief Free-function swap: exchanges the contents of \p a and \p b. */
    template <typename T, usize N>
    void swap(FixedArray<T, N>& a, FixedArray<T, N>& b) noexcept
    {
        a.swap(b);
    }
} // namespace worse::core::container

namespace worse::core
{
    /**
     * \brief `FixedArray<T,N>` is byte-relocatable exactly when its element type is.
     *
     * Size is an index, not a self-pointer, so the bytes carry no internal references. (R7)
     * \note Lets containers memcpy-relocate a `FixedArray`-of-POD.
     */
    template <typename T, usize N>
    struct WeIsTriviallyRelocatable<worse::core::container::FixedArray<T, N>>
    {
        static constexpr bool value = IsTriviallyRelocatable<T>;
    };
} // namespace worse::core
