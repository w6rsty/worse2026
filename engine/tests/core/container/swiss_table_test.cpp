#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <vector>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.hash;
import worse.core.container.swiss_table;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    struct IntKeyOfValue
    {
        int const& operator()(int const& v) const noexcept { return v; }
    };
    using Table = SwissTable<int, int, IntKeyOfValue>;

    struct Tracked
    {
        static int sAlive;
        int v = 0;
        Tracked() { ++sAlive; }
        explicit Tracked(int x) : v(x) { ++sAlive; }
        Tracked(Tracked const& o) : v(o.v) { ++sAlive; }
        Tracked(Tracked&& o) noexcept : v(o.v) { ++sAlive; }
        Tracked& operator=(Tracked const&)     = default;
        Tracked& operator=(Tracked&&) noexcept = default;
        ~Tracked() { --sAlive; }
    };
    int Tracked::sAlive = 0;
    struct TrackedKeyOfValue
    {
        int const& operator()(Tracked const& t) const noexcept { return t.v; }
    };
    struct TrackedHash
    {
        usize operator()(int v) const noexcept { return Hash<int>{}(v); }
    };
    struct TrackedEq
    {
        bool operator()(int a, int b) const noexcept { return a == b; }
    };

    template <typename T>
    std::vector<int> sortedContents(T const& t)
    {
        std::vector<int> out;
        for (int v : t)
        {
            out.push_back(v);
        }
        std::sort(out.begin(), out.end());
        return out;
    }
} // namespace

TEST(SwissTableTest, IteratorModelsForward)
{
    static_assert(ForwardIterator<Table::Iterator>);
    static_assert(ForwardIterator<Table::ConstIterator>);
    SUCCEED();
}

TEST(SwissTableTest, InsertFindEraseBasics)
{
    Table t;
    EXPECT_TRUE(t.empty());
    for (int x : {5, 3, 8, 1, 9, 2})
    {
        EXPECT_TRUE(t.insertUnique(x).second);
    }
    EXPECT_EQ(t.size(), 6u);
    EXPECT_FALSE(t.insertUnique(5).second); // dup
    EXPECT_EQ(t.size(), 6u);

    EXPECT_TRUE(t.contains(8));
    EXPECT_FALSE(t.contains(7));
    EXPECT_EQ(t.count(3), 1u);
    EXPECT_EQ(*t.find(9), 9);
    EXPECT_EQ(t.find(100), t.end());

    EXPECT_EQ(t.erase(8), 1u);
    EXPECT_EQ(t.erase(8), 0u);
    EXPECT_FALSE(t.contains(8));
    EXPECT_EQ(t.size(), 5u);
    EXPECT_EQ(sortedContents(t), (std::vector<int>{1, 2, 3, 5, 9}));
}

TEST(SwissTableTest, GrowthThroughManyInserts)
{
    Table t;
    for (int i = 0; i < 1000; ++i)
    {
        t.insertUnique(i * 7);
    }
    EXPECT_EQ(t.size(), 1000u);
    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_TRUE(t.contains(i * 7));
    }
    EXPECT_FALSE(t.contains(3)); // 3 is not a multiple of 7
    EXPECT_LE(t.loadFactor(), 0.875f);
}

TEST(SwissTableTest, TombstoneReuse)
{
    Table t;
    for (int i = 0; i < 50; ++i)
    {
        t.insertUnique(i);
    }
    // Erase then re-insert many times: tombstones must be reclaimed, no unbounded growth.
    for (int round = 0; round < 500; ++round)
    {
        EXPECT_EQ(t.erase(round % 50), 1u);
        EXPECT_TRUE(t.insertUnique(round % 50).second);
        EXPECT_EQ(t.size(), 50u);
    }
    EXPECT_EQ(sortedContents(t).size(), 50u);
}

