#include <gtest/gtest.h>

#include <new>
#include <string>
#include <vector>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.forward_list;

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

    struct CountingAllocator
    {
        int* mpAllocs = nullptr;

        CountingAllocator() = default;
        explicit CountingAllocator(int* c) : mpAllocs(c) {}

        void* allocate(usize bytes, usize align) noexcept
        {
            if (mpAllocs != nullptr)
            {
                ++*mpAllocs;
            }
            return ::operator new(bytes, std::align_val_t(align));
        }
        void deallocate(void* p, usize, usize align) noexcept { ::operator delete(p, std::align_val_t(align)); }
        friend bool operator==(CountingAllocator const& a, CountingAllocator const& b) noexcept
        {
            return a.mpAllocs == b.mpAllocs;
        }
    };

    struct EmptyAlloc
    {
        using IsAlwaysEqual = std::true_type;
        void* allocate(usize bytes, usize align) noexcept { return ::operator new(bytes, std::align_val_t(align)); }
        void deallocate(void* p, usize, usize align) noexcept { ::operator delete(p, std::align_val_t(align)); }
        friend bool operator==(EmptyAlloc const&, EmptyAlloc const&) noexcept { return true; }
    };

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

TEST(ForwardListTest, IteratorModelsForwardOnly)
{
    static_assert(ForwardIterator<ForwardList<int>::Iterator>);
    static_assert(ForwardIterator<ForwardList<int>::ConstIterator>);
    static_assert(!BidirectionalIterator<ForwardList<int>::Iterator>);
    static_assert(IsSame<decltype(*ForwardList<int>::ConstIterator{}), int const&>);
    SUCCEED();
}

TEST(ForwardListTest, PushPopFrontAndSize)
{
    ForwardList<int> l;
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.size(), 0u);
    l.pushFront(3); // 3
    l.pushFront(2); // 2 3
    l.pushFront(1); // 1 2 3
    EXPECT_EQ(l.size(), 3u);
    EXPECT_EQ(l.front(), 1);
    EXPECT_EQ(seq(l), "123");
    l.popFront(); // 2 3
    EXPECT_EQ(l.front(), 2);
    EXPECT_EQ(l.size(), 2u);
    EXPECT_EQ(l.emplaceFront(9), 9);
    EXPECT_EQ(seq(l), "923");
}

TEST(ForwardListTest, BeforeBeginBeginEnd)
{
    ForwardList<int> empty;
    EXPECT_EQ(empty.begin(), empty.end());

    ForwardList<int> l{1, 2, 3};
    auto bb = l.beforeBegin();
    ++bb; // beforeBegin + 1 == begin
    EXPECT_EQ(bb, l.begin());
    EXPECT_EQ(*bb, 1);
}

TEST(ForwardListTest, InsertAfterSingleNAndRange)
{
    ForwardList<int> l{1, 4};
    auto r = l.insertAfter(l.begin(), 2); // after 1 -> 1 2 4
    EXPECT_EQ(*r, 2);
    EXPECT_EQ(seq(l), "124");

    auto it = l.begin();
    ++it;                               // at 2
    auto r2 = l.insertAfter(it, 2u, 3); // 1 2 3 3 4 ; returns last inserted
    EXPECT_EQ(*r2, 3);
    EXPECT_EQ(seq(l), "12334");

    // n == 0 returns pos
    auto r3 = l.insertAfter(l.begin(), 0u, 9);
    EXPECT_EQ(r3, l.begin());

    int src[] = {7, 8};
    auto r4   = l.insertAfter(l.begin(), src, src + 2); // 1 7 8 2 3 3 4 ; returns last
    EXPECT_EQ(*r4, 8);
    EXPECT_EQ(seq(l), "1782334");
}

TEST(ForwardListTest, EmplaceAfter)
{
    ForwardList<int> l{1, 3};
    auto r = l.emplaceAfter(l.begin(), 2); // 1 2 3
    EXPECT_EQ(*r, 2);
    EXPECT_EQ(seq(l), "123");
}

TEST(ForwardListTest, EraseAfterSingleAndRange)
{
    ForwardList<int> l{1, 2, 3, 4, 5};
    auto nx = l.eraseAfter(l.begin()); // erase the 2 -> 1 3 4 5
    EXPECT_EQ(*nx, 3);
    EXPECT_EQ(seq(l), "1345");

    auto first = l.begin(); // at 1
    auto last  = first;
    ++last;
    ++last;
    ++last;                             // at 5
    auto e = l.eraseAfter(first, last); // erase (1,5) open -> 3,4 gone -> 1 5
    EXPECT_EQ(*e, 5);
    EXPECT_EQ(seq(l), "15");
    EXPECT_EQ(l.size(), 2u);
}

