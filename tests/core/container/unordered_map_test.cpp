#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.hash;
import worse.core.container.unordered_map;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    struct Tracked
    {
        static int alive;
        static int ctorCalls; // counts value constructions (default + explicit-int)
        int v = 0;
        Tracked()
        {
            ++alive;
            ++ctorCalls;
        }
        explicit Tracked(int x) : v(x)
        {
            ++alive;
            ++ctorCalls;
        }
        Tracked(Tracked const& o) : v(o.v) { ++alive; }
        Tracked(Tracked&& o) noexcept : v(o.v) { ++alive; }
        Tracked& operator=(Tracked const&)     = default;
        Tracked& operator=(Tracked&&) noexcept = default;
        ~Tracked() { --alive; }
        static void reset()
        {
            alive     = 0;
            ctorCalls = 0;
        }
    };
    int Tracked::alive     = 0;
    int Tracked::ctorCalls = 0;

    struct ConstantHash
    {
        usize operator()(int) const noexcept { return 7; }
    };
} // namespace

TEST(UnorderedMapTest, SubscriptInsertOverwriteDefault)
{
    UnorderedMap<int, int> m;
    m[2] = 20;
    m[1] = 10;
    m[3] = 30;
    EXPECT_EQ(m.size(), 3u);
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m[2], 20);

    m[2] = 222; // overwrite
    EXPECT_EQ(m.size(), 3u);
    EXPECT_EQ(m[2], 222);

    m[5]; // default-inserts 0
    EXPECT_EQ(m.size(), 4u);
    EXPECT_EQ(m[5], 0);
}

TEST(UnorderedMapTest, AtPresentAndMissing)
{
    UnorderedMap<int, int> m;
    m[7] = 70;
    EXPECT_EQ(m.at(7), 70);
    m.at(7) = 71;
    EXPECT_EQ(m.at(7), 71);

    using Map = UnorderedMap<int, int>;
    EXPECT_DEATH({ Map e; (void)e.at(99); }, "");
}

TEST(UnorderedMapTest, InsertReturnsFlag)
{
    UnorderedMap<int, int> m;
    auto r1 = m.insert(makePair(4, 40));
    EXPECT_TRUE(r1.second);
    EXPECT_EQ(r1.first->second, 40);

    auto r2 = m.insert(makePair(4, 999)); // exists -> no change
    EXPECT_FALSE(r2.second);
    EXPECT_EQ(r2.first->second, 40);
    EXPECT_EQ(m.size(), 1u);
}

TEST(UnorderedMapTest, InsertOrAssignAndTryEmplace)
{
    UnorderedMap<int, int> m;
    auto a = m.insertOrAssign(1, 10);
    EXPECT_TRUE(a.second);
    auto b = m.insertOrAssign(1, 11); // overwrites
    EXPECT_FALSE(b.second);
    EXPECT_EQ(m[1], 11);

    auto c = m.tryEmplace(1, 12); // exists -> not assigned
    EXPECT_FALSE(c.second);
    EXPECT_EQ(m[1], 11);

    auto d = m.tryEmplace(2, 22); // inserts
    EXPECT_TRUE(d.second);
    EXPECT_EQ(m[2], 22);
}

TEST(UnorderedMapTest, TryEmplaceDoesNotBuildOnExistingKey)
{
    Tracked::reset();
    {
        UnorderedMap<int, Tracked> m;
        m.tryEmplace(1, 100); // inserts -> builds one Tracked
        int const after1 = Tracked::ctorCalls;
        EXPECT_GE(after1, 1);

        m.tryEmplace(1, 200); // key exists -> must build ZERO Tracked
        EXPECT_EQ(Tracked::ctorCalls, after1);
        EXPECT_EQ(m.at(1).v, 100); // unchanged
    }
    EXPECT_EQ(Tracked::alive, 0);
}

