#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.utility;
import worse.core.algorithm.sort;

using namespace worse;
using namespace worse::core;

namespace
{
    // Deterministic pseudo-random fill (LCG) so failures reproduce.
    void lcgFill(int* first, int* last, u32 seed)
    {
        u32 state = seed;
        for (int* it = first; it != last; ++it)
        {
            state = state * 1664525u + 1013904223u;
            *it   = static_cast<int>(state % 100000u);
        }
    }

    bool checkSorted(int const* first, int const* last)
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

TEST(SortTest, InsertionSortSmall)
{
    int a[6] = {5, 2, 4, 1, 3, 0};
    insertionSort(a, a + 6);
    EXPECT_TRUE(checkSorted(a, a + 6));
    EXPECT_EQ(a[0], 0);
    EXPECT_EQ(a[5], 5);
}

TEST(SortTest, SortRandomLarge)
{
    // > kInsertionThreshold and deep enough to exercise the quicksort recursion.
    constexpr int N = 2000;
    static int a[N];
    lcgFill(a, a + N, 12345u);
    sort(a, a + N);
    EXPECT_TRUE(checkSorted(a, a + N));
    EXPECT_TRUE(isSorted(a, a + N));
}

TEST(SortTest, SortAdversarialInputs)
{
    constexpr int N = 1000;
    static int a[N];

    // already sorted
    for (int i = 0; i < N; ++i)
    {
        a[i] = i;
    }
    sort(a, a + N);
    EXPECT_TRUE(checkSorted(a, a + N));

    // reverse sorted (a classic naive-quicksort worst case -> heapsort fallback path)
    for (int i = 0; i < N; ++i)
    {
        a[i] = N - i;
    }
    sort(a, a + N);
    EXPECT_TRUE(checkSorted(a, a + N));

    // all equal
    for (int i = 0; i < N; ++i)
    {
        a[i] = 7;
    }
    sort(a, a + N);
    EXPECT_TRUE(checkSorted(a, a + N));
    EXPECT_EQ(a[0], 7);
    EXPECT_EQ(a[N - 1], 7);
}

TEST(SortTest, SortDescendingComparator)
{
    int a[10];
    lcgFill(a, a + 10, 99u);
    sort(a, a + 10, Greater<>{});
    for (int i = 1; i < 10; ++i)
    {
        EXPECT_GE(a[i - 1], a[i]);
    }
}

TEST(SortTest, PartialSortTopK)
{
    constexpr int N = 200;
    static int a[N];
    lcgFill(a, a + N, 555u);

    // Keep a copy to know the true k smallest.
    static int ref[N];
    for (int i = 0; i < N; ++i)
    {
        ref[i] = a[i];
    }
    sort(ref, ref + N);

    int const k = 10;
    partialSort(a, a + k, a + N);
    EXPECT_TRUE(checkSorted(a, a + k));
    for (int i = 0; i < k; ++i)
    {
        EXPECT_EQ(a[i], ref[i]); // the k smallest, in order
    }
}

TEST(SortTest, NthElementMedian)
{
    constexpr int N = 301;
    static int a[N];
    lcgFill(a, a + N, 7u);

    static int ref[N];
    for (int i = 0; i < N; ++i)
    {
        ref[i] = a[i];
    }
    sort(ref, ref + N);

    int const mid = N / 2;
    nthElement(a, a + mid, a + N);
    EXPECT_EQ(a[mid], ref[mid]); // the true median sits at mid
    // partition property: everything left <= a[mid] <= everything right
    for (int i = 0; i < mid; ++i)
    {
        EXPECT_LE(a[i], a[mid]);
    }
    for (int i = mid + 1; i < N; ++i)
    {
        EXPECT_GE(a[i], a[mid]);
    }
}

TEST(SortTest, IsSortedAndUntil)
{
    int good[5] = {1, 2, 3, 4, 5};
    EXPECT_TRUE(isSorted(good, good + 5));
    EXPECT_EQ(isSortedUntil(good, good + 5), good + 5);

    int bad[5] = {1, 2, 9, 4, 5};
    EXPECT_FALSE(isSorted(bad, bad + 5));
    EXPECT_EQ(isSortedUntil(bad, bad + 5), bad + 3); // breaks at 4 (index 3)
}

TEST(SortTest, Constexpr)
{
    constexpr auto sortedFirst = []
    {
        int a[7] = {4, 1, 7, 3, 2, 6, 5};
        sort(a, a + 7);
        return a[0];
    };
    static_assert(sortedFirst() == 1);

    constexpr auto isS = []
    {
        int a[4] = {1, 2, 3, 4};
        return isSorted(a, a + 4);
    };
    static_assert(isS());
    SUCCEED();
}
