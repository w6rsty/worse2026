module;

#include "worse/core/macro.hpp"

export module worse.core.container.iterator;
import worse.core.basic_type;
import worse.core.type_traits;

// Iterator vocabulary for the container + algorithm library: the category tag
// hierarchy, the concepts that constrain algorithms (e.g. `sort` needs random
// access), `IteratorTraits` (raw `T*` is the contiguous iterator everywhere here),
// `ReverseIterator`, and the `distance`/`advance`/`next`/`prev` free functions.
//
// Lives in `worse::core` (the parent namespace), NOT `worse::core::container`, so
// the vocabulary is visible unqualified from both `worse::core::container::*` and
// `worse::core::algorithm::*` — the same rationale as type_traits/utility.
export namespace worse::core
{
    // --- category tags --------------------------------------------------------
    //
    // Empty tags forming an inheritance chain so "is at least X" is a derived->base
    // convertibility test. Contiguous refines RandomAccess (the elements are a single
    // array, so `&*it` is a real pointer into contiguous storage).
    struct InputIteratorTag
    {
    };
    struct ForwardIteratorTag : InputIteratorTag
    {
    };
    struct BidirectionalIteratorTag : ForwardIteratorTag
    {
    };
    struct RandomAccessIteratorTag : BidirectionalIteratorTag
    {
    };
    struct ContiguousIteratorTag : RandomAccessIteratorTag
    {
    };

    // --- IteratorTraits -------------------------------------------------------
    //
    // The primary template is EMPTY unless `It` actually exposes the five nested
    // typedefs. This matters: a non-empty primary that wrote `typename It::ValueType`
    // would hard-error (not SFINAE) the moment IteratorTraits<int> is named, because
    // instantiating the class instantiates every member alias. The void_t-detected
    // partial specialization keeps the trait absent for non-iterators, so the concepts
    // below fail cleanly instead of breaking the build.
    template <typename...>
    using VoidT = void;

    template <typename It, typename = void>
    struct IteratorTraits
    {
    };

    template <typename It>
    struct IteratorTraits<
        It,
        VoidT<
            typename It::IteratorCategory,
            typename It::ValueType,
            typename It::DifferenceType,
            typename It::Pointer,
            typename It::Reference>>
    {
        using IteratorCategory = typename It::IteratorCategory;
        using ValueType        = typename It::ValueType;
        using DifferenceType   = typename It::DifferenceType;
        using Pointer          = typename It::Pointer;
        using Reference        = typename It::Reference;
    };

    // Raw pointer -> contiguous iterator (matches the `Iterator = T*` typedef the
    // contiguous containers use). Pointers have no members, so only this matches.
    template <typename T>
    struct IteratorTraits<T*>
    {
        using IteratorCategory = ContiguousIteratorTag;
        using ValueType        = RemoveCv<T>;
        using DifferenceType   = isize;
        using Pointer          = T*;
        using Reference        = T&;
    };

    // --- concepts -------------------------------------------------------------
    //
    // SFINAE-friendly: `HasIteratorCategory` guards the trait access so a non-iterator
    // type fails the concept instead of triggering a hard error. "Is at least X" =
    // the advertised category tag is convertible to X's tag.
    template <typename It>
    concept HasIteratorCategory = requires { typename IteratorTraits<It>::IteratorCategory; };

    template <typename It>
    concept InputIterator =
        HasIteratorCategory<It> && IsConvertible<typename IteratorTraits<It>::IteratorCategory, InputIteratorTag>;

    template <typename It>
    concept ForwardIterator =
        InputIterator<It> && IsConvertible<typename IteratorTraits<It>::IteratorCategory, ForwardIteratorTag>;

    template <typename It>
    concept BidirectionalIterator =
        ForwardIterator<It> && IsConvertible<typename IteratorTraits<It>::IteratorCategory, BidirectionalIteratorTag>;

    template <typename It>
    concept RandomAccessIterator =
        BidirectionalIterator<It> && IsConvertible<typename IteratorTraits<It>::IteratorCategory, RandomAccessIteratorTag>;

    template <typename It>
    concept ContiguousIterator =
        RandomAccessIterator<It> && IsConvertible<typename IteratorTraits<It>::IteratorCategory, ContiguousIteratorTag>;

    // --- addressOf ------------------------------------------------------------
    //
    // True address of an object, immune to an overloaded unary `operator&`. Needed by
    // ReverseIterator::operator->; reused by memory_util. (__builtin_addressof is the
    // portable intrinsic clang/gcc/msvc all provide and is constexpr-usable.)
    template <typename T>
    WE_NODISCARD constexpr T* addressOf(T& arg) noexcept
    {
        return __builtin_addressof(arg);
    }
    template <typename T>
    T const* addressOf(T const&&) = delete; // never take the address of an rvalue

    // --- ReverseIterator ------------------------------------------------------
    //
    // Adapts an iterator so traversal runs backwards. `base()` returns the wrapped
    // iterator; dereference reads the element BEFORE base() (so `rbegin = reverse(end)`
    // dereferences the last element). Random-access ops are provided and only
    // instantiate when the underlying iterator supports them.
    template <typename It>
    class ReverseIterator
    {
    public:
        using IteratorType     = It;
        using IteratorCategory = typename IteratorTraits<It>::IteratorCategory;
        using ValueType        = typename IteratorTraits<It>::ValueType;
        using DifferenceType   = typename IteratorTraits<It>::DifferenceType;
        using Pointer          = typename IteratorTraits<It>::Pointer;
        using Reference        = typename IteratorTraits<It>::Reference;

        constexpr ReverseIterator() = default;
        constexpr explicit ReverseIterator(It current) : mCurrent(current) {}

