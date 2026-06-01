#include <gtest/gtest.h>

#include <string>
#include <vector>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.list;
import worse.core.container.fixed_list;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
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

    template <typename L>
    std::string seq(L const& list)
    {
        std::string s;
        for (auto const& v : list)
        {
            s += static_cast<char>('0' + v);
        }
        return s;
    }

    template <typename L>
    std::vector<int> collect(L const& list)
    {
        std::vector<int> out;
        for (auto const& v : list)
        {
            out.push_back(v);
        }
        return out;
    }
} // namespace

TEST(FixedListTest, IteratorModelsBidirectionalAndInteropsWithList)
{
    static_assert(BidirectionalIterator<FixedList<int, 8>::Iterator>);
    static_assert(!RandomAccessIterator<FixedList<int, 8>::Iterator>);
    // Reuses List's node + iterator types -> the iterator type is literally List's.
    static_assert(IsSame<FixedList<int, 8>::Iterator, List<int>::Iterator>);
    static_assert(IsSame<decltype(*FixedList<int, 8>::ConstIterator{}), int const&>);
    SUCCEED();
}

TEST(FixedListTest, PushPopFrontBackSizeFullCapacity)
{
    FixedList<int, 4> l;
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.capacity(), 4u);
    l.pushBack(2);
    l.pushBack(3);
    l.pushFront(1);
    EXPECT_EQ(seq(l), "123");
    EXPECT_EQ(l.front(), 1);
    EXPECT_EQ(l.back(), 3);
    l.pushBack(4);
    EXPECT_TRUE(l.full());
    EXPECT_EQ(l.size(), 4u);
    l.popFront();
    l.popBack();
    EXPECT_EQ(seq(l), "23");
    EXPECT_FALSE(l.full());
}

TEST(FixedListTest, FreeListRecyclesSlots)
{
    // Cycle pop+push at full capacity many more times than N: the inline pool must recycle
    // the freed slot each round and never overflow.
    FixedList<int, 3> l;
    l.pushBack(1);
    l.pushBack(2);
    l.pushBack(3);
    EXPECT_TRUE(l.full());
    for (int round = 0; round < 100; ++round)
    {
        l.popFront();          // frees a slot
        l.pushBack(round % 9); // must reuse it
        EXPECT_EQ(l.size(), 3u);
    }
    EXPECT_EQ(l.size(), 3u);

    // Fill exactly, clear, refill -- repeatedly -- with no leak/exhaustion.
    for (int round = 0; round < 50; ++round)
    {
        FixedList<int, 3> f;
        f.pushBack(1);
        f.pushBack(2);
        f.pushBack(3);
        EXPECT_TRUE(f.full());
        f.clear();
        EXPECT_TRUE(f.empty());
        f.pushBack(9);
        EXPECT_EQ(f.front(), 9);
    }
}

TEST(FixedListTest, EmplaceInsertEraseResize)
{
    FixedList<int, 8> l{1, 4};
    auto it = l.begin();
    ++it;
    auto r = l.insert(it, 2); // 1 2 4
    EXPECT_EQ(*r, 2);
    auto it2 = l.begin();
    ++it2;
    ++it2;                          // at 4
    auto r2 = l.insert(it2, 2u, 3); // 1 2 3 3 4
    EXPECT_EQ(*r2, 3);
    EXPECT_EQ(seq(l), "12334");

    auto e = l.begin();
    ++e;                  // at 2
    auto nx = l.erase(e); // 1 3 3 4
    EXPECT_EQ(*nx, 3);
    EXPECT_EQ(seq(l), "1334");

    l.resize(2); // 1 3
    EXPECT_EQ(seq(l), "13");
    l.resize(4, 9); // 1 3 9 9
    EXPECT_EQ(seq(l), "1399");
}

TEST(FixedListTest, CopyMoveAreElementWiseAndIndependent)
{
    FixedList<int, 8> a{1, 2, 3};
    FixedList<int, 8> b = a; // copy
    EXPECT_EQ(seq(b), "123");
    b.front() = 9;
    EXPECT_EQ(a.front(), 1); // independent storage

    FixedList<int, 8> c{4, 5, 6};
    FixedList<int, 8> d = static_cast<FixedList<int, 8>&&>(c); // move (element-wise)
    EXPECT_EQ(seq(d), "456");
    EXPECT_TRUE(c.empty());

    a = b; // copy assign
    EXPECT_EQ(seq(a), "923");
    a = static_cast<FixedList<int, 8>&&>(d); // move assign
    EXPECT_EQ(seq(a), "456");
    EXPECT_TRUE(d.empty());
}

