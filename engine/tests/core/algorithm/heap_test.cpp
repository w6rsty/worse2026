#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.utility;
import worse.core.algorithm.heap;

using namespace worse;
using namespace worse::core;

namespace
{
    bool isSortedAscending(int const* first, int const* last)
    {
        for (int const* it = first + 1; it < last; ++it)
        {
            if (it[0] < it[-1])
            {
                return false;
            }
        }
        return true;
    }
} // namespace

TEST(HeapTest, MakeHeapAndMaxOnTop)
{
    int a[8] = {3, 1, 4, 1, 5, 9, 2, 6};
    makeHeap(a, a + 8);
    EXPECT_TRUE(isHeap(a, a + 8));
    // The maximum is at the root of a max-heap.
    EXPECT_EQ(a[0], 9);
}

TEST(HeapTest, PushHeap)
{
    int a[6] = {5, 4, 3, 2, 1, 0};
    makeHeap(a, a + 5); // heap over first 5
    EXPECT_TRUE(isHeap(a, a + 5));
    a[5] = 100; // append a new largest element
    pushHeap(a, a + 6);
    EXPECT_TRUE(isHeap(a, a + 6));
    EXPECT_EQ(a[0], 100);
}

TEST(HeapTest, PopHeapMovesMaxToBack)
{
    int a[6] = {1, 8, 3, 8, 5, 2};
    makeHeap(a, a + 6);
    int const top = a[0];
    popHeap(a, a + 6);
    EXPECT_EQ(a[5], top);          // max relocated to the back
    EXPECT_TRUE(isHeap(a, a + 5)); // remainder still a heap
}

TEST(HeapTest, SortHeapAscending)
{
    int a[10] = {9, 3, 7, 1, 8, 2, 6, 0, 5, 4};
    makeHeap(a, a + 10);
    sortHeap(a, a + 10);
    EXPECT_TRUE(isSortedAscending(a, a + 10));
    EXPECT_EQ(a[0], 0);
    EXPECT_EQ(a[9], 9);
}

TEST(HeapTest, MinHeapWithGreater)
{
    int a[7] = {3, 1, 4, 1, 5, 9, 2};
    makeHeap(a, a + 7, Greater<>{});
    EXPECT_TRUE(isHeap(a, a + 7, Greater<>{}));
    EXPECT_EQ(a[0], 1); // smallest on top for a min-heap
    // sortHeap with Greater yields descending order.
    sortHeap(a, a + 7, Greater<>{});
    for (int i = 1; i < 7; ++i)
    {
        EXPECT_GE(a[i - 1], a[i]);
    }
}

TEST(HeapTest, IsHeapUntil)
{
    int good[4] = {9, 5, 4, 1};
    EXPECT_EQ(isHeapUntil(good, good + 4), good + 4);

    int bad[4] = {9, 5, 4, 8}; // 8 > parent 5 at index 1 -> breaks at index 3
    EXPECT_EQ(isHeapUntil(bad, bad + 4), bad + 3);
    EXPECT_FALSE(isHeap(bad, bad + 4));
}

TEST(HeapTest, EdgeCasesEmptyAndSingle)
{
    int a[1] = {42};
    makeHeap(a, a);     // empty
    makeHeap(a, a + 1); // single
    EXPECT_TRUE(isHeap(a, a + 1));
    popHeap(a, a + 1);  // no-op
    sortHeap(a, a + 1); // no-op
    EXPECT_EQ(a[0], 42);
}

TEST(HeapTest, Constexpr)
{
    constexpr auto topAfterMakeHeap = []
    {
        int a[5] = {2, 7, 1, 8, 3};
        makeHeap(a, a + 5);
        return a[0];
    };
    static_assert(topAfterMakeHeap() == 8);

    constexpr auto sortedFirst = []
    {
        int a[5] = {2, 7, 1, 8, 3};
        makeHeap(a, a + 5);
        sortHeap(a, a + 5);
        return a[0];
    };
    static_assert(sortedFirst() == 1);
    SUCCEED();
}
