#include <gtest/gtest.h>

#include "worse/core/macro.hpp"

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.container.array;
import worse.core.container.allocator;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    // Lifetime/relocation instrumentation, parameterized so each variant has its own
    // counters. Non-trivially-copyable (user copy/move/dtor); variant 1 is opted into
    // trivial relocation below to exercise the memcpy grow path.
    template <int Tag>
    struct BasicTracked
    {
        static int alive;
        static int moves;
        int v = 0;

        BasicTracked() { ++alive; }
        explicit BasicTracked(int x) : v(x) { ++alive; }
        BasicTracked(BasicTracked const& o) : v(o.v) { ++alive; }
        BasicTracked(BasicTracked&& o) noexcept : v(o.v)
        {
            ++alive;
            ++moves;
        }
        BasicTracked& operator=(BasicTracked const&)     = default;
        BasicTracked& operator=(BasicTracked&&) noexcept = default;
        ~BasicTracked() { --alive; }

        static void reset() { alive = moves = 0; }
    };
    template <int Tag>
    int BasicTracked<Tag>::alive = 0;
    template <int Tag>
    int BasicTracked<Tag>::moves = 0;

    using Plain = BasicTracked<0>;
    using Reloc = BasicTracked<1>;
} // namespace

WE_DECLARE_TRIVIALLY_RELOCATABLE(Reloc);

// --- the original layout/EBO contract (unchanged) ------------------------------
TEST(ArrayTest, Template)
{
    struct Foo
    {
        u32 a;
        f32 b;
    };
    struct EmptyAlloc
    {
    };
    struct Alloc
    {
        u32 counter;
        u32 pointer;
    };
    struct Type
    {
        Array<Foo, EmptyAlloc> _1;
        Array<Foo, Alloc> _3;
        Array<Foo> _4;
    };

    static_assert(sizeof(Type::_1) == (sizeof(Foo*) * 3));
    static_assert(sizeof(Type::_3) == (sizeof(Foo*) * 3 + sizeof(Alloc)));
}

TEST(ArrayTest, PushBackGrowsAndKeepsValues)
{
    Array<int> a;
    EXPECT_TRUE(a.empty());
    for (int i = 0; i < 100; ++i)
    {
        a.pushBack(i);
    }
    EXPECT_EQ(a.size(), 100u);
    EXPECT_GE(a.capacity(), a.size());
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(a[i], i);
    }
    EXPECT_EQ(a.front(), 0);
    EXPECT_EQ(a.back(), 99);
}

TEST(ArrayTest, EmplaceBackReturnsReference)
{
    Array<int> a;
    int& r = a.emplaceBack(42);
    EXPECT_EQ(r, 42);
    r = 7;
    EXPECT_EQ(a[0], 7);
}

TEST(ArrayTest, PopBack)
{
    Array<int> a{1, 2, 3};
    a.popBack();
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a.back(), 2);
}

TEST(ArrayTest, ReserveAndShrinkToFit)
{
    Array<int> a;
    a.reserve(50);
    EXPECT_GE(a.capacity(), 50u);
    EXPECT_EQ(a.size(), 0u);
    a.pushBack(1);
    a.pushBack(2);
    a.shrinkToFit();
    EXPECT_EQ(a.capacity(), 2u);
    EXPECT_EQ(a[0], 1);
}

TEST(ArrayTest, InsertAndErase)
{
    Array<int> a{1, 2, 4, 5};
    auto it = a.insert(a.begin() + 2, 3); // {1,2,3,4,5}
    EXPECT_EQ(*it, 3);
    EXPECT_EQ(a.size(), 5u);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a[4], 5);

    a.erase(a.begin() + 2); // {1,2,4,5}
    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a[2], 4);

    a.erase(a.begin(), a.begin() + 2); // {4,5}
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a[0], 4);
    EXPECT_EQ(a[1], 5);
}

TEST(ArrayTest, InsertCausingRealloc)
{
    Array<int> a;
    a.reserve(2);
    a.pushBack(1);
    a.pushBack(3);
    a.insert(a.begin() + 1, 2); // forces a grow; {1,2,3}
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[1], 2);
    EXPECT_EQ(a[2], 3);
}

