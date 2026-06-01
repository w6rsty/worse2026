module;

#include "worse/core/macro.hpp"

#include <type_traits> // std::is_nothrow_assignable_v (no in-house equivalent yet)

export module worse.core.utility;
import worse.core.basic_type;
import worse.core.type_traits;

// Foundational value-category + small-vocabulary layer for the container/algorithm
// library: the cast helpers (`move`/`forward`/`swap`/`exchange`/`moveIfNoexcept`),
// the two pair types, and the default comparison functors.
//
// Like type_traits, this lives directly in `worse::core` (not a `::utility`
// sub-namespace) so `move`, `Pair`, `Less`, … are usable unqualified throughout
// `worse::core::*`. We deliberately do NOT pull in <utility>: these are a handful
// of one-line casts, and re-implementing them keeps the module's import surface
// minimal and free of the std associated-namespace overloads we don't want via ADL.
export namespace worse::core
{
    // --- value-category casts -------------------------------------------------

    // Unconditional rvalue cast. `move(x)` says "you may pillage x".
    template <typename T>
    WE_NODISCARD constexpr RemoveReference<T>&& move(T&& value) noexcept
    {
        return static_cast<RemoveReference<T>&&>(value);
    }

    // Perfect forwarding. Preserves the value category of a forwarding-reference
    // parameter when passing it on. The lvalue overload also accepts rvalues bound
    // to a named parameter; the rvalue overload forbids forwarding<T&> on an rvalue.
    template <typename T>
    WE_NODISCARD constexpr T&& forward(RemoveReference<T>& value) noexcept
    {
        return static_cast<T&&>(value);
    }

    template <typename T>
    WE_NODISCARD constexpr T&& forward(RemoveReference<T>&& value) noexcept
    {
        static_assert(!IsReference<T>, "forward must not convert an rvalue to an lvalue reference");
        return static_cast<T&&>(value);
    }

    // Move when moving cannot throw (or no copy exists), else copy. This is what a
    // container's grow loop uses to keep the strong guarantee for throwing-move
    // types: it returns `T const&` (forcing a copy) when the move could throw and a
    // copy is available, and `T&&` otherwise.
    template <typename T>
    WE_NODISCARD constexpr Conditional<!IsNothrowMoveConstructible<T> && IsCopyConstructible<T>, T const&, T&&>
    moveIfNoexcept(T& value) noexcept
    {
        return worse::core::move(value);
    }

    // --- swap -----------------------------------------------------------------
    //
    // NB: every internal call to move/forward/swap below is FULLY QUALIFIED. These
    // names collide with std::move/forward/swap, so an unqualified call whose
    // argument is a std type would pull std's overload in via ADL and become
    // ambiguous. Qualification suppresses ADL. (See PITFALLS: utility ADL clash.)

    template <typename T>
    constexpr void swap(T& a, T& b) noexcept(IsNothrowMoveConstructible<T> && IsNothrowMoveAssignable<T>)
    {
        T tmp = worse::core::move(a);
        a     = worse::core::move(b);
        b     = worse::core::move(tmp);
    }

    // Element-wise array swap (so `swap` works on the raw buffers our containers hold).
    template <typename T, usize N>
    constexpr void swap(T (&a)[N], T (&b)[N]) noexcept(noexcept(worse::core::swap(a[0], b[0])))
    {
        for (usize i = 0; i < N; ++i)
        {
            worse::core::swap(a[i], b[i]);
        }
    }

    // Assign `newValue` to `obj`, returning its previous value.
    template <typename T, typename U = T>
    WE_NODISCARD constexpr T exchange(T& obj, U&& newValue) noexcept(
        IsNothrowMoveConstructible<T> && std::is_nothrow_assignable_v<T&, U>)
    {
        T old = worse::core::move(obj);
        obj   = worse::core::forward<U>(newValue);
        return old;
    }

    // --- Pair -----------------------------------------------------------------
    //
    // Public `first`/`second` (R2): keeps STL muscle-memory and structured bindings
    // (`auto [a, b] = pair;` works because all members are public). Comparison is
    // `==` (member-wise) and `<` (lexicographic); C++20 synthesizes `!=` from `==`.

    template <typename T1, typename T2>
    struct Pair
    {
        using FirstType  = T1;
        using SecondType = T2;

        T1 first{};
        T2 second{};

        constexpr Pair()                       = default;
        constexpr Pair(Pair const&)            = default;
        constexpr Pair(Pair&&)                 = default;
        constexpr Pair& operator=(Pair const&) = default;
        constexpr Pair& operator=(Pair&&)      = default;

        constexpr Pair(T1 const& a, T2 const& b) : first(a), second(b) {}

