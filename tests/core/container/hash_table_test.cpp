#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.hash;
import worse.core.container.hash_table;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    // Identity extractor: the value IS its key (the set shape).
    struct IntIdentity
    {
        int const& operator()(int const& v) const noexcept { return v; }
    };

    using IntTable = HashTable<int, int, IntIdentity, Hash<int>, EqualTo<int>>;

    // Degenerate hash: every key collides at bucket 0, forcing maximal probing -- the
    // hardest case for Robin Hood insert / backward-shift erase.
    struct BadHash
    {
        usize operator()(int) const noexcept { return 0; }
    };
    using BadTable = HashTable<int, int, IntIdentity, BadHash, EqualTo<int>>;

    // A lifetime-counting value keyed by its `v`. Exercises construct/destroy through the
    // swap-displacement, backward-shift, rehash, and clear paths.
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

    struct TrackedKeyOf
    {
        int const& operator()(Tracked const& t) const noexcept { return t.v; }
    };
    using TrackedTable = HashTable<Tracked, int, TrackedKeyOf, Hash<int>, EqualTo<int>>;
} // namespace

TEST(HashTableTest, InsertFindRoundTrip)
{
    IntTable t;
    for (int i = 0; i < 50; ++i)
    {
        auto r = t.insertUnique(i);
        EXPECT_TRUE(r.second);
        EXPECT_EQ(*r.first, i);
    }
    EXPECT_EQ(t.size(), 50u);
    for (int i = 0; i < 50; ++i)
    {
        EXPECT_TRUE(t.contains(i));
        EXPECT_EQ(t.count(i), 1u);
        EXPECT_NE(t.find(i), t.end());
    }
    EXPECT_FALSE(t.contains(999));
    EXPECT_EQ(t.find(999), t.end());
    EXPECT_TRUE(t.checkRobinHoodInvariant());
}

TEST(HashTableTest, DuplicateInsertReturnsFalse)
{
    IntTable t;
    auto a = t.insertUnique(7);
    EXPECT_TRUE(a.second);
    auto b = t.insertUnique(7);
    EXPECT_FALSE(b.second);
    EXPECT_EQ(b.first, a.first);
    EXPECT_EQ(t.size(), 1u);
}

TEST(HashTableTest, EmplaceAndBulkInsert)
{
    IntTable t;
    t.emplace(5);
    t.emplace(5); // duplicate, no-op
    EXPECT_EQ(t.size(), 1u);

    int data[] = {10, 20, 30, 20, 10};
    t.insert(data, data + 5);
    EXPECT_EQ(t.size(), 4u); // {5,10,20,30}
    EXPECT_TRUE(t.contains(30));
    EXPECT_TRUE(t.checkRobinHoodInvariant());
}

TEST(HashTableTest, ForcedCollisionsAllProbing)
{
    BadTable t;
    constexpr int n = 200;
    for (int i = 0; i < n; ++i)
    {
        EXPECT_TRUE(t.insertUnique(i).second);
    }
    EXPECT_EQ(t.size(), static_cast<usize>(n));
    EXPECT_TRUE(t.checkRobinHoodInvariant());
    for (int i = 0; i < n; ++i)
    {
        EXPECT_TRUE(t.contains(i)) << "missing " << i;
    }
    // Erase every third element; survivors must remain findable.
    for (int i = 0; i < n; i += 3)
    {
        EXPECT_EQ(t.eraseByKey(i), 1u);
    }
    EXPECT_TRUE(t.checkRobinHoodInvariant());
    for (int i = 0; i < n; ++i)
    {
        EXPECT_EQ(t.contains(i), (i % 3) != 0);
    }
}

TEST(HashTableTest, BackwardShiftEraseMidCluster)
{
    BadTable t; // everything collides -> one long probe run
    for (int i = 0; i < 10; ++i)
    {
        t.insertUnique(i);
    }
    EXPECT_EQ(t.eraseByKey(5), 1u);
    EXPECT_EQ(t.eraseByKey(5), 0u); // already gone
    EXPECT_FALSE(t.contains(5));
    EXPECT_EQ(t.size(), 9u);
    for (int i = 0; i < 10; ++i)
    {
        if (i != 5)
        {
            EXPECT_TRUE(t.contains(i));
        }
    }
    EXPECT_TRUE(t.checkRobinHoodInvariant());
}

TEST(HashTableTest, GrowPreservesAllElementsAndBoundsLoad)
{
    IntTable t;
    usize const initialBuckets = t.bucketCount();
    for (int i = 0; i < 1000; ++i)
    {
        t.insertUnique(i * 7 + 1);
        EXPECT_LE(t.loadFactor(), t.maxLoadFactor());
    }
    EXPECT_EQ(t.size(), 1000u);
    EXPECT_GT(t.bucketCount(), initialBuckets);
    EXPECT_TRUE(t.checkRobinHoodInvariant());
    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_TRUE(t.contains(i * 7 + 1));
    }
}