TEST(ArrayTest, EraseUnsortedIsO1Swap)
{
    Array<int> a{10, 20, 30, 40, 50};
    a.eraseUnsorted(a.begin() + 1); // 20 replaced by last (50)
    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a[1], 50);
    EXPECT_EQ(a[3], 40);
}

TEST(ArrayTest, ResizeGrowAndShrink)
{
    Array<int> a{1, 2, 3};
    a.resize(5); // value-initialized tail
    EXPECT_EQ(a.size(), 5u);
    EXPECT_EQ(a[3], 0);
    EXPECT_EQ(a[4], 0);
    a.resize(7, 9);
    EXPECT_EQ(a[5], 9);
    EXPECT_EQ(a[6], 9);
    a.resize(2);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a.back(), 2);
}

TEST(ArrayTest, CopyIsDeepIndependent)
{
    Array<int> a{1, 2, 3};
    Array<int> b = a;
    b[0]         = 99;
    EXPECT_EQ(a[0], 1); // original unchanged
    EXPECT_EQ(b[0], 99);
    EXPECT_EQ(b.size(), 3u);
}

TEST(ArrayTest, MoveStealsBuffer)
{
    Array<int> a{1, 2, 3};
    int* const data = a.data();
    Array<int> b    = static_cast<Array<int>&&>(a);
    EXPECT_EQ(b.data(), data); // same buffer, no reallocation
    EXPECT_EQ(b.size(), 3u);
    EXPECT_TRUE(a.empty()); // moved-from is empty-but-valid
    EXPECT_EQ(a.data(), nullptr);
}

TEST(ArrayTest, AssignAndClear)
{
    Array<int> a{1, 2, 3};
    a.assign(4, 7);
    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a[0], 7);
    EXPECT_EQ(a[3], 7);
    a.clear();
    EXPECT_TRUE(a.empty());
    EXPECT_GE(a.capacity(), 4u); // clear keeps capacity
}

TEST(ArrayTest, IteratorsForwardAndReverse)
{
    Array<int> a{1, 2, 3, 4};
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

TEST(ArrayTest, Swap)
{
    Array<int> a{1, 2};
    Array<int> b{9, 8, 7};
    swap(a, b);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(a[0], 9);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b[0], 1);
}

TEST(ArrayTest, NoLeaksAcrossOperations)
{
    Plain::reset();
    {
        Array<Plain> a;
        for (int i = 0; i < 50; ++i)
        {
            a.emplaceBack(i);
        }
        a.erase(a.begin() + 10);
        a.eraseUnsorted(a.begin());
        a.insert(a.begin() + 5, Plain{123});
        a.resize(20);
        Array<Plain> b = a; // copy
        Array<Plain> c = static_cast<Array<Plain>&&>(b);
        a.clear();
        EXPECT_GT(Plain::alive, 0); // c still holds elements
    }
    EXPECT_EQ(Plain::alive, 0); // every construction was matched by a destruction
}

TEST(ArrayTest, RelocatableTypeGrowsViaMemcpyNoMoves)
{
    Reloc::reset();
    {
        Array<Reloc> a;
        for (int i = 0; i < 64; ++i)
        {
            a.emplaceBack(i); // many grows
        }
        EXPECT_EQ(a.size(), 64u);
        EXPECT_EQ(Reloc::alive, 64);
        // Grows relocated via memcpy => the move constructor was never invoked.
        EXPECT_EQ(Reloc::moves, 0);
        EXPECT_EQ(a[63].v, 63);
    }
    EXPECT_EQ(Reloc::alive, 0);
}

TEST(ArrayTest, NonRelocatableTypeUsesMoveOnGrow)
{
    Plain::reset();
    {
        Array<Plain> a;
        for (int i = 0; i < 64; ++i)
        {
            a.emplaceBack(i);
        }
        // A non-relocatable type must be moved element-by-element on grow.
        EXPECT_GT(Plain::moves, 0);
    }
    EXPECT_EQ(Plain::alive, 0);
}