TEST(FixedListTest, Swap)
{
    FixedList<int, 8> x{1, 2};
    FixedList<int, 8> y{3, 4, 5};
    swap(x, y);
    EXPECT_EQ(seq(x), "345");
    EXPECT_EQ(x.size(), 3u);
    EXPECT_EQ(seq(y), "12");
    // ring integrity after the element-wise swap
    EXPECT_EQ(x.front(), 3);
    EXPECT_EQ(x.back(), 5);
    x.pushBack(6);
    EXPECT_EQ(seq(x), "3456");
}

TEST(FixedListTest, ForwardAndReverseIteration)
{
    FixedList<int, 8> l{1, 2, 3, 4};
    std::string rev;
    for (auto it = l.rbegin(); it != l.rend(); ++it)
    {
        rev += static_cast<char>('0' + *it);
    }
    EXPECT_EQ(rev, "4321");
}

TEST(FixedListTest, RemoveRemoveIfUnique)
{
    FixedList<int, 8> l{1, 2, 2, 3, 2, 4};
    EXPECT_EQ(l.remove(2), 3u);
    EXPECT_EQ(seq(l), "134");
    EXPECT_EQ(l.removeIf([](int x)
                         { return x % 2 == 1; }),
              2u);
    EXPECT_EQ(seq(l), "4");

    FixedList<int, 8> d{1, 1, 2, 3, 3, 3, 2};
    EXPECT_EQ(d.unique(), 3u);
    EXPECT_EQ(seq(d), "1232");
}

TEST(FixedListTest, MergeElementWise)
{
    FixedList<int, 8> a{1, 3, 5};
    FixedList<int, 8> b{2, 4, 6};
    a.merge(b);
    EXPECT_EQ(seq(a), "123456");
    EXPECT_TRUE(b.empty());

    // stability
    FixedList<Pair<int, int>, 8> x{{1, 0}, {2, 1}};
    FixedList<Pair<int, int>, 8> y{{1, 2}, {2, 3}};
    x.merge(y, [](Pair<int, int> const& p, Pair<int, int> const& q)
            { return p.first < q.first; });
    std::vector<int> order;
    for (auto const& p : x)
    {
        order.push_back(p.second);
    }
    EXPECT_EQ(order, (std::vector<int>{0, 2, 1, 3}));
}

TEST(FixedListTest, SortStableAndReverse)
{
    FixedList<int, 16> l{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    l.sort();
    EXPECT_EQ(collect(l), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    EXPECT_EQ(l.size(), 10u);

    FixedList<int, 8> rev{3, 2, 1};
    rev.sort([](int a, int b)
             { return a > b; });
    EXPECT_EQ(collect(rev), (std::vector<int>{3, 2, 1}));

    FixedList<Pair<int, int>, 8> p{{2, 0}, {1, 1}, {2, 2}, {1, 3}, {2, 4}};
    p.sort([](Pair<int, int> const& a, Pair<int, int> const& b)
           { return a.first < b.first; });
    std::vector<int> idx;
    for (auto const& e : p)
    {
        idx.push_back(e.second);
    }
    EXPECT_EQ(idx, (std::vector<int>{1, 3, 0, 2, 4}));

    FixedList<int, 8> r{1, 2, 3, 4};
    r.reverse();
    EXPECT_EQ(seq(r), "4321");
}

TEST(FixedListTest, NoLeakAcrossAllPaths)
{
    EXPECT_EQ(Tracked::sAlive, 0);
    {
        FixedList<Tracked, 8> l;
        for (int i = 0; i < 5; ++i)
        {
            l.emplaceBack(i);
        }
        EXPECT_EQ(Tracked::sAlive, 5);
        l.popBack();
        l.popFront();
        EXPECT_EQ(Tracked::sAlive, 3);

        FixedList<Tracked, 8> copy = l;
        EXPECT_EQ(Tracked::sAlive, 6);
        FixedList<Tracked, 8> moved = static_cast<FixedList<Tracked, 8>&&>(copy);
        EXPECT_EQ(Tracked::sAlive, 6); // copy emptied, moved holds 3

        l.clear();
        EXPECT_EQ(Tracked::sAlive, 3);
    }
    EXPECT_EQ(Tracked::sAlive, 0);
}

#ifndef NDEBUG
TEST(FixedListDeathTest, OverflowAndEmptyAccessAbort)
{
    using FL2 = FixedList<int, 2>;
    EXPECT_DEATH(
        {
            FL2 l;
            l.pushBack(1);
            l.pushBack(2);
            l.pushBack(3); // past inline capacity -> hard-cap abort
        },
        "");
    EXPECT_DEATH(
        {
            FL2 l;
            l.popBack();
        },
        "");
    EXPECT_DEATH(
        {
            FL2 l;
            (void)l.front();
        },
        "");
}
#endif