TEST(HashTableTest, ReserveAvoidsRehash)
{
    IntTable t;
    t.reserve(500);
    usize const buckets = t.bucketCount();
    EXPECT_GE(buckets, 512u); // 500 / (7/8) rounded up to a power of two
    for (int i = 0; i < 400; ++i)
    {
        t.insertUnique(i);
    }
    EXPECT_EQ(t.bucketCount(), buckets); // no rehash happened
    EXPECT_TRUE(t.checkRobinHoodInvariant());
}

TEST(HashTableTest, IteratorVisitsEveryElementOnce)
{
    IntTable t;
    for (int i = 0; i < 64; ++i)
    {
        t.insertUnique(i);
    }
    int seen[64]  = {};
    usize visited = 0;
    for (int v : t)
    {
        ASSERT_GE(v, 0);
        ASSERT_LT(v, 64);
        ++seen[v];
        ++visited;
    }
    EXPECT_EQ(visited, 64u);
    for (int i = 0; i < 64; ++i)
    {
        EXPECT_EQ(seen[i], 1);
    }
}

TEST(HashTableTest, EmptyBeginEqualsEnd)
{
    IntTable t;
    EXPECT_TRUE(t.empty());
    EXPECT_EQ(t.begin(), t.end());
    EXPECT_EQ(t.bucketCount(), 0u); // lazy: no allocation until first insert
    t.insertUnique(1);
    EXPECT_NE(t.begin(), t.end());
    t.clear();
    EXPECT_EQ(t.begin(), t.end());
    EXPECT_EQ(t.size(), 0u);
}

TEST(HashTableTest, EraseByIteratorReturnsValidIterator)
{
    IntTable t;
    for (int i = 0; i < 20; ++i)
    {
        t.insertUnique(i);
    }
    auto it = t.find(10);
    ASSERT_NE(it, t.end());
    auto nx = t.eraseByIterator(it);
    EXPECT_FALSE(t.contains(10));
    EXPECT_EQ(t.size(), 19u);
    // nx is either end() or points at a live element.
    if (nx != t.end())
    {
        EXPECT_TRUE(t.contains(*nx));
    }
    EXPECT_TRUE(t.checkRobinHoodInvariant());
}

TEST(HashTableTest, CopyAndMove)
{
    IntTable a;
    for (int i = 0; i < 30; ++i)
    {
        a.insertUnique(i);
    }
    IntTable b = a; // copy
    EXPECT_EQ(b.size(), 30u);
    EXPECT_TRUE(b.checkRobinHoodInvariant());
    for (int i = 0; i < 30; ++i)
    {
        EXPECT_TRUE(b.contains(i));
    }
    a.insertUnique(999);
    EXPECT_FALSE(b.contains(999)); // deep copy: independent

    IntTable c = worse::core::move(b); // move
    EXPECT_EQ(c.size(), 30u);
    EXPECT_EQ(b.size(), 0u);
    EXPECT_TRUE(c.contains(15));
    EXPECT_TRUE(c.checkRobinHoodInvariant());
}

TEST(HashTableTest, Swap)
{
    IntTable a;
    IntTable bb;
    for (int i = 0; i < 10; ++i)
    {
        a.insertUnique(i);
    }
    for (int i = 100; i < 105; ++i)
    {
        bb.insertUnique(i);
    }
    swap(a, bb);
    EXPECT_EQ(a.size(), 5u);
    EXPECT_TRUE(a.contains(100));
    EXPECT_EQ(bb.size(), 10u);
    EXPECT_TRUE(bb.contains(5));
}

TEST(HashTableTest, NoLeaksAcrossAllPaths)
{
    Tracked::reset();
    {
        TrackedTable t;
        for (int i = 0; i < 100; ++i)
        {
            t.emplace(i); // builds Tracked(i); forces several rehashes
        }
        EXPECT_EQ(Tracked::alive, 100);
        EXPECT_TRUE(t.checkRobinHoodInvariant());

        for (int i = 0; i < 50; ++i)
        {
            t.eraseByKey(i); // backward-shift relocations
        }
        EXPECT_EQ(Tracked::alive, 50);

        TrackedTable copy = t;
        EXPECT_EQ(Tracked::alive, 100);
        (void)copy;

        t.clear();
        EXPECT_EQ(Tracked::alive, 50); // only `copy` remains
    }
    EXPECT_EQ(Tracked::alive, 0); // everything destroyed at scope exit
}

// Heavy-collision stress on the lifetime-counting type: the constant-bucket BadHash funnels
// all elements into one probe run, maximising swap-displacement and backward-shift work.
TEST(HashTableTest, CollisionLifetimeStress)
{
    Tracked::reset();
    {
        HashTable<Tracked, int, TrackedKeyOf, BadHash, EqualTo<int>> t;
        for (int i = 0; i < 80; ++i)
        {
            t.emplace(i);
        }
        EXPECT_EQ(Tracked::alive, 80);
        EXPECT_TRUE(t.checkRobinHoodInvariant());
        for (int i = 0; i < 80; i += 2)
        {
            t.eraseByKey(i);
        }
        EXPECT_EQ(Tracked::alive, 40);
        EXPECT_TRUE(t.checkRobinHoodInvariant());
    }
    EXPECT_EQ(Tracked::alive, 0);
}
