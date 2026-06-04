#include <gtest/gtest.h>

#include <string>
#include <vector>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.forward_list;
import worse.core.container.fixed_slist;

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

TEST(FixedSListTest, IteratorModelsForwardAndInteropsWithForwardList)
{
    static_assert(ForwardIterator<FixedSList<int, 8>::Iterator>);
    static_assert(!BidirectionalIterator<FixedSList<int, 8>::Iterator>);
    static_assert(IsSame<FixedSList<int, 8>::Iterator, ForwardList<int>::Iterator>);
    static_assert(IsSame<decltype(*FixedSList<int, 8>::ConstIterator{}), int const&>);
    SUCCEED();
}

TEST(FixedSListTest, PushPopFrontSizeFullCapacity)
{
    FixedSList<int, 4> l;
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.capacity(), 4u);
    l.pushFront(3);
    l.pushFront(2);
    l.pushFront(1);
    EXPECT_EQ(seq(l), "123");
    EXPECT_EQ(l.front(), 1);
    l.pushFront(0);
    EXPECT_TRUE(l.full());
    EXPECT_EQ(l.size(), 4u);
    l.popFront();
    EXPECT_EQ(seq(l), "123");
    EXPECT_FALSE(l.full());
}

TEST(FixedSListTest, FreeListRecyclesSlots)
{
    FixedSList<int, 3> l;
    l.pushFront(1);
    l.pushFront(2);
    l.pushFront(3);
    EXPECT_TRUE(l.full());
    for (int round = 0; round < 100; ++round)
    {
        l.popFront();
        l.pushFront(round % 9);
        EXPECT_EQ(l.size(), 3u);
    }
    EXPECT_EQ(l.size(), 3u);

    for (int round = 0; round < 50; ++round)
    {
        FixedSList<int, 3> f{1, 2, 3};
        EXPECT_TRUE(f.full());
        f.clear();
        EXPECT_TRUE(f.empty());
        f.pushFront(9);
        EXPECT_EQ(f.front(), 9);
    }
}

TEST(FixedSListTest, BeforeBeginInsertAfterEraseAfter)
{
    FixedSList<int, 8> l{1, 4};
    auto r = l.insertAfter(l.begin(), 2); // 1 2 4
    EXPECT_EQ(*r, 2);
    auto it = l.begin();
    ++it;                               // at 2
    auto r2 = l.insertAfter(it, 2u, 3); // 1 2 3 3 4
    EXPECT_EQ(*r2, 3);
    EXPECT_EQ(seq(l), "12334");

    auto nx = l.eraseAfter(l.begin()); // erase 2 -> 1 3 3 4
    EXPECT_EQ(*nx, 3);
    EXPECT_EQ(seq(l), "1334");

    // beforeBegin handle
    auto bb = l.beforeBegin();
    ++bb;
    EXPECT_EQ(bb, l.begin());
}

TEST(FixedSListTest, ResizeGrowShrink)
{
    FixedSList<int, 8> l{1, 2, 3};
    l.resize(5);
    EXPECT_EQ(collect(l), (std::vector<int>{1, 2, 3, 0, 0}));
    l.resize(1);
    EXPECT_EQ(seq(l), "1");
    l.resize(3, 9);
    EXPECT_EQ(seq(l), "199");
}

TEST(FixedSListTest, CopyMoveAreElementWiseAndIndependent)
{
    FixedSList<int, 8> a{1, 2, 3};
    FixedSList<int, 8> b = a;
    EXPECT_EQ(seq(b), "123");
    b.front() = 9;
    EXPECT_EQ(a.front(), 1);

    FixedSList<int, 8> c{4, 5, 6};
    FixedSList<int, 8> d = static_cast<FixedSList<int, 8>&&>(c);
    EXPECT_EQ(seq(d), "456");
    EXPECT_TRUE(c.empty());

    a = static_cast<FixedSList<int, 8>&&>(d);
    EXPECT_EQ(seq(a), "456");
    EXPECT_TRUE(d.empty());
}

TEST(FixedSListTest, Swap)
{
    FixedSList<int, 8> x{1, 2};
    FixedSList<int, 8> y{3, 4, 5};
    swap(x, y);
    EXPECT_EQ(seq(x), "345");
    EXPECT_EQ(x.size(), 3u);
    EXPECT_EQ(seq(y), "12");
    x.pushFront(0);
    EXPECT_EQ(seq(x), "0345");
}

