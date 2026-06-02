#include <gtest/gtest.h>

#include "worse/core/macro.hpp"

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.container.fixed_array;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    template <int Tag>
    struct BasicTracked
    {
        static int alive;
        int v = 0;
        BasicTracked() { ++alive; }
        explicit BasicTracked(int x) : v(x) { ++alive; }
        BasicTracked(BasicTracked const& o) : v(o.v) { ++alive; }
        BasicTracked(BasicTracked&& o) noexcept : v(o.v) { ++alive; }
        BasicTracked& operator=(BasicTracked const&)     = default;
        BasicTracked& operator=(BasicTracked&&) noexcept = default;
        ~BasicTracked() { --alive; }
        static void reset() { alive = 0; }
    };
    template <int Tag>
    int BasicTracked<Tag>::alive = 0;

    using Plain = BasicTracked<0>; // not trivially relocatable

    struct NonReloc
    {
        int* p     = nullptr;
        NonReloc() = default;
        NonReloc(NonReloc&&) noexcept {}
        ~NonReloc() {}
    };
} // namespace

TEST(FixedArrayTest, LayoutAndRelocationTrait)
{
    // Inline buffer (N*sizeof(T)) plus the usize size field.
    static_assert(sizeof(FixedArray<int, 4>) == sizeof(int) * 4 + sizeof(usize));
    // Relocatable iff the element type is (size is an index, not a self-pointer).
    static_assert(IsTriviallyRelocatable<FixedArray<int, 4>>);
    static_assert(!IsTriviallyRelocatable<FixedArray<NonReloc, 4>>);
    SUCCEED();
}

TEST(FixedArrayTest, PushUntilFull)
{
    FixedArray<int, 4> a;
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(a.capacity(), 4u);
    a.pushBack(1);
    a.pushBack(2);
    EXPECT_EQ(a.emplaceBack(3), 3);
    a.pushBack(4);
    EXPECT_TRUE(a.full());
    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a.front(), 1);
    EXPECT_EQ(a.back(), 4);
}

TEST(FixedArrayTest, OverflowHardCapAborts)
{
    // Alias first: the comma in FixedArray<int, 2> would otherwise split the macro args.
    using FA2 = FixedArray<int, 2>;
    EXPECT_DEATH(
        {
            FA2 a;
            a.pushBack(1);
            a.pushBack(2);
            a.pushBack(3); // past the hard cap -> WE_VERIFY abort (always-on, incl. release; R46)
        },
        "");
}

TEST(FixedArrayTest, TryPushAndRemaining)
{
    FixedArray<int, 3> a;
    EXPECT_EQ(a.remaining(), 3u);
    EXPECT_TRUE(a.tryPushBack(1));
    EXPECT_EQ(a.remaining(), 2u);
    int* p = a.tryEmplaceBack(2);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 2);
    EXPECT_TRUE(a.tryPushBack(3));
    EXPECT_TRUE(a.full());
    EXPECT_EQ(a.remaining(), 0u);
    // Non-aborting at capacity (R46): returns false / nullptr, container unchanged.
    EXPECT_FALSE(a.tryPushBack(4));
    EXPECT_EQ(a.tryEmplaceBack(5), nullptr);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a.back(), 3);
}

TEST(FixedArrayTest, PopAndIndex)
{
    FixedArray<int, 5> a{10, 20, 30};
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a[1], 20);
    a.popBack();
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a.back(), 20);
}

TEST(FixedArrayTest, InsertEraseEraseUnsorted)
{
    FixedArray<int, 8> a{1, 2, 4, 5};
    a.insert(a.begin() + 2, 3); // {1,2,3,4,5}
    EXPECT_EQ(a.size(), 5u);
    EXPECT_EQ(a[2], 3);

    a.erase(a.begin() + 2); // {1,2,4,5}
    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a[2], 4);

    a.eraseUnsorted(a.begin()); // last (5) replaces index 0 -> {5,2,4}
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a[0], 5);
}

TEST(FixedArrayTest, Resize)
{
    FixedArray<int, 6> a{1, 2, 3};
    a.resize(5); // value-initialized tail
    EXPECT_EQ(a.size(), 5u);
    EXPECT_EQ(a[3], 0);
    a.resize(6, 9);
    EXPECT_EQ(a[5], 9);
    a.resize(1);
    EXPECT_EQ(a.size(), 1u);
    EXPECT_EQ(a.back(), 1);
}

TEST(FixedArrayTest, CopyIsIndependent)
{
    FixedArray<int, 4> a{1, 2, 3};
    FixedArray<int, 4> b = a;
    b[0]                 = 99;
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(b[0], 99);
    EXPECT_EQ(b.size(), 3u);
}

TEST(FixedArrayTest, MoveLeavesSourceEmpty)
{
    FixedArray<int, 4> a{1, 2, 3};
    FixedArray<int, 4> b = static_cast<FixedArray<int, 4>&&>(a);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b[2], 3);
    EXPECT_TRUE(a.empty()); // inline storage: elements relocated, source emptied
}

TEST(FixedArrayTest, IteratorsForwardAndReverse)
{
    FixedArray<int, 4> a{1, 2, 3, 4};
    int sum = 0;
    for (int x : a)
    {
        sum += x;
    }
    EXPECT_EQ(sum, 10);

    int expected[4] = {4, 3, 2, 1};
    int i           = 0;
    for (auto it = a.rbegin(); it != a.rend(); ++it, ++i)
    {
        EXPECT_EQ(*it, expected[i]);
    }
    EXPECT_EQ(i, 4);
}

TEST(FixedArrayTest, SwapDifferentSizes)
{
    FixedArray<int, 5> a{1, 2};
    FixedArray<int, 5> b{7, 8, 9, 10};
    swap(a, b);
    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a[3], 10);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
}

TEST(FixedArrayTest, NoLeaks)
{
    Plain::reset();
    {
        FixedArray<Plain, 16> a;
        for (int i = 0; i < 10; ++i)
        {
            a.emplaceBack(i);
        }
        a.erase(a.begin() + 2);
        a.eraseUnsorted(a.begin());
        a.insert(a.begin() + 1, Plain{42});
        a.resize(5);
        FixedArray<Plain, 16> b = a;                                       // copy
        FixedArray<Plain, 16> c = static_cast<FixedArray<Plain, 16>&&>(b); // move
        a.clear();
        EXPECT_GT(Plain::alive, 0);
    }
    EXPECT_EQ(Plain::alive, 0); // balanced ctors/dtors
}
