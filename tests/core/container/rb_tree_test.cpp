#include <gtest/gtest.h>

#include <set>
#include <vector>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.rb_tree;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    struct IntKeyOfValue
    {
        int const& operator()(int const& v) const noexcept { return v; }
    };
    using Tree = RBTree<int, int, IntKeyOfValue>;

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

    template <typename T>
    std::vector<int> collect(T const& tree)
    {
        std::vector<int> out;
        for (int v : tree)
        {
            out.push_back(v);
        }
        return out;
    }
} // namespace

TEST(RBTreeTest, IteratorModelsBidirectional)
{
    static_assert(BidirectionalIterator<Tree::Iterator>);
    static_assert(BidirectionalIterator<Tree::ConstIterator>);
    static_assert(IsSame<decltype(*Tree::ConstIterator{}), int const&>);
    SUCCEED();
}

TEST(RBTreeTest, InsertUniqueAndOrderedIteration)
{
    Tree t;
    int const in[] = {5, 3, 8, 1, 4, 7, 9, 2, 6, 0};
    for (int x : in)
    {
        auto r = t.insertUnique(x);
        EXPECT_TRUE(r.second);
        EXPECT_EQ(*r.first, x);
    }
    EXPECT_EQ(t.size(), 10u);
    EXPECT_EQ(collect(t), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    // duplicate rejected
    auto d = t.insertUnique(5);
    EXPECT_FALSE(d.second);
    EXPECT_EQ(*d.first, 5);
    EXPECT_EQ(t.size(), 10u);
    EXPECT_GE(t.checkInvariant(), 0);
}

TEST(RBTreeTest, FindContainsCountLowerUpperBound)
{
    Tree t;
    for (int x : {10, 20, 30, 40, 50})
    {
        t.insertUnique(x);
    }
    EXPECT_TRUE(t.contains(30));
    EXPECT_FALSE(t.contains(35));
    EXPECT_EQ(t.count(40), 1u);
    EXPECT_EQ(t.count(41), 0u);
    EXPECT_EQ(*t.find(20), 20);
    EXPECT_EQ(t.find(99), t.end());

    EXPECT_EQ(*t.lowerBound(30), 30); // first >= 30
    EXPECT_EQ(*t.lowerBound(31), 40); // first >= 31
    EXPECT_EQ(*t.upperBound(30), 40); // first > 30
    auto er = t.equalRange(30);
    EXPECT_EQ(*er.first, 30);
    EXPECT_EQ(*er.second, 40);
    EXPECT_EQ(t.lowerBound(100), t.end());
}

TEST(RBTreeTest, EraseByKeyAndIterator)
{
    Tree t;
    for (int i = 0; i < 16; ++i)
    {
        t.insertUnique(i);
    }
    EXPECT_EQ(t.erase(7), 1u);  // erase interior
    EXPECT_EQ(t.erase(7), 0u);  // already gone
    EXPECT_EQ(t.erase(0), 1u);  // erase leftmost
    EXPECT_EQ(t.erase(15), 1u); // erase rightmost
    EXPECT_EQ(t.size(), 13u);
    EXPECT_GE(t.checkInvariant(), 0);
    EXPECT_EQ(*t.begin(), 1);

    auto it = t.find(8);
    auto nx = t.erase(it);
    EXPECT_EQ(*nx, 9);
    EXPECT_GE(t.checkInvariant(), 0);
}

TEST(RBTreeTest, ReverseIteration)
{
    Tree t;
    for (int x : {3, 1, 2, 5, 4})
    {
        t.insertUnique(x);
    }
    std::vector<int> rev;
    for (auto it = t.rbegin(); it != t.rend(); ++it)
    {
        rev.push_back(*it);
    }
    EXPECT_EQ(rev, (std::vector<int>{5, 4, 3, 2, 1}));
}

TEST(RBTreeTest, CopyMoveSwap)
{
    Tree a;
    for (int x : {4, 2, 6, 1, 3, 5, 7})
    {
        a.insertUnique(x);
    }
    Tree b = a; // copy (structural clone)
    EXPECT_EQ(collect(b), collect(a));
    EXPECT_GE(b.checkInvariant(), 0);
    b.insertUnique(8);
    EXPECT_FALSE(a.contains(8)); // independent

    Tree c = static_cast<Tree&&>(a); // move (re-seats root parent)
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(collect(c), (std::vector<int>{1, 2, 3, 4, 5, 6, 7}));
    EXPECT_GE(c.checkInvariant(), 0);
    c.insertUnique(0); // mutate moved-to: header re-seat must be intact
    EXPECT_EQ(*c.begin(), 0);
    EXPECT_GE(c.checkInvariant(), 0);

    Tree x, y;
    for (int v : {1, 2, 3})
    {
        x.insertUnique(v);
    }
    for (int v : {10, 20})
    {
        y.insertUnique(v);
    }
    swap(x, y);
    EXPECT_EQ(collect(x), (std::vector<int>{10, 20}));
    EXPECT_EQ(collect(y), (std::vector<int>{1, 2, 3}));
    EXPECT_GE(x.checkInvariant(), 0);
    EXPECT_GE(y.checkInvariant(), 0);
    x.insertUnique(15);
    EXPECT_GE(x.checkInvariant(), 0);
}

TEST(RBTreeTest, NoLeak)
{
    EXPECT_EQ(Tracked::sAlive, 0);
    {
        RBTree<Tracked, int, TrackedKeyOfValue> t;
        for (int i = 0; i < 20; ++i)
        {
            t.insertUnique(Tracked{i});
        }
        EXPECT_EQ(Tracked::sAlive, 20);
        t.erase(5);
        t.erase(10);
        EXPECT_EQ(Tracked::sAlive, 18);
        RBTree<Tracked, int, TrackedKeyOfValue> copy = t;
        EXPECT_EQ(Tracked::sAlive, 36);
        t.clear();
        EXPECT_EQ(Tracked::sAlive, 18);
    }
    EXPECT_EQ(Tracked::sAlive, 0);
}

// The real correctness net: thousands of randomized insert/erase ops cross-checked against
// std::set, with the full red-black + BST invariant verified after every single mutation.
TEST(RBTreeTest, RandomizedStressAgainstStdSet)
{
    Tree t;
    std::set<int> ref;
    u32 state = 0xC0FFEEu;
    auto next = [&]
    {
        state = state * 1664525u + 1013904223u;
        return state;
    };

    for (int op = 0; op < 6000; ++op)
    {
        int const key = static_cast<int>(next() % 256u);
        if ((next() & 1u) != 0u)
        {
            bool const wasNew = ref.insert(key).second;
            auto const r      = t.insertUnique(key);
            EXPECT_EQ(r.second, wasNew);
            EXPECT_EQ(*r.first, key);
        }
        else
        {
            usize const refErased = ref.erase(key);
            EXPECT_EQ(t.erase(key), refErased);
        }
        ASSERT_GE(t.checkInvariant(), 0) << "invariant broken after op " << op;
        ASSERT_EQ(t.size(), ref.size());
    }
    // Full content + order equality.
    std::vector<int> mine = collect(t);
    std::vector<int> theirs(ref.begin(), ref.end());
    EXPECT_EQ(mine, theirs);

    // Drain everything via erase(begin) and confirm it empties cleanly.
    while (!t.empty())
    {
        t.erase(t.begin());
        ASSERT_GE(t.checkInvariant(), 0);
    }
    EXPECT_EQ(t.size(), 0u);
    EXPECT_EQ(t.begin(), t.end());
}
