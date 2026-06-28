#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.utility;
import worse.core.algorithm.nonmodifying;

using namespace worse;
using namespace worse::core;

TEST(NonModifyingTest, Find)
{
    int a[5] = {10, 20, 30, 40, 50};
    EXPECT_EQ(find(a, a + 5, 30) - a, 2);
    EXPECT_EQ(find(a, a + 5, 99), a + 5); // not found
}

TEST(NonModifyingTest, FindIfAndFindIfNot)
{
    int a[5]       = {2, 4, 5, 6, 8};
    auto const odd = [](int x)
    {
        return x % 2 != 0;
    };
    EXPECT_EQ(findIf(a, a + 5, odd) - a, 2); // first odd (5)
    EXPECT_EQ(findIfNot(a, a + 5, [](int x)
                        { return x % 2 == 0; }) -
                  a,
              2);
}

TEST(NonModifyingTest, CountAndCountIf)
{
    int a[8] = {1, 2, 2, 3, 2, 4, 2, 5};
    EXPECT_EQ(count(a, a + 8, 2), 4);
    EXPECT_EQ(countIf(a, a + 8, [](int x)
                      { return x > 2; }),
              3);
}

TEST(NonModifyingTest, ForEachAccumulatesAndMutatesFunctor)
{
    int a[4] = {1, 2, 3, 4};
    int sum  = 0;
    forEach(a, a + 4, [&](int x)
            { sum += x; });
    EXPECT_EQ(sum, 10);

    struct Acc
    {
        int total = 0;
        void operator()(int x) { total += x; }
    };
    Acc const result = forEach(a, a + 4, Acc{});
    EXPECT_EQ(result.total, 10); // returned functor carries the state
}

TEST(NonModifyingTest, AllAnyNone)
{
    int a[4]        = {2, 4, 6, 8};
    auto const even = [](int x)
    {
        return x % 2 == 0;
    };
    auto const negative = [](int x)
    {
        return x < 0;
    };
    EXPECT_TRUE(allOf(a, a + 4, even));
    EXPECT_FALSE(allOf(a, a + 4, [](int x)
                       { return x > 4; }));
    EXPECT_TRUE(anyOf(a, a + 4, [](int x)
                      { return x > 4; }));
    EXPECT_FALSE(anyOf(a, a + 4, negative));
    EXPECT_TRUE(noneOf(a, a + 4, negative));
}

TEST(NonModifyingTest, EqualThreeAndFourArg)
{
    int a[4] = {1, 2, 3, 4};
    int b[4] = {1, 2, 3, 4};
    int c[4] = {1, 2, 9, 4};
    int d[5] = {1, 2, 3, 4, 5};

    EXPECT_TRUE(equal(a, a + 4, b));
    EXPECT_FALSE(equal(a, a + 4, c));

    EXPECT_TRUE(equal(a, a + 4, b, b + 4));
    EXPECT_FALSE(equal(a, a + 4, d, d + 5)); // different lengths -> not equal
}

TEST(NonModifyingTest, Mismatch)
{
    int a[5] = {1, 2, 3, 4, 5};
    int b[5] = {1, 2, 9, 4, 5};
    auto m   = mismatch(a, a + 5, b);
    EXPECT_EQ(m.first - a, 2);
    EXPECT_EQ(m.second - b, 2);
    EXPECT_EQ(*m.first, 3);
    EXPECT_EQ(*m.second, 9);

    int same[5] = {1, 2, 3, 4, 5};
    auto m2     = mismatch(a, a + 5, same);
    EXPECT_EQ(m2.first, a + 5); // no mismatch -> ends
}

TEST(NonModifyingTest, Constexpr)
{
    constexpr auto found = []
    {
        int a[4] = {3, 1, 4, 1};
        return find(a, a + 4, 4) - a;
    };
    static_assert(found() == 2);
    SUCCEED();
}