        // Perfect-forwarding ctor. Two parameters, so it never competes with the
        // one-argument copy/move ctors above.
        template <typename U1, typename U2>
        constexpr Pair(U1&& a, U2&& b) : first(worse::core::forward<U1>(a)), second(worse::core::forward<U2>(b))
        {
        }

        template <typename U1, typename U2>
        constexpr Pair(Pair<U1, U2> const& other) : first(other.first), second(other.second)
        {
        }

        template <typename U1, typename U2>
        constexpr Pair(Pair<U1, U2>&& other)
            : first(worse::core::move(other.first)), second(worse::core::move(other.second))
        {
        }

        constexpr void swap(Pair& other) noexcept(
            noexcept(worse::core::swap(first, other.first)) && noexcept(worse::core::swap(second, other.second)))
        {
            worse::core::swap(first, other.first);
            worse::core::swap(second, other.second);
        }

        // Hidden friends: only instantiated when actually used, so a Pair of a
        // non-comparable type stays valid as long as it is never compared.
        WE_NODISCARD friend constexpr bool operator==(Pair const& a, Pair const& b)
        {
            return a.first == b.first && a.second == b.second;
        }

        WE_NODISCARD friend constexpr bool operator<(Pair const& a, Pair const& b)
        {
            return (a.first < b.first) || (!(b.first < a.first) && a.second < b.second);
        }
    };

    template <typename T1, typename T2>
    WE_NODISCARD constexpr Pair<Decay<T1>, Decay<T2>> makePair(T1&& a, T2&& b)
    {
        return Pair<Decay<T1>, Decay<T2>>(worse::core::forward<T1>(a), worse::core::forward<T2>(b));
    }

    // --- CompressedPair -------------------------------------------------------
    //
    // Stores two members with `[[no_unique_address]]` so an empty (stateless) member
    // — a comparator or hash functor — costs zero bytes. This is how hash/flat
    // containers carry their policy functor "for free": e.g. a hash container holds
    // `CompressedPair<usize, Hash>` and pays only `sizeof(usize)` when `Hash` is empty.
    // Access is via `first()`/`second()` methods (members can't be public when EBO'd).

    template <typename T1, typename T2>
    class CompressedPair
    {
    public:
        using FirstType  = T1;
        using SecondType = T2;

        constexpr CompressedPair() = default;

        constexpr CompressedPair(T1 const& a, T2 const& b) : mFirst(a), mSecond(b) {}

        template <typename U1, typename U2>
        constexpr CompressedPair(U1&& a, U2&& b)
            : mFirst(worse::core::forward<U1>(a)), mSecond(worse::core::forward<U2>(b))
        {
        }

        WE_NODISCARD constexpr T1& first() noexcept { return mFirst; }
        WE_NODISCARD constexpr T1 const& first() const noexcept { return mFirst; }
        WE_NODISCARD constexpr T2& second() noexcept { return mSecond; }
        WE_NODISCARD constexpr T2 const& second() const noexcept { return mSecond; }

    private:
        WE_NO_UNIQUE_ADDRESS T1 mFirst{};
        WE_NO_UNIQUE_ADDRESS T2 mSecond{};
    };

    // --- default comparison functors ------------------------------------------
    //
    // constexpr, stateless (empty -> EBO via CompressedPair). Each has a transparent
    // `void` specialization (marked `IsTransparent`) that compares heterogeneous
    // operands without forcing a conversion — enabling later heterogeneous lookup
    // (e.g. find a `char const*` key in a `Pair<String, V>` flat map without building
    // a temporary String).

    template <typename T = void>
    struct Less
    {
        WE_NODISCARD constexpr bool operator()(T const& a, T const& b) const { return a < b; }
    };

    template <>
    struct Less<void>
    {
        using IsTransparent = void;
        template <typename A, typename B>
        WE_NODISCARD constexpr bool operator()(A&& a, B&& b) const
        {
            return worse::core::forward<A>(a) < worse::core::forward<B>(b);
        }
    };

    template <typename T = void>
    struct Greater
    {
        WE_NODISCARD constexpr bool operator()(T const& a, T const& b) const { return b < a; }
    };

    template <>
    struct Greater<void>
    {
        using IsTransparent = void;
        template <typename A, typename B>
        WE_NODISCARD constexpr bool operator()(A&& a, B&& b) const
        {
            return worse::core::forward<B>(b) < worse::core::forward<A>(a);
        }
    };

    template <typename T = void>
    struct EqualTo
    {
        WE_NODISCARD constexpr bool operator()(T const& a, T const& b) const { return a == b; }
    };

    template <>
    struct EqualTo<void>
    {
        using IsTransparent = void;
        template <typename A, typename B>
        WE_NODISCARD constexpr bool operator()(A&& a, B&& b) const
        {
            return worse::core::forward<A>(a) == worse::core::forward<B>(b);
        }
    };
} // namespace worse::core