TEST(ForwardListTest, ResizeGrowShrink)
{
    ForwardList<int> l{1, 2, 3};
    l.resize(5); // 1 2 3 0 0
    EXPECT_EQ(l.size(), 5u);
    EXPECT_EQ(collect(l), (std::vector<int>{1, 2, 3, 0, 0}));
    l.resize(1);
    EXPECT_EQ(seq(l), "1");
    l.resize(3, 9); // 1 9 9
    EXPECT_EQ(seq(l), "199");
    l.resize(0);
    EXPECT_TRUE(l.empty());
}

TEST(ForwardListTest, AssignVariants)
{
    ForwardList<int> l{1, 2, 3};
    l.assign(2u, 5);
    EXPECT_EQ(seq(l), "55");
    int src[] = {7, 8, 9};
    l.assign(src, src + 3);
    EXPECT_EQ(seq(l), "789");
    l.assign({1, 2});
    EXPECT_EQ(seq(l), "12");
}

TEST(ForwardListTest, CopyDeepMoveAndSwap)
{
    ForwardList<int> a{1, 2, 3};
    ForwardList<int> b = a; // copy ctor (deep)
    EXPECT_EQ(seq(b), "123");
    b.front() = 9;
    EXPECT_EQ(a.front(), 1);

    ForwardList<int> c{7};
    c = a; // copy assign
    EXPECT_EQ(seq(c), "123");

    // move ctor steals head, no reseat needed
    ForwardList<int> src{4, 5, 6};
    ForwardList<int> dst = static_cast<ForwardList<int>&&>(src);
    EXPECT_TRUE(src.empty());
    EXPECT_EQ(seq(dst), "456");
    EXPECT_EQ(dst.size(), 3u);

    // move assign
    ForwardList<int> e{1};
    e = static_cast<ForwardList<int>&&>(dst);
    EXPECT_EQ(seq(e), "456");
    EXPECT_TRUE(dst.empty());

    // swap (head pointers only)
    ForwardList<int> x{1, 2};
    ForwardList<int> y{3, 4, 5};
    swap(x, y);
    EXPECT_EQ(seq(x), "345");
    EXPECT_EQ(x.size(), 3u);
    EXPECT_EQ(seq(y), "12");
    EXPECT_EQ(y.size(), 2u);
}

TEST(ForwardListTest, SpliceAfterWholeSingleRange)
{
    // whole (O(other.size))
    {
        ForwardList<int> a{1, 4};
        ForwardList<int> b{2, 3};
        a.spliceAfter(a.begin(), b); // after 1 -> 1 2 3 4
        EXPECT_EQ(seq(a), "1234");
        EXPECT_EQ(a.size(), 4u);
        EXPECT_TRUE(b.empty());
        EXPECT_EQ(b.size(), 0u);
    }
    // single: move element AFTER `it` in b
    {
        ForwardList<int> a{1, 3};
        ForwardList<int> b{9, 2};               // element after begin() is 2
        a.spliceAfter(a.begin(), b, b.begin()); // move 2 -> after 1 -> 1 2 3
        EXPECT_EQ(seq(a), "123");
        EXPECT_EQ(a.size(), 3u);
        EXPECT_EQ(seq(b), "9");
        EXPECT_EQ(b.size(), 1u);
    }
    // range: move (first, last) open range
    {
        ForwardList<int> a{1, 5};
        ForwardList<int> b{0, 2, 3, 4, 8}; // (begin, it@8) -> 2,3,4
        auto last = b.begin();
        ++last;
        ++last;
        ++last;
        ++last; // at 8
        a.spliceAfter(a.begin(), b, b.begin(), last);
        EXPECT_EQ(seq(a), "12345");
        EXPECT_EQ(a.size(), 5u);
        EXPECT_EQ(seq(b), "08");
        EXPECT_EQ(b.size(), 2u);
    }
}

TEST(ForwardListTest, SplicedIteratorStaysValid)
{
    ForwardList<int> a{1, 3};
    ForwardList<int> b{0, 2};
    auto held = b.begin();
    ++held;                                 // points at 2
    a.spliceAfter(a.begin(), b, b.begin()); // move 2 into a
    EXPECT_EQ(*held, 2);
    EXPECT_EQ(seq(a), "123");
}

