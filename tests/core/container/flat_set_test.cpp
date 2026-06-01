#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.flat_set;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    struct Tracked
    {
        static int alive;
        int v = 0;
        Tracked() { ++alive; }
        explicit Tracked(int x) : v(x) { ++alive; }
        Tracked(Tracked const& o) : v(o.v) { ++alive; }
        Tracked(Tracked&& o) noexcept : v(o.v) { ++alive; }
        Tracked& operator=(Tracked const&)     = default;
        Tracked& operator=(Tracked&&) noexcept = default;
        ~Tracked() { --alive; }
        friend bool operator<(Tracked const& a, Tracked const& b) { return a.v < b.v; }
        static void reset() { alive = 0; }
    };
    int Tracked::alive = 0;

    template <typename Set>
    bool isStrictlySorted(Set const& s)
    {
        auto it = s.begin();
        if (it == s.end())
        {
            return true;
        }
        auto prev = it;
        for (++it; it != s.end(); ++it, ++prev)
        {
            if (!(*prev < *it))
            {
                return false;
            }
        }
        return true;
    }
} // namespace

TEST(FlatSetTest, IteratorsAreConst)
{
    // Keys are immutable through the set (sorted invariant). Dereference yields const.
    static_assert(IsSame<decltype(*FlatSet<int>{}.begin()), int const&>);
    SUCCEED();
}

TEST(FlatSetTest, InsertUniqueAndSorted)
{
    FlatSet<int> s;
    EXPECT_TRUE(s.empty());

    auto r1 = s.insert(5);
    EXPECT_TRUE(r1.second);
    EXPECT_EQ(*r1.first, 5);

    auto r2 = s.insert(5); // duplicate
    EXPECT_FALSE(r2.second);
    EXPECT_EQ(*r2.first, 5);
    EXPECT_EQ(s.size(), 1u);

    for (int x : {3, 8, 1, 8, 4})
    {
        s.insert(x);
    }
    EXPECT_EQ(s.size(), 5u); // {1,3,4,5,8}
    EXPECT_TRUE(isStrictlySorted(s));

    int expected[] = {1, 3, 4, 5, 8};
    int i          = 0;
    for (int v : s)
    {
        EXPECT_EQ(v, expected[i++]);
    }
}

TEST(FlatSetTest, FindContainsCount)
{
    FlatSet<int> s{2, 4, 6, 8};
    EXPECT_NE(s.find(6), s.end());
    EXPECT_EQ(*s.find(6), 6);
    EXPECT_EQ(s.find(5), s.end());
    EXPECT_TRUE(s.contains(2));
    EXPECT_FALSE(s.contains(3));
    EXPECT_EQ(s.count(8), 1u);
    EXPECT_EQ(s.count(9), 0u);
}

TEST(FlatSetTest, EraseByKeyAndIterator)
{
    FlatSet<int> s{1, 2, 3, 4, 5};
    EXPECT_EQ(s.erase(3), 1u); // {1,2,4,5}
    EXPECT_EQ(s.erase(3), 0u); // already gone
    EXPECT_EQ(s.size(), 4u);

    auto it = s.find(4);
    auto nx = s.erase(it); // {1,2,5}, returns iterator to 5
    EXPECT_EQ(*nx, 5);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_FALSE(s.contains(4));
    EXPECT_TRUE(isStrictlySorted(s));
}

TEST(FlatSetTest, LowerUpperEqualRange)
{
    FlatSet<int> s{10, 20, 30, 40};
    EXPECT_EQ(*s.lowerBound(20), 20);
    EXPECT_EQ(*s.upperBound(20), 30);
    EXPECT_EQ(*s.lowerBound(25), 30); // first not-before 25
    EXPECT_EQ(*s.upperBound(25), 30);

    auto er = s.equalRange(30);
    EXPECT_EQ(er.second - er.first, 1); // exactly one equivalent element
    EXPECT_EQ(*er.first, 30);

    auto miss = s.equalRange(35);
    EXPECT_EQ(miss.first, miss.second); // empty range
}

TEST(FlatSetTest, BulkBuildSortsAndDedups)
{
    int data[] = {9, 3, 3, 7, 1, 9, 5, 1};
    FlatSet<int> s(data, data + 8);
    EXPECT_EQ(s.size(), 5u); // {1,3,5,7,9}
    EXPECT_TRUE(isStrictlySorted(s));
    EXPECT_TRUE(s.contains(1) && s.contains(9));

    // Bulk insert merging into existing contents.
    int more[] = {2, 9, 6};
    s.insert(more, more + 3);
    EXPECT_EQ(s.size(), 7u); // {1,2,3,5,6,7,9}
    EXPECT_TRUE(isStrictlySorted(s));
}

TEST(FlatSetTest, CustomComparatorDescending)
{
    FlatSet<int, Greater<>> s{1, 5, 3, 2, 4};
    EXPECT_EQ(s.size(), 5u);
    int expected[] = {5, 4, 3, 2, 1}; // descending
    int i          = 0;
    for (int v : s)
    {
        EXPECT_EQ(v, expected[i++]);
    }
    EXPECT_NE(s.find(3), s.end());
    EXPECT_EQ(s.erase(5), 1u);
    EXPECT_EQ(*s.begin(), 4);
}

TEST(FlatSetTest, TransparentHeterogeneousLookup)
{
    // Default Less<> is transparent: probe a long-keyed set with an int query without
    // building a long temporary.
    FlatSet<long> s{1L, 2L, 3L};
    int query = 2;
    EXPECT_TRUE(s.contains(query));
    EXPECT_NE(s.find(query), s.end());
}

TEST(FlatSetTest, Swap)
{
    FlatSet<int> a{1, 2, 3};
    FlatSet<int> b{9, 8};
    swap(a, b);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(*a.begin(), 8);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(*b.begin(), 1);
}

TEST(FlatSetTest, NoLeaks)
{
    Tracked::reset();
    {
        FlatSet<Tracked> s;
        for (int i = 0; i < 20; ++i)
        {
            s.emplace((i * 5) % 13); // many duplicates
        }
        EXPECT_LE(s.size(), 13u);
        s.erase(Tracked{0});
        FlatSet<Tracked> copy = s;
        EXPECT_GT(Tracked::alive, 0);
        (void)copy;
    }
    EXPECT_EQ(Tracked::alive, 0);
}