TEST(FixedSListTest, RemoveRemoveIfUnique)
{
    FixedSList<int, 8> l{1, 2, 2, 3, 2, 4};
    EXPECT_EQ(l.remove(2), 3u);
    EXPECT_EQ(seq(l), "134");
    EXPECT_EQ(l.removeIf([](int x)
                         { return x % 2 == 1; }),
              2u);
    EXPECT_EQ(seq(l), "4");

    FixedSList<int, 8> d{1, 1, 2, 3, 3, 3, 2};
    EXPECT_EQ(d.unique(), 3u);
    EXPECT_EQ(seq(d), "1232");
}

TEST(FixedSListTest, MergeAndSortAndReverse)
{
    FixedSList<int, 8> a{1, 3, 5};
    FixedSList<int, 8> b{2, 4, 6};
    a.merge(b);
    EXPECT_EQ(seq(a), "123456");
    EXPECT_TRUE(b.empty());

    FixedSList<int, 16> l{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    l.sort();
    EXPECT_EQ(collect(l), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));

    // stable sort on (key, origIndex)
    FixedSList<Pair<int, int>, 8> p{{2, 0}, {1, 1}, {2, 2}, {1, 3}, {2, 4}};
    p.sort([](Pair<int, int> const& x, Pair<int, int> const& y)
           { return x.first < y.first; });
    std::vector<int> idx;
    for (auto const& e : p)
    {
        idx.push_back(e.second);
    }
    EXPECT_EQ(idx, (std::vector<int>{1, 3, 0, 2, 4}));

    FixedSList<int, 8> r{1, 2, 3, 4};
    r.reverse();
    EXPECT_EQ(seq(r), "4321");
}

TEST(FixedSListTest, NoLeakAcrossAllPaths)
{
    EXPECT_EQ(Tracked::sAlive, 0);
    {
        FixedSList<Tracked, 8> l;
        for (int i = 0; i < 5; ++i)
        {
            l.pushFront(Tracked{i});
        }
        EXPECT_EQ(Tracked::sAlive, 5);
        l.popFront();
        l.eraseAfter(l.begin());
        EXPECT_EQ(Tracked::sAlive, 3);

        FixedSList<Tracked, 8> copy = l;
        EXPECT_EQ(Tracked::sAlive, 6);
        FixedSList<Tracked, 8> moved = static_cast<FixedSList<Tracked, 8>&&>(copy);
        EXPECT_EQ(Tracked::sAlive, 6);

        l.clear();
        EXPECT_EQ(Tracked::sAlive, 3);
    }
    EXPECT_EQ(Tracked::sAlive, 0);
}

TEST(FixedSListTest, TryEmplaceAndRemaining)
{
    FixedSList<int, 3> l;
    EXPECT_EQ(l.remaining(), 3u);
    int* a = l.tryEmplaceFront(2);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(*a, 2);
    EXPECT_NE(l.tryEmplaceFront(1), nullptr); // {1,2}
    EXPECT_EQ(l.remaining(), 1u);
    EXPECT_NE(l.tryEmplaceAfter(l.begin(), 9), nullptr); // {1,9,2}
    EXPECT_TRUE(l.full());
    EXPECT_EQ(l.remaining(), 0u);
    // Non-aborting at capacity (R46): nullptr, container unchanged.
    EXPECT_EQ(l.tryEmplaceFront(0), nullptr);
    EXPECT_EQ(l.tryEmplaceAfter(l.begin(), 7), nullptr);
    EXPECT_EQ(l.size(), 3u);
    EXPECT_EQ(l.front(), 1);
}

TEST(FixedSListDeathTest, OverflowHardCapAborts)
{
    // Hard-cap overflow aborts in EVERY build (WE_VERIFY, R46) -- not NDEBUG-guarded.
    using FS2 = FixedSList<int, 2>;
    EXPECT_DEATH(
        {
            FS2 l;
            l.pushFront(1);
            l.pushFront(2);
            l.pushFront(3); // past inline capacity -> WE_VERIFY abort (always-on)
        },
        "");
}

#ifndef NDEBUG
TEST(FixedSListDeathTest, EmptyAccessAborts)
{
    // Empty-access preconditions stay debug-only WE_ASSERT (R4/R46).
    using FS2 = FixedSList<int, 2>;
    EXPECT_DEATH(
        {
            FS2 l;
            l.popFront();
        },
        "");
    EXPECT_DEATH(
        {
            FS2 l;
            (void)l.front();
        },
        "");
}
#endif