TEST(SwissTableTest, ReserveReclaimsTombstonesForBoundedInsert)
{
    Table t;
    t.reserve(200);
    for (int i = 0; i < 160; ++i) // load up near 7/8 of the reserved capacity
    {
        t.insertUnique(i);
    }
    for (int i = 0; i < 150; ++i) // erase most -> many tombstones (size drops, deleted high)
    {
        EXPECT_EQ(t.erase(i), 1u);
    }
    EXPECT_EQ(t.size(), 10u);

    // Reserve for the count we intend to reach: reclaims tombstones in place (R45) so the
    // following burst is rehash-free even though plain reserve(n>maxLoad) would not have grown.
    t.reserve(160);
    EXPECT_FALSE(t.wouldRehashOnInsert());
    auto const capBefore = t.capacity();
    for (int i = 1000; i < 1150; ++i) // 150 fresh inserts -> 160 live total
    {
        t.insertUnique(i);
    }
    EXPECT_EQ(t.capacity(), capBefore); // no rehash/grow during the burst
    EXPECT_EQ(t.size(), 160u);
}

TEST(SwissTableTest, ClearCopyMoveSwap)
{
    Table a;
    for (int x : {1, 2, 3, 4, 5})
    {
        a.insertUnique(x);
    }
    Table b = a; // copy
    EXPECT_EQ(sortedContents(b), (std::vector<int>{1, 2, 3, 4, 5}));
    b.insertUnique(6);
    EXPECT_FALSE(a.contains(6)); // independent

    Table c = static_cast<Table&&>(a); // move
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(sortedContents(c), (std::vector<int>{1, 2, 3, 4, 5}));

    Table x, y;
    for (int v : {10, 20})
    {
        x.insertUnique(v);
    }
    for (int v : {1, 2, 3})
    {
        y.insertUnique(v);
    }
    swap(x, y);
    EXPECT_EQ(sortedContents(x), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(sortedContents(y), (std::vector<int>{10, 20}));

    c.clear();
    EXPECT_TRUE(c.empty());
    EXPECT_EQ(c.size(), 0u);
    c.insertUnique(42);
    EXPECT_TRUE(c.contains(42));
}

TEST(SwissTableTest, NoLeak)
{
    EXPECT_EQ(Tracked::sAlive, 0);
    {
        SwissTable<Tracked, int, TrackedKeyOfValue, TrackedHash, TrackedEq> t;
        for (int i = 0; i < 200; ++i)
        {
            t.insertUnique(Tracked{i});
        }
        EXPECT_EQ(Tracked::sAlive, 200);
        for (int i = 0; i < 100; ++i)
        {
            t.erase(i);
        }
        EXPECT_EQ(Tracked::sAlive, 100);
        SwissTable<Tracked, int, TrackedKeyOfValue, TrackedHash, TrackedEq> copy = t;
        EXPECT_EQ(Tracked::sAlive, 200);
        t.clear();
        EXPECT_EQ(Tracked::sAlive, 100);
    }
    EXPECT_EQ(Tracked::sAlive, 0);
}

// Thousands of randomized insert/erase/find ops cross-checked against std::set.
TEST(SwissTableTest, RandomizedStressAgainstStdSet)
{
    Table t;
    std::set<int> ref;
    u32 state = 0x12345u;
    auto next = [&]
    {
        state = state * 1664525u + 1013904223u;
        return state;
    };

    for (int op = 0; op < 8000; ++op)
    {
        int const key = static_cast<int>(next() % 300u);
        if ((next() & 1u) != 0u)
        {
            bool const wasNew = ref.insert(key).second;
            EXPECT_EQ(t.insertUnique(key).second, wasNew);
        }
        else
        {
            EXPECT_EQ(t.erase(key), static_cast<usize>(ref.erase(key)));
        }
        ASSERT_EQ(t.size(), ref.size()) << "size mismatch after op " << op;
        ASSERT_EQ(t.contains(key), ref.count(key) != 0);
    }
    std::vector<int> mine = sortedContents(t);
    std::vector<int> theirs(ref.begin(), ref.end());
    EXPECT_EQ(mine, theirs);
}