        // Converting construction from a reverse iterator over a related iterator.
        template <typename U>
        constexpr ReverseIterator(ReverseIterator<U> const& other) : mCurrent(other.base())
        {
        }

        WE_NODISCARD constexpr It base() const { return mCurrent; }

        WE_NODISCARD constexpr Reference operator*() const
        {
            It tmp = mCurrent;
            --tmp;
            return *tmp;
        }

        WE_NODISCARD constexpr Pointer operator->() const
        {
            It tmp = mCurrent;
            --tmp;
            return addressOf(*tmp);
        }

        constexpr ReverseIterator& operator++()
        {
            --mCurrent;
            return *this;
        }
        constexpr ReverseIterator operator++(int)
        {
            ReverseIterator tmp = *this;
            --mCurrent;
            return tmp;
        }
        constexpr ReverseIterator& operator--()
        {
            ++mCurrent;
            return *this;
        }
        constexpr ReverseIterator operator--(int)
        {
            ReverseIterator tmp = *this;
            ++mCurrent;
            return tmp;
        }

        WE_NODISCARD constexpr ReverseIterator operator+(DifferenceType n) const
        {
            return ReverseIterator(mCurrent - n);
        }
        WE_NODISCARD constexpr ReverseIterator operator-(DifferenceType n) const
        {
            return ReverseIterator(mCurrent + n);
        }
        constexpr ReverseIterator& operator+=(DifferenceType n)
        {
            mCurrent -= n;
            return *this;
        }
        constexpr ReverseIterator& operator-=(DifferenceType n)
        {
            mCurrent += n;
            return *this;
        }
        WE_NODISCARD constexpr Reference operator[](DifferenceType n) const { return *(*this + n); }

    private:
        It mCurrent{};
    };

    // Comparisons reverse the sense of the underlying order (a < b iff b.base() < a.base()).
    template <typename It1, typename It2>
    WE_NODISCARD constexpr bool operator==(ReverseIterator<It1> const& a, ReverseIterator<It2> const& b)
    {
        return a.base() == b.base();
    }
    template <typename It1, typename It2>
    WE_NODISCARD constexpr bool operator!=(ReverseIterator<It1> const& a, ReverseIterator<It2> const& b)
    {
        return a.base() != b.base();
    }
    template <typename It1, typename It2>
    WE_NODISCARD constexpr bool operator<(ReverseIterator<It1> const& a, ReverseIterator<It2> const& b)
    {
        return b.base() < a.base();
    }
    template <typename It1, typename It2>
    WE_NODISCARD constexpr bool operator>(ReverseIterator<It1> const& a, ReverseIterator<It2> const& b)
    {
        return b.base() > a.base();
    }
    template <typename It1, typename It2>
    WE_NODISCARD constexpr bool operator<=(ReverseIterator<It1> const& a, ReverseIterator<It2> const& b)
    {
        return b.base() <= a.base();
    }
    template <typename It1, typename It2>
    WE_NODISCARD constexpr bool operator>=(ReverseIterator<It1> const& a, ReverseIterator<It2> const& b)
    {
        return b.base() >= a.base();
    }

    // Distance between reverse iterators (note the operand swap).
    template <typename It1, typename It2>
    WE_NODISCARD constexpr auto operator-(ReverseIterator<It1> const& a, ReverseIterator<It2> const& b)
        -> decltype(b.base() - a.base())
    {
        return b.base() - a.base();
    }

    // n + it (symmetric to it + n).
    template <typename It>
    WE_NODISCARD constexpr ReverseIterator<It>
    operator+(typename ReverseIterator<It>::DifferenceType n, ReverseIterator<It> const& it)
    {
        return it + n;
    }

    template <typename It>
    WE_NODISCARD constexpr ReverseIterator<It> makeReverseIterator(It it)
    {
        return ReverseIterator<It>(it);
    }

    // --- distance / advance / next / prev -------------------------------------
    //
    // O(1) on random-access iterators (pointer arithmetic), O(n) walk otherwise. The
    // `if constexpr` on the category tag picks the path at compile time.
    template <typename It>
    WE_NODISCARD constexpr typename IteratorTraits<It>::DifferenceType distance(It first, It last)
    {
        using Category = typename IteratorTraits<It>::IteratorCategory;
        if constexpr (IsConvertible<Category, RandomAccessIteratorTag>)
        {
            return last - first;
        }
        else
        {
            typename IteratorTraits<It>::DifferenceType n = 0;
            for (; first != last; ++first)
            {
                ++n;
            }
            return n;
        }
    }

    template <typename It, typename Distance>
    constexpr void advance(It& it, Distance n)
    {
        using Category = typename IteratorTraits<It>::IteratorCategory;
        using Diff     = typename IteratorTraits<It>::DifferenceType;
        if constexpr (IsConvertible<Category, RandomAccessIteratorTag>)
        {
            it += static_cast<Diff>(n);
        }
        else if constexpr (IsConvertible<Category, BidirectionalIteratorTag>)
        {
            Diff steps = static_cast<Diff>(n);
            for (; steps > 0; --steps)
            {
                ++it;
            }
            for (; steps < 0; ++steps)
            {
                --it;
            }
        }
        else
        {
            // input/forward: forward motion only.
            WE_ASSERT(static_cast<Diff>(n) >= 0);
            for (Diff steps = static_cast<Diff>(n); steps > 0; --steps)
            {
                ++it;
            }
        }
    }

    template <typename It>
    WE_NODISCARD constexpr It next(It it, typename IteratorTraits<It>::DifferenceType n = 1)
    {
        advance(it, n);
        return it;
    }

    template <typename It>
    WE_NODISCARD constexpr It prev(It it, typename IteratorTraits<It>::DifferenceType n = 1)
    {
        advance(it, -n);
        return it;
    }
} // namespace worse::core
