#include <gtest/gtest.h>

#include "worse/core/macro.hpp"

#include <string>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;

using namespace worse;
using namespace worse::core;

namespace
{
    // Counts how it was constructed so we can prove move vs copy selection.
    struct Counter
    {
        static int moves;
        static int copies;
        int value = 0;

        Counter() = default;
        explicit Counter(int v) : value(v) {}
        Counter(Counter const& o) : value(o.value) { ++copies; }
        Counter(Counter&& o) noexcept : value(o.value) { ++moves; }
        Counter& operator=(Counter const&) = default;
        Counter& operator=(Counter&&)      = default;

        static void reset() { moves = copies = 0; }
    };
    int Counter::moves  = 0;
    int Counter::copies = 0;

    // Move may throw + is copyable -> moveIfNoexcept must fall back to a copy (const&).
    struct ThrowingMove
    {
        ThrowingMove() = default;
        ThrowingMove(ThrowingMove const&) {}
        ThrowingMove(ThrowingMove&&) noexcept(false) {}
    };

    // Move is noexcept -> moveIfNoexcept hands out an rvalue (T&&).
    struct NoexceptMove
    {
        NoexceptMove() = default;
        NoexceptMove(NoexceptMove const&) {}
        NoexceptMove(NoexceptMove&&) noexcept {}
    };

    struct Empty
    {
    };
} // namespace

TEST(UtilityTest, MoveSelectsMoveCtor)
{
    Counter::reset();
    Counter a{7};
    Counter b{move(a)};
    EXPECT_EQ(b.value, 7);
    EXPECT_EQ(Counter::moves, 1);
    EXPECT_EQ(Counter::copies, 0);

    Counter c{a}; // lvalue -> copy
    EXPECT_EQ(Counter::copies, 1);
    (void)c;
}

TEST(UtilityTest, ForwardPreservesValueCategory)
{
    // forward<T> with T an lvalue ref yields an lvalue; with T a value/rvalue ref
    // yields an rvalue. Verify via the ctor that the forwarded object selects.
    Counter::reset();
    auto make = [](auto&& x) -> Counter
    {
        return Counter{forward<decltype(x)>(x)};
    };
    Counter src{3};
    Counter fromLvalue = make(src); // -> copy
    EXPECT_EQ(Counter::copies, 1);
    EXPECT_EQ(Counter::moves, 0);
    Counter fromRvalue = make(Counter{4}); // -> move
    EXPECT_EQ(Counter::moves, 1);
    (void)fromLvalue;
    (void)fromRvalue;
}

TEST(UtilityTest, MoveIfNoexceptReturnType)
{
    ThrowingMove tm;
    NoexceptMove nm;
    // Throwing move + copyable -> const& (forces copy).
    static_assert(IsSame<decltype(moveIfNoexcept(tm)), ThrowingMove const&>);
    // Nothrow move -> rvalue reference.
    static_assert(IsSame<decltype(moveIfNoexcept(nm)), NoexceptMove&&>);
    (void)tm;
    (void)nm;
    SUCCEED();
}

TEST(UtilityTest, SwapScalarAndArray)
{
    int x = 1, y = 2;
    swap(x, y);
    EXPECT_EQ(x, 2);
    EXPECT_EQ(y, 1);

    int a[3] = {1, 2, 3};
    int b[3] = {4, 5, 6};
    swap(a, b);
    EXPECT_EQ(a[0], 4);
    EXPECT_EQ(a[2], 6);
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[2], 3);
}

TEST(UtilityTest, Exchange)
{
    int x         = 5;
    int const old = exchange(x, 10);
    EXPECT_EQ(old, 5);
    EXPECT_EQ(x, 10);
}

TEST(UtilityTest, PairConstructionAndBindings)
{
    Pair<int, std::string> p{1, "hi"};
    EXPECT_EQ(p.first, 1);
    EXPECT_EQ(p.second, "hi");

    // structured bindings work because first/second are public.
    auto [a, b] = p;
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, "hi");

    // converting construction from a different Pair instantiation.
    Pair<long, std::string> q = p;
    EXPECT_EQ(q.first, 1L);

    auto made = makePair(2, std::string{"x"});
    static_assert(IsSame<decltype(made), Pair<int, std::string>>);
    EXPECT_EQ(made.first, 2);
}

TEST(UtilityTest, PairComparison)
{
    Pair<int, int> a{1, 2};
    Pair<int, int> b{1, 3};
    Pair<int, int> c{1, 2};
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a != b); // synthesized from == in C++20
    EXPECT_TRUE(a < b);  // lexicographic
    EXPECT_FALSE(b < a);
    EXPECT_FALSE(a < c);
}

TEST(UtilityTest, PairSwap)
{
    Pair<int, int> a{1, 2};
    Pair<int, int> b{3, 4};
    a.swap(b);
    EXPECT_EQ(a.first, 3);
    EXPECT_EQ(a.second, 4);
    EXPECT_EQ(b.first, 1);
    EXPECT_EQ(b.second, 2);
}

TEST(UtilityTest, CompressedPairEbo)
{
    // Empty second member overlaps -> whole pair is just the first member's size.
    static_assert(sizeof(CompressedPair<usize, Empty>) == sizeof(usize));
    static_assert(sizeof(CompressedPair<usize, Less<int>>) == sizeof(usize));
    static_assert(sizeof(CompressedPair<usize, Less<>>) == sizeof(usize));
    // Two non-empty members cannot overlap.
    static_assert(sizeof(CompressedPair<usize, usize>) == 2 * sizeof(usize));

    CompressedPair<usize, Empty> cp{42u, Empty{}};
    EXPECT_EQ(cp.first(), 42u);
    cp.first() = 7u;
    EXPECT_EQ(cp.first(), 7u);
}

TEST(UtilityTest, Functors)
{
    static_assert(Less<int>{}(1, 2));
    static_assert(!Less<int>{}(2, 2));
    static_assert(Greater<int>{}(2, 1));
    static_assert(EqualTo<int>{}(3, 3));
    static_assert(!EqualTo<int>{}(3, 4));

    // transparent (void) specializations compare heterogeneous operands.
    Less<> lt;
    Greater<> gt;
    EqualTo<> eq;
    EXPECT_TRUE(lt(1, 2L));
    EXPECT_TRUE(gt(2L, 1));
    EXPECT_TRUE(eq(3, 3L));
    static_assert(IsSame<Less<>::IsTransparent, void>);
}

TEST(UtilityTest, FunctorsAreStatelessAndUsableAsPolicy)
{
    // The whole point of the void functors + CompressedPair: zero-cost policy.
    static_assert(sizeof(CompressedPair<int, Greater<>>) == sizeof(int));
    static_assert(sizeof(CompressedPair<int, EqualTo<>>) == sizeof(int));
    SUCCEED();
}
