#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.flat_map;

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
    };
    int Tracked::alive = 0;

    template <typename Map>
    bool keysAscending(Map const& m)
    {
        auto it = m.begin();
        if (it == m.end())
        {
            return true;
        }
        auto prev = it;
        for (++it; it != m.end(); ++it, ++prev)
        {
            if (!(prev->first < it->first))
            {
                return false;
            }
        }
        return true;
    }
} // namespace

TEST(FlatMapTest, SubscriptInsertsAndReturnsRef)
{
    FlatMap<int, int> m;
    m[2] = 20;
    m[1] = 10;
    m[3] = 30;
    EXPECT_EQ(m.size(), 3u);
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m[2], 20);
    EXPECT_TRUE(keysAscending(m));

    m[2] = 222; // overwrite, no new element
    EXPECT_EQ(m.size(), 3u);
    EXPECT_EQ(m[2], 222);

    m[5]; // default-inserts value-initialized 0
    EXPECT_EQ(m.size(), 4u);
    EXPECT_EQ(m[5], 0);
}

TEST(FlatMapTest, AtPresentAndMissing)
{
    FlatMap<int, int> m;
    m[7] = 70;
    EXPECT_EQ(m.at(7), 70);
    m.at(7) = 71;
    EXPECT_EQ(m.at(7), 71);

    using Map = FlatMap<int, int>;
    EXPECT_DEATH({ Map e; (void)e.at(99); }, "");
}

TEST(FlatMapTest, InsertReturnsInsertedFlag)
{
    FlatMap<int, int> m;
    auto r1 = m.insert(makePair(4, 40));
    EXPECT_TRUE(r1.second);
    EXPECT_EQ(r1.first->second, 40);

    auto r2 = m.insert(makePair(4, 999)); // key exists -> no change
    EXPECT_FALSE(r2.second);
    EXPECT_EQ(r2.first->second, 40);
    EXPECT_EQ(m.size(), 1u);
}

TEST(FlatMapTest, InsertOrAssignAndTryEmplace)
{
    FlatMap<int, int> m;
    auto a = m.insertOrAssign(1, 10);
    EXPECT_TRUE(a.second);
    auto b = m.insertOrAssign(1, 11); // overwrites
    EXPECT_FALSE(b.second);
    EXPECT_EQ(m[1], 11);

    auto c = m.tryEmplace(1, 12); // key exists -> NOT assigned
    EXPECT_FALSE(c.second);
    EXPECT_EQ(m[1], 11);

    auto d = m.tryEmplace(2, 22); // inserts
    EXPECT_TRUE(d.second);
    EXPECT_EQ(m[2], 22);
}

TEST(FlatMapTest, FindContainsCountAndMutateThroughIterator)
{
    FlatMap<int, int> m{{1, 10}, {2, 20}, {3, 30}};
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

TEST(FlatMapTest, EraseByKeyAndIterator)
{
    FlatMap<int, int> m{{1, 1}, {2, 2}, {3, 3}, {4, 4}};
    EXPECT_EQ(m.erase(2), 1u);
    EXPECT_EQ(m.erase(2), 0u);
    EXPECT_EQ(m.size(), 3u);

    auto it = m.find(3);
    auto nx = m.erase(it); // returns iterator to next (key 4)
    EXPECT_EQ(nx->first, 4);
    EXPECT_FALSE(m.contains(3));
    EXPECT_TRUE(keysAscending(m));
}

TEST(FlatMapTest, LowerUpperEqualRange)
{
    FlatMap<int, int> m{{10, 1}, {20, 2}, {30, 3}};
    EXPECT_EQ(m.lowerBound(20)->first, 20);
    EXPECT_EQ(m.upperBound(20)->first, 30);
    EXPECT_EQ(m.lowerBound(25)->first, 30);

    auto er = m.equalRange(20);
    EXPECT_EQ(er.second - er.first, 1);
    EXPECT_EQ(er.first->second, 2);
}

TEST(FlatMapTest, BulkBuildDedupsKeys)
{
    Pair<int, int> data[] = {{3, 33}, {1, 11}, {2, 22}, {1, 99}, {3, 0}};
    FlatMap<int, int> m(data, data + 5);
    EXPECT_EQ(m.size(), 3u); // keys {1,2,3}
    EXPECT_TRUE(keysAscending(m));
    EXPECT_TRUE(m.contains(1) && m.contains(2) && m.contains(3));
}

TEST(FlatMapTest, CustomComparatorDescending)
{
    FlatMap<int, int, Greater<>> m;
    m[1]              = 1;
    m[3]              = 3;
    m[2]              = 2;
    int expectedKey[] = {3, 2, 1};
    int i             = 0;
    for (auto const& kv : m)
    {
        EXPECT_EQ(kv.first, expectedKey[i++]);
    }
    EXPECT_NE(m.find(2), m.end());
}

TEST(FlatMapTest, TransparentHeterogeneousLookup)
{
    FlatMap<long, int> m{{1L, 1}, {2L, 2}, {3L, 3}};
    int query = 2;
    EXPECT_TRUE(m.contains(query));
    EXPECT_NE(m.find(query), m.end());
}

TEST(FlatMapTest, Swap)
{
    FlatMap<int, int> a{{1, 1}, {2, 2}};
    FlatMap<int, int> b{{9, 9}};
    swap(a, b);
    EXPECT_EQ(a.size(), 1u);
    EXPECT_TRUE(a.contains(9));
    EXPECT_EQ(b.size(), 2u);
    EXPECT_TRUE(b.contains(1));
}

TEST(FlatMapTest, NoLeaks)
{
    Tracked::reset();
    {
        FlatMap<int, Tracked> m;
        for (int i = 0; i < 20; ++i)
        {
            m.insertOrAssign(i % 7, Tracked{i}); // collisions overwrite
        }
        EXPECT_LE(m.size(), 7u);
        m.erase(0);
        FlatMap<int, Tracked> copy = m;
        EXPECT_GT(Tracked::alive, 0);
        (void)copy;
    }
    EXPECT_EQ(Tracked::alive, 0);
}
