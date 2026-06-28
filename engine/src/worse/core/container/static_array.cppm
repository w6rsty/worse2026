module;

#include "worse/core/macro.hpp"

export module worse.core.container.static_array;
import worse.core.basic_type;
import worse.core.utility;
import worse.core.container.iterator;

export namespace worse::core::container
{
    /**
     * \brief Fixed-size inline array (the engine's `std::array`): N elements stored directly,
     *        no allocator, no heap, fully constexpr.
     * \ingroup ctr_contiguous
     *
     * Aggregate (public data member, no user-declared ctors) so aggregate initialization works:
     * `StaticArray<int, 3> a{1, 2, 3}`; `sizeof(StaticArray<T, N>) == sizeof(T[N])` (no overhead).
     * \note Bounds checked by a debug-only `WE_ASSERT` (release: UB, matching the engine's
     *       no-throw contract).
     */
    template <typename T, usize N>
    struct StaticArray
    {
        static_assert(N > 0, "StaticArray requires N > 0");

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

        // Public so the type stays an aggregate (enables `{...}` initialization).
        // Named with the `m` prefix per convention despite being public.
        T mData[N];

        // --- element access ---------------------------------------------------

        WE_NODISCARD constexpr Reference operator[](SizeType i) noexcept
        {
            WE_ASSERT(i < N);
            return mData[i];
        }
        WE_NODISCARD constexpr ConstReference operator[](SizeType i) const noexcept
        {
            WE_ASSERT(i < N);
            return mData[i];
        }
        WE_NODISCARD constexpr Reference at(SizeType i) noexcept
        {
            WE_ASSERT(i < N);
            return mData[i];
        }
        WE_NODISCARD constexpr ConstReference at(SizeType i) const noexcept
        {
            WE_ASSERT(i < N);
            return mData[i];
        }

        WE_NODISCARD constexpr Reference front() noexcept { return mData[0]; }
        WE_NODISCARD constexpr ConstReference front() const noexcept { return mData[0]; }
        WE_NODISCARD constexpr Reference back() noexcept { return mData[N - 1]; }
        WE_NODISCARD constexpr ConstReference back() const noexcept { return mData[N - 1]; }

        WE_NODISCARD constexpr Pointer data() noexcept { return mData; }
        WE_NODISCARD constexpr ConstPointer data() const noexcept { return mData; }

        // --- capacity ---------------------------------------------------------

        WE_NODISCARD constexpr SizeType size() const noexcept { return N; }
        WE_NODISCARD constexpr bool empty() const noexcept { return false; }

        // --- iterators --------------------------------------------------------

        WE_NODISCARD constexpr Iterator begin() noexcept { return mData; }
        WE_NODISCARD constexpr ConstIterator begin() const noexcept { return mData; }
        WE_NODISCARD constexpr ConstIterator cbegin() const noexcept { return mData; }
        WE_NODISCARD constexpr Iterator end() noexcept { return mData + N; }
        WE_NODISCARD constexpr ConstIterator end() const noexcept { return mData + N; }
        WE_NODISCARD constexpr ConstIterator cend() const noexcept { return mData + N; }

        WE_NODISCARD constexpr ReverseIter rbegin() noexcept { return ReverseIter(mData + N); }
        WE_NODISCARD constexpr ConstReverseIter rbegin() const noexcept { return ConstReverseIter(mData + N); }
        WE_NODISCARD constexpr ConstReverseIter crbegin() const noexcept { return ConstReverseIter(mData + N); }
        WE_NODISCARD constexpr ReverseIter rend() noexcept { return ReverseIter(mData); }
        WE_NODISCARD constexpr ConstReverseIter rend() const noexcept { return ConstReverseIter(mData); }
        WE_NODISCARD constexpr ConstReverseIter crend() const noexcept { return ConstReverseIter(mData); }

        // --- operations -------------------------------------------------------

        /** \brief Assign \p value to every element. */
        constexpr void fill(ConstReference value)
        {
            for (SizeType i = 0; i < N; ++i)
            {
                mData[i] = value;
            }
        }

        /** \brief Swap contents with \p other element-wise. */
        constexpr void swap(StaticArray& other)
        {
            for (SizeType i = 0; i < N; ++i)
            {
                worse::core::swap(mData[i], other.mData[i]);
            }
        }

        /**
         * \brief Element-wise equality of \p a and \p b.
         * \return true iff every element compares equal.
         * \note Hidden friend; only instantiated when used.
         */
        WE_NODISCARD friend constexpr bool operator==(StaticArray const& a, StaticArray const& b)
        {
            for (SizeType i = 0; i < N; ++i)
            {
                if (!(a.mData[i] == b.mData[i]))
                {
                    return false;
                }
            }
            return true;
        }
    };

    /** \brief Free-function swap: exchanges the contents of \p a and \p b element-wise. */
    template <typename T, usize N>
    constexpr void swap(StaticArray<T, N>& a, StaticArray<T, N>& b)
    {
        a.swap(b);
    }
} // namespace worse::core::container
