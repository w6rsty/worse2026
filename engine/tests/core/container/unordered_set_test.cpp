#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.hash;
import worse.core.container.unordered_set;

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
        static void reset() { alive = 0; }
        friend bool operator==(Tracked const& a, Tracked const& b) { return a.v == b.v; }
    };
    int Tracked::alive = 0;

    struct TrackedHash
    {
        usize operator()(Tracked const& t) const noexcept { return Hash<int>{}(t.v); }
    };

    struct ConstantHash
    {
        usize operator()(int) const noexcept { return 42; }
    };
} // namespace

TEST(UnorderedSetTest, IteratorsAreConst)
{
    UnorderedSet<int> s;
    s.insert(1);
    static_assert(IsSame<decltype(*s.begin()), int const&>);
}

TEST(UnorderedSetTest, InsertUniqueAndDuplicateFlag)
{
    UnorderedSet<int> s;
    auto a = s.insert(5);
    EXPECT_TRUE(a.second);
    EXPECT_EQ(*a.first, 5);
    auto b = s.insert(5);
    EXPECT_FALSE(b.second);
    EXPECT_EQ(s.size(), 1u);

    s.emplace(6);
    s.emplace(6);
    EXPECT_EQ(s.size(), 2u);
}

TEST(UnorderedSetTest, FindContainsCount)
{
    UnorderedSet<int> s{1, 2, 3};
    EXPECT_EQ(s.size(), 3u);
    EXPECT_NE(s.find(2), s.end());
    EXPECT_EQ(s.find(99), s.end());
    EXPECT_TRUE(s.contains(1));
    EXPECT_FALSE(s.contains(99));
    EXPECT_EQ(s.count(3), 1u);
    EXPECT_EQ(s.count(8), 0u);
}

TEST(UnorderedSetTest, EraseByKeyAndIterator)
{
    UnorderedSet<int> s{1, 2, 3, 4};
    EXPECT_EQ(s.erase(2), 1u);
    EXPECT_EQ(s.erase(2), 0u);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_FALSE(s.contains(2));

    auto it = s.find(3);
    ASSERT_NE(it, s.end());
    auto nx = s.erase(it);
    EXPECT_FALSE(s.contains(3));
    EXPECT_EQ(s.size(), 2u);
    if (nx != s.end())
    {
        EXPECT_TRUE(s.contains(*nx));
    }
}

TEST(UnorderedSetTest, BulkBuildDedups)
{
    int data[] = {3, 1, 2, 1, 3, 2, 5};
    UnorderedSet<int> s(data, data + 7);
    EXPECT_EQ(s.size(), 4u); // {1,2,3,5}
    EXPECT_TRUE(s.contains(1) && s.contains(2) && s.contains(3) && s.contains(5));
}

TEST(UnorderedSetTest, IteratorVisitsAll)
{
    UnorderedSet<int> s;
    for (int i = 0; i < 100; ++i)
    {
        s.insert(i);
    }
    int count      = 0;
    bool seen[100] = {};
    for (int v : s)
    {
        ASSERT_GE(v, 0);
        ASSERT_LT(v, 100);
        EXPECT_FALSE(seen[v]);
        seen[v] = true;
        ++count;
    }
    EXPECT_EQ(count, 100);
}

TEST(UnorderedSetTest, CollisionsViaCustomHash)
{
    UnorderedSet<int, ConstantHash> s;
    for (int i = 0; i < 64; ++i)
    {
        s.insert(i);
    }
    EXPECT_EQ(s.size(), 64u);
    for (int i = 0; i < 64; ++i)
    {
        EXPECT_TRUE(s.contains(i));
    }
    s.erase(32);
    EXPECT_FALSE(s.contains(32));
    EXPECT_EQ(s.size(), 63u);
}

TEST(UnorderedSetTest, InsertEraseStress)
{
    UnorderedSet<int> s;
    for (int i = 0; i < 1000; ++i)
    {
        s.insert(i * 3);
    }
    EXPECT_EQ(s.size(), 1000u);
    for (int i = 0; i < 1000; i += 2)
    {
        s.erase(i * 3);
    }
    EXPECT_EQ(s.size(), 500u);
    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_EQ(s.contains(i * 3), (i % 2) != 0);
    }
}

TEST(UnorderedSetTest, Swap)
{
    UnorderedSet<int> a{1, 2, 3};
    UnorderedSet<int> b{9};
    swap(a, b);
    EXPECT_EQ(a.size(), 1u);
    EXPECT_TRUE(a.contains(9));
    EXPECT_EQ(b.size(), 3u);
    EXPECT_TRUE(b.contains(1));
}

TEST(UnorderedSetTest, CustomKeyTypeWithCustomHash)
{
    UnorderedSet<Tracked, TrackedHash> s;
    s.emplace(1);
    s.emplace(2);
    s.emplace(1); // duplicate by value
    EXPECT_EQ(s.size(), 2u);
    EXPECT_TRUE(s.contains(Tracked{2}));
}

TEST(UnorderedSetTest, NoLeaks)
{
    Tracked::reset();
    {
        UnorderedSet<Tracked, TrackedHash> s;
        for (int i = 0; i < 50; ++i)
        {
            s.emplace(i);
        }
        EXPECT_EQ(Tracked::alive, 50);
        s.erase(Tracked{10});
        UnorderedSet<Tracked, TrackedHash> copy = s;
        EXPECT_GT(Tracked::alive, 0);
        (void)copy;
    }
    EXPECT_EQ(Tracked::alive, 0);
}
