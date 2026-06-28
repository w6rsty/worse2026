#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.utility;
import worse.core.algorithm.binary_search;

using namespace worse;
using namespace worse::core;

TEST(BinarySearchTest, LowerBound)
{
    int a[7] = {1, 3, 3, 3, 5, 7, 9};
    EXPECT_EQ(lowerBound(a, a + 7, 3) - a, 1);  // first 3
    EXPECT_EQ(lowerBound(a, a + 7, 0) - a, 0);  // before all
    EXPECT_EQ(lowerBound(a, a + 7, 4) - a, 4);  // between -> at 5
    EXPECT_EQ(lowerBound(a, a + 7, 9) - a, 6);  // last
    EXPECT_EQ(lowerBound(a, a + 7, 10) - a, 7); // past end -> last
}

TEST(BinarySearchTest, UpperBound)
{
    int a[7] = {1, 3, 3, 3, 5, 7, 9};
    EXPECT_EQ(upperBound(a, a + 7, 3) - a, 4); // one past the last 3
    EXPECT_EQ(upperBound(a, a + 7, 0) - a, 0);
    EXPECT_EQ(upperBound(a, a + 7, 9) - a, 7);
    EXPECT_EQ(upperBound(a, a + 7, 5) - a, 5);
}

TEST(BinarySearchTest, BinarySearchPresentAndAbsent)
{
    int a[6] = {2, 4, 6, 8, 10, 12};
    EXPECT_TRUE(binarySearch(a, a + 6, 2));
    EXPECT_TRUE(binarySearch(a, a + 6, 12));
    EXPECT_TRUE(binarySearch(a, a + 6, 8));
    EXPECT_FALSE(binarySearch(a, a + 6, 1));
    EXPECT_FALSE(binarySearch(a, a + 6, 7));
    EXPECT_FALSE(binarySearch(a, a + 6, 13));
}

TEST(BinarySearchTest, EqualRangeDuplicates)
{
    int a[8] = {1, 2, 2, 2, 2, 5, 6, 7};
    auto r   = equalRange(a, a + 8, 2);
    EXPECT_EQ(r.first - a, 1);
    EXPECT_EQ(r.second - a, 5);
    EXPECT_EQ(r.second - r.first, 4); // four 2s

    auto empty = equalRange(a, a + 8, 3); // absent -> empty range at insertion point
    EXPECT_EQ(empty.first, empty.second);
    EXPECT_EQ(empty.first - a, 5);
}

TEST(BinarySearchTest, DescendingWithGreater)
{
    // Range sorted descending must be searched with a matching comparator.
    int a[6] = {9, 7, 5, 3, 1, 0};
    EXPECT_TRUE(binarySearch(a, a + 6, 5, Greater<>{}));
    EXPECT_FALSE(binarySearch(a, a + 6, 4, Greater<>{}));
    EXPECT_EQ(lowerBound(a, a + 6, 7, Greater<>{}) - a, 1);
    EXPECT_EQ(upperBound(a, a + 6, 7, Greater<>{}) - a, 2);
}

TEST(BinarySearchTest, EmptyRange)
{
    int a[1] = {0};
    EXPECT_EQ(lowerBound(a, a, 5), a);
    EXPECT_EQ(upperBound(a, a, 5), a);
    EXPECT_FALSE(binarySearch(a, a, 5));
}

TEST(BinarySearchTest, Constexpr)
{
    constexpr auto found = []
    {
        int a[5] = {1, 2, 3, 4, 5};
        return binarySearch(a, a + 5, 4);
    };
    static_assert(found());

    constexpr auto lb = []
    {
        int a[5] = {1, 2, 2, 2, 5};
        return static_cast<int>(lowerBound(a, a + 5, 2) - a);
    };
    static_assert(lb() == 1);
    SUCCEED();
}