TEST(UnorderedMapTest, FindContainsCountAndMutateThroughIterator)
{
    UnorderedMap<int, int> m{{1, 10}, {2, 20}, {3, 30}};
    EXPECT_EQ(m.size(), 3u);
    auto it = m.find(2);
    ASSERT_NE(it, m.end());
    EXPECT_EQ(it->second, 20);
    it->second = 222; // mutating the mapped value is allowed
    EXPECT_EQ(m[2], 222);

    EXPECT_EQ(m.find(9), m.end());
    EXPECT_TRUE(m.contains(1));
    EXPECT_FALSE(m.contains(9));
    EXPECT_EQ(m.count(3), 1u);
    EXPECT_EQ(m.count(8), 0u);
}

TEST(UnorderedMapTest, EraseByKeyAndIterator)
{
    UnorderedMap<int, int> m{{1, 1}, {2, 2}, {3, 3}, {4, 4}};
    EXPECT_EQ(m.erase(2), 1u);
    EXPECT_EQ(m.erase(2), 0u);
    EXPECT_EQ(m.size(), 3u);

    auto it = m.find(3);
    ASSERT_NE(it, m.end());
    auto nx = m.erase(it);
    EXPECT_FALSE(m.contains(3));
    EXPECT_EQ(m.size(), 2u);
    if (nx != m.end())
    {
        EXPECT_TRUE(m.contains(nx->first));
    }
}

TEST(UnorderedMapTest, BulkBuildDedupsKeys)
{
    Pair<int, int> data[] = {{3, 33}, {1, 11}, {2, 22}, {1, 99}, {3, 0}};
    UnorderedMap<int, int> m(data, data + 5);
    EXPECT_EQ(m.size(), 3u); // keys {1,2,3}
    EXPECT_TRUE(m.contains(1) && m.contains(2) && m.contains(3));
}

TEST(UnorderedMapTest, CollisionsViaCustomHash)
{
    UnorderedMap<int, int, ConstantHash> m;
    for (int i = 0; i < 64; ++i)
    {
        m[i] = i * 10;
    }
    EXPECT_EQ(m.size(), 64u);
    for (int i = 0; i < 64; ++i)
    {
        EXPECT_EQ(m.at(i), i * 10);
    }
    m.erase(40);
    EXPECT_FALSE(m.contains(40));
    EXPECT_EQ(m.size(), 63u);
}

TEST(UnorderedMapTest, GrowStressPreservesValues)
{
    UnorderedMap<int, int> m;
    for (int i = 0; i < 1000; ++i)
    {
        m[i * 5 + 2] = i;
    }
    EXPECT_EQ(m.size(), 1000u);
    for (int i = 0; i < 1000; ++i)
    {
        ASSERT_TRUE(m.contains(i * 5 + 2));
        EXPECT_EQ(m.at(i * 5 + 2), i);
    }
}

TEST(UnorderedMapTest, IteratorVisitsAllPairs)
{
    UnorderedMap<int, int> m;
    for (int i = 0; i < 50; ++i)
    {
        m[i] = i + 1000;
    }
    int count = 0;
    for (auto const& kv : m)
    {
        EXPECT_EQ(kv.second, kv.first + 1000);
        ++count;
    }
    EXPECT_EQ(count, 50);
}

TEST(UnorderedMapTest, Swap)
{
    UnorderedMap<int, int> a{{1, 1}, {2, 2}};
    UnorderedMap<int, int> b{{9, 9}};
    swap(a, b);
    EXPECT_EQ(a.size(), 1u);
    EXPECT_TRUE(a.contains(9));
    EXPECT_EQ(b.size(), 2u);
    EXPECT_TRUE(b.contains(1));
}

TEST(UnorderedMapTest, NoLeaks)
{
    Tracked::reset();
    {
        UnorderedMap<int, Tracked> m;
        for (int i = 0; i < 60; ++i)
        {
            m.insertOrAssign(i % 20, Tracked{i}); // collisions overwrite
        }
        EXPECT_LE(m.size(), 20u);
        m.erase(0);
        UnorderedMap<int, Tracked> copy = m;
        EXPECT_GT(Tracked::alive, 0);
        (void)copy;
    }
    EXPECT_EQ(Tracked::alive, 0);
}