TEST(ForwardListTest, RemoveRemoveIfUnique)
{
    ForwardList<int> l{1, 2, 2, 3, 2, 4};
    EXPECT_EQ(l.remove(2), 3u);
    EXPECT_EQ(seq(l), "134");
    EXPECT_EQ(l.removeIf([](int x)
                         { return x % 2 == 1; }),
              2u);
    EXPECT_EQ(seq(l), "4");

    ForwardList<int> d{1, 1, 2, 3, 3, 3, 2};
    EXPECT_EQ(d.unique(), 3u);
    EXPECT_EQ(seq(d), "1232");
}

TEST(ForwardListTest, MergeIsStable)
{
    ForwardList<int> a{1, 3, 5};
    ForwardList<int> b{2, 4, 6};
    a.merge(b);
    EXPECT_EQ(seq(a), "123456");
    EXPECT_TRUE(b.empty());

    ForwardList<Pair<int, int>> x{{1, 0}, {2, 1}};
    ForwardList<Pair<int, int>> y{{1, 2}, {2, 3}};
    x.merge(y, [](Pair<int, int> const& p, Pair<int, int> const& q)
            { return p.first < q.first; });
    std::vector<int> order;
    for (auto const& p : x)
    {
        order.push_back(p.second);
    }
    EXPECT_EQ(order, (std::vector<int>{0, 2, 1, 3}));
}

TEST(ForwardListTest, SortCorrectAndStable)
{
    ForwardList<int> l{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    l.sort();
    EXPECT_EQ(collect(l), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    EXPECT_EQ(l.size(), 10u);

    ForwardList<int> rev{3, 2, 1};
    rev.sort([](int a, int b)
             { return a > b; });
    EXPECT_EQ(collect(rev), (std::vector<int>{3, 2, 1}));

    // stability on (key, origIndex)
    ForwardList<Pair<int, int>> p{{2, 0}, {1, 1}, {2, 2}, {1, 3}, {2, 4}};
    p.sort([](Pair<int, int> const& a, Pair<int, int> const& b)
           { return a.first < b.first; });
    std::vector<int> idx;
    for (auto const& e : p)
    {
        idx.push_back(e.second);
    }
    EXPECT_EQ(idx, (std::vector<int>{1, 3, 0, 2, 4}));
}

TEST(ForwardListTest, SortIsAllocationFree)
{
    int allocs = 0;
    ForwardList<int, CountingAllocator> l{CountingAllocator{&allocs}};
    for (int i = 0; i < 10; ++i)
    {
        l.pushFront(i); // 9 8 ... 0
    }
    int const before = allocs;
    EXPECT_EQ(before, 10);
    l.sort();
    EXPECT_EQ(allocs, before);
    EXPECT_EQ(collect(l), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST(ForwardListTest, ReverseMethod)
{
    ForwardList<int> l{1, 2, 3, 4};
    l.reverse();
    EXPECT_EQ(seq(l), "4321");
    EXPECT_EQ(l.front(), 4);

    ForwardList<int> single{5};
    single.reverse();
    EXPECT_EQ(seq(single), "5");

    ForwardList<int> empty;
    empty.reverse();
    EXPECT_TRUE(empty.empty());
}

TEST(ForwardListTest, NoLeakAcrossAllPaths)
{
    EXPECT_EQ(Tracked::sAlive, 0);
    {
        ForwardList<Tracked> l;
        for (int i = 0; i < 5; ++i)
        {
            l.pushFront(Tracked{i});
        }
        EXPECT_EQ(Tracked::sAlive, 5);
        l.popFront();
        l.eraseAfter(l.begin());
        EXPECT_EQ(Tracked::sAlive, 3);

        ForwardList<Tracked> copy = l;
        EXPECT_EQ(Tracked::sAlive, 6);
        ForwardList<Tracked> moved = static_cast<ForwardList<Tracked>&&>(copy);
        EXPECT_EQ(Tracked::sAlive, 6);

        l.clear();
        EXPECT_EQ(Tracked::sAlive, 3);
    }
    EXPECT_EQ(Tracked::sAlive, 0);
}

TEST(ForwardListTest, EmptyAllocatorAddsNoBytes)
{
    // head pointer + size counter, allocator folded away.
    static_assert(sizeof(ForwardList<int, EmptyAlloc>) == sizeof(void*) + sizeof(usize));
    SUCCEED();
}

#ifndef NDEBUG
TEST(ForwardListDeathTest, PopAccessAndEraseOnEmptyAbort)
{
    using FL = ForwardList<int>;
    EXPECT_DEATH(
        {
            FL l;
            l.popFront();
        },
        "");
    EXPECT_DEATH(
        {
            FL l;
            (void)l.front();
        },
        "");
    EXPECT_DEATH(
        {
            FL l;
            l.eraseAfter(l.beforeBegin()); // nothing after beforeBegin
        },
        "");
}
#endif
