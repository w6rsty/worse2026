#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.container.static_array;

using namespace worse;
using namespace worse::core::container;

TEST(StaticArrayTest, LayoutHasNoOverhead)
{
    static_assert(sizeof(StaticArray<int, 4>) == sizeof(int[4]));
    static_assert(sizeof(StaticArray<double, 8>) == sizeof(double[8]));
    SUCCEED();
}

TEST(StaticArrayTest, AggregateInitAndIndexing)
{
    StaticArray<int, 3> a{1, 2, 3};
    EXPECT_EQ(a.size(), 3u);
    EXPECT_FALSE(a.empty());
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a.front(), 1);
    EXPECT_EQ(a.back(), 3);
    a[1] = 20;
    EXPECT_EQ(a.at(1), 20);
}

TEST(StaticArrayTest, FillAndData)
{
    StaticArray<int, 5> a{};
    a.fill(7);
    for (int x : a)
    {
        EXPECT_EQ(x, 7);
    }
    EXPECT_EQ(a.data()[0], 7);
}

TEST(StaticArrayTest, Iterators)
{
    StaticArray<int, 4> a{1, 2, 3, 4};
    int sum = 0;
    for (auto it = a.begin(); it != a.end(); ++it)
    {
        sum += *it;
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

TEST(StaticArrayTest, SwapAndEquality)
{
    StaticArray<int, 3> a{1, 2, 3};
    StaticArray<int, 3> b{4, 5, 6};
    StaticArray<int, 3> c{1, 2, 3};
    EXPECT_TRUE(a == c);
    EXPECT_FALSE(a == b);

    swap(a, b);
    EXPECT_EQ(a[0], 4);
    EXPECT_EQ(b[0], 1);
    EXPECT_TRUE(b == c);
}

TEST(StaticArrayTest, Constexpr)
{
    constexpr StaticArray<int, 4> a{10, 20, 30, 40};
    static_assert(a.size() == 4);
    static_assert(a[0] == 10);
    static_assert(a.back() == 40);

    constexpr auto sum = []
    {
        StaticArray<int, 4> v{1, 2, 3, 4};
        int s = 0;
        for (int x : v)
        {
            s += x;
        }
        return s;
    };
    static_assert(sum() == 10);

    constexpr auto filled = []
    {
        StaticArray<int, 3> v{};
        v.fill(5);
        return v[2];
    };
    static_assert(filled() == 5);
    SUCCEED();
}
