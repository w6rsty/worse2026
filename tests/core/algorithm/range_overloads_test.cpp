#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.utility;
import worse.core.algorithm.algorithm; // umbrella -> brings the container-range overloads
import worse.core.container.array;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    Array<int> make(std::initializer_list<int> v)
    {
        Array<int> a;
        for (int x : v)
        {
            a.pushBack(x);
        }
        return a;
    }
} // namespace

TEST(RangeOverloadsTest, SortStableReverse)
{
    Array<int> a = make({5, 3, 1, 4, 2});
    sort(a);
    EXPECT_TRUE(isSorted(a.begin(), a.end()));
    EXPECT_EQ(a.front(), 1);
    EXPECT_EQ(a.back(), 5);

    sort(a, Greater<>{});
    EXPECT_EQ(a.front(), 5);

    reverse(a);
    EXPECT_EQ(a.front(), 1);

    Array<int> b = make({3, 1, 2, 1, 3});
    stableSort(b);
    EXPECT_TRUE(isSorted(b.begin(), b.end()));
}

TEST(RangeOverloadsTest, IteratorPairOverloadStillResolves)
{
    // The range overload must not shadow / collide with the iterator-pair one.
    Array<int> a = make({3, 2, 1});
    sort(a.begin(), a.end()); // iterator-pair
    EXPECT_TRUE(isSorted(a.begin(), a.end()));
}

TEST(RangeOverloadsTest, FindCountQueries)
{
    Array<int> a = make({2, 4, 6, 8, 4});
    EXPECT_EQ(*find(a, 6), 6);
    EXPECT_EQ(find(a, 5), a.end());
    EXPECT_EQ(count(a, 4), 2);
    auto it = findIf(a, [](int x)
                     { return x > 5; });
    EXPECT_EQ(*it, 6);
}

TEST(RangeOverloadsTest, ForEachAndPredicates)
{
    Array<int> a = make({2, 4, 6});
    int sum      = 0;
    forEach(a, [&](int x)
            { sum += x; });
    EXPECT_EQ(sum, 12);

    EXPECT_TRUE(allOf(a, [](int x)
                      { return x % 2 == 0; }));
    EXPECT_TRUE(anyOf(a, [](int x)
                      { return x == 4; }));
    EXPECT_TRUE(noneOf(a, [](int x)
                       { return x > 100; }));
}
