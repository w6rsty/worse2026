#include <gtest/gtest.h>

#include <vector>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.set;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    template <typename S>
    std::vector<int> collect(S const& s)
    {
        std::vector<int> out;
        for (int v : s)
        {
            out.push_back(v);
        }
        return out;
    }
} // namespace

TEST(SetTest, OrderedInsertDedupAndIterate)
{
    Set<int> s;
    for (int x : {5, 3, 8, 3, 1, 8, 9})
    {
        s.insert(x);
    }
    EXPECT_EQ(s.size(), 5u);
    EXPECT_EQ(collect(s), (std::vector<int>{1, 3, 5, 8, 9})); // sorted + unique
    auto r = s.insert(5);
    EXPECT_FALSE(r.second);
    EXPECT_EQ(*r.first, 5);
}

TEST(SetTest, LookupAndBounds)
{
    Set<int> s{10, 20, 30, 40};
    EXPECT_TRUE(s.contains(30));
    EXPECT_FALSE(s.contains(25));
    EXPECT_EQ(s.count(20), 1u);
    EXPECT_EQ(*s.find(40), 40);
    EXPECT_EQ(s.find(99), s.end());
    EXPECT_EQ(*s.lowerBound(25), 30);
    EXPECT_EQ(*s.upperBound(30), 40);
    auto er = s.equalRange(30);
    EXPECT_EQ(*er.first, 30);
    EXPECT_EQ(*er.second, 40);
}

TEST(SetTest, EraseAndReverse)
{
    Set<int> s{1, 2, 3, 4, 5};
    EXPECT_EQ(s.erase(3), 1u);
    EXPECT_EQ(s.erase(3), 0u);
    EXPECT_EQ(collect(s), (std::vector<int>{1, 2, 4, 5}));
    auto it = s.find(4);
    auto nx = s.erase(it);
    EXPECT_EQ(*nx, 5);

    std::vector<int> rev;
    for (auto r = s.rbegin(); r != s.rend(); ++r)
    {
        rev.push_back(*r);
    }
    EXPECT_EQ(rev, (std::vector<int>{5, 2, 1}));
}

TEST(SetTest, CopyMoveSwap)
{
    Set<int> a{3, 1, 2};
    Set<int> b = a;
    EXPECT_EQ(collect(b), (std::vector<int>{1, 2, 3}));
    b.insert(4);
    EXPECT_FALSE(a.contains(4));

    Set<int> c = static_cast<Set<int>&&>(a);
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(collect(c), (std::vector<int>{1, 2, 3}));

    Set<int> x{1, 2}, y{7, 8, 9};
    swap(x, y);
    EXPECT_EQ(collect(x), (std::vector<int>{7, 8, 9}));
    EXPECT_EQ(collect(y), (std::vector<int>{1, 2}));
}

TEST(SetTest, CustomComparatorDescending)
{
    Set<int, Greater<int>> s{1, 5, 3, 2, 4};
    EXPECT_EQ(collect(s), (std::vector<int>{5, 4, 3, 2, 1}));
    EXPECT_EQ(*s.begin(), 5);
}

namespace
{
    // A key comparable to a bare int in both directions -> transparent heterogeneous lookup.
    struct HKey
    {
        int v;
        friend bool operator<(HKey a, HKey b) noexcept { return a.v < b.v; }
        friend bool operator<(HKey a, int b) noexcept { return a.v < b; }
        friend bool operator<(int a, HKey b) noexcept { return a < b.v; }
    };
} // namespace

TEST(SetTest, TransparentHeterogeneousLookup)
{
    Set<HKey> s; // default Compare is the transparent Less<>
    s.insert(HKey{1});
    s.insert(HKey{3});
    s.insert(HKey{5});
    // Look up by a bare int -> no temporary HKey is constructed (R45).
    EXPECT_TRUE(s.contains(3));
    EXPECT_FALSE(s.contains(4));
    EXPECT_EQ(s.count(5), 1u);
    auto it = s.find(3);
    ASSERT_NE(it, s.end());
    EXPECT_EQ(it->v, 3);
    EXPECT_EQ(s.lowerBound(3)->v, 3);
    EXPECT_EQ(s.upperBound(3)->v, 5);
}
