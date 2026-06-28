#include <gtest/gtest.h>

#include <new>
#include <string>
#include <vector>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.utility;
import worse.core.container.iterator;
import worse.core.container.list;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    // Leak net: every constructed element bumps sAlive, every destroyed one drops it. A test
    // that builds + tears down a List<Tracked> must end with sAlive == 0, proving the node
    // value-destroy + storage-free are correctly paired on every path.
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

    // Stateful allocator that counts allocate() calls (shared via an external counter), used
    // to prove sort/merge/reverse are allocation-free. Byte-based contract only.
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
        void deallocate(void* p, usize, usize align) noexcept
        {
            ::operator delete(p, std::align_val_t(align));
        }
        [[maybe_unused]] friend bool operator==(CountingAllocator const& a, CountingAllocator const& b) noexcept
        {
            return a.mpAllocs == b.mpAllocs;
        }
    };

    // Stateless always-equal allocator, for the EBO/layout check.
    struct EmptyAlloc
    {
        using IsAlwaysEqual = std::true_type;
        void* allocate(usize bytes, usize align) noexcept { return ::operator new(bytes, std::align_val_t(align)); }
        void deallocate(void* p, usize, usize align) noexcept { ::operator delete(p, std::align_val_t(align)); }
        [[maybe_unused]] friend bool operator==(EmptyAlloc const&, EmptyAlloc const&) noexcept { return true; }
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

TEST(ListTest, IteratorModelsBidirectional)
{
    static_assert(BidirectionalIterator<List<int>::Iterator>);
    static_assert(BidirectionalIterator<List<int>::ConstIterator>);
    static_assert(!RandomAccessIterator<List<int>::Iterator>);
    static_assert(IsSame<decltype(*List<int>::ConstIterator{}), int const&>);
    SUCCEED();
}

TEST(ListTest, PushPopFrontBackAndSize)
{
    List<int> l;
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.size(), 0u);
    l.pushBack(2);  // 2
    l.pushBack(3);  // 2 3
    l.pushFront(1); // 1 2 3
    EXPECT_EQ(l.size(), 3u);
    EXPECT_EQ(l.front(), 1);
    EXPECT_EQ(l.back(), 3);
    EXPECT_EQ(seq(l), "123");
    l.popFront(); // 2 3
    EXPECT_EQ(l.front(), 2);
    l.popBack(); // 2
    EXPECT_EQ(l.back(), 2);
    EXPECT_EQ(l.size(), 1u);
}

TEST(ListTest, EmplaceReturnsReferenceAndValue)
{
    List<int> l;
    int& r = l.emplaceBack(42);
    EXPECT_EQ(r, 42);
    r = 7;
    EXPECT_EQ(l.back(), 7);
    EXPECT_EQ(l.emplaceFront(1), 1);
    EXPECT_EQ(seq(l), "17");
}

TEST(ListTest, InsertSingleNAndRangeReturns)
{
    List<int> l{1, 4};
    auto it = l.begin();
    ++it;                     // at 4
    auto r = l.insert(it, 2); // 1 2 4
    EXPECT_EQ(*r, 2);
    EXPECT_EQ(seq(l), "124");

    // insert n copies; returns first inserted
    auto it2 = l.begin();
    ++it2;
    ++it2;                          // at 4
    auto r2 = l.insert(it2, 2u, 3); // 1 2 3 3 4
    EXPECT_EQ(*r2, 3);
    EXPECT_EQ(seq(l), "12334");

    // n == 0 returns pos
    auto pos = l.begin();
    auto r3  = l.insert(pos, 0u, 9);
    EXPECT_EQ(r3, l.begin());

    // range insert
    int src[] = {7, 8};
    auto r4   = l.insert(l.end(), src, src + 2); // ...7 8
    EXPECT_EQ(*r4, 7);
    EXPECT_EQ(seq(l), "1233478");
}

TEST(ListTest, EraseSingleAndRange)
{
    List<int> l{1, 2, 3, 4, 5};
    auto it = l.begin();
    ++it;                  // at 2
    auto nx = l.erase(it); // remove 2 -> 1 3 4 5
    EXPECT_EQ(*nx, 3);
    EXPECT_EQ(seq(l), "1345");

    auto first = l.begin();
    ++first; // at 3
    auto last = first;
    ++last;
    ++last;                        // at 5
    auto e = l.erase(first, last); // remove 3,4 -> 1 5
    EXPECT_EQ(*e, 5);
    EXPECT_EQ(seq(l), "15");
    EXPECT_EQ(l.size(), 2u);
}

TEST(ListTest, ResizeGrowShrink)
{
    List<int> l{1, 2, 3};
    l.resize(5); // value-init two more (0 0)
    EXPECT_EQ(l.size(), 5u);
    EXPECT_EQ(l.back(), 0);
    l.resize(1);
    EXPECT_EQ(seq(l), "1");
    l.resize(3, 9); // 1 9 9
    EXPECT_EQ(seq(l), "199");
}

TEST(ListTest, AssignVariants)
{
    List<int> l{1, 2, 3};
    l.assign(2u, 5);
    EXPECT_EQ(seq(l), "55");
    int src[] = {7, 8, 9};
    l.assign(src, src + 3);
    EXPECT_EQ(seq(l), "789");
    l.assign({1, 2});
    EXPECT_EQ(seq(l), "12");
}

TEST(ListTest, CopyConstructAndAssignAreDeep)
{
    List<int> a{1, 2, 3};
    List<int> b = a; // copy ctor
    EXPECT_EQ(seq(b), "123");
    // distinct nodes: mutating b leaves a untouched
    b.front() = 9;
    EXPECT_EQ(a.front(), 1);
    EXPECT_EQ(b.front(), 9);

    List<int> c{7};
    c = a; // copy assign drops 7, deep-copies a
    EXPECT_EQ(seq(c), "123");
    c.back() = 8;
    EXPECT_EQ(a.back(), 3);
}

TEST(ListTest, MoveConstructStealsAndReseats)
{
    List<int> src{1, 2, 3};
    List<int> dst = static_cast<List<int>&&>(src);
    EXPECT_TRUE(src.empty());
    EXPECT_EQ(src.size(), 0u);
    EXPECT_EQ(dst.size(), 3u);
    EXPECT_EQ(seq(dst), "123");
    EXPECT_EQ(dst.front(), 1);
    EXPECT_EQ(dst.back(), 3);
}

TEST(ListTest, MoveAssignDropsOldAdoptsNew)
{
    List<int> dst{7, 8};
    List<int> src{1, 2, 3};
    dst = static_cast<List<int>&&>(src);
    EXPECT_EQ(seq(dst), "123");
    EXPECT_TRUE(src.empty());
}

TEST(ListTest, MoveReseatBoundaryIntegrity)
{
    // After a move, the destination's boundary nodes must route through ITS anchor; mutate at
    // both ends and walk forward + backward to prove the ring is intact.
    List<int> src{2, 3, 4};
    List<int> dst = static_cast<List<int>&&>(src);
    dst.pushFront(1); // 1 2 3 4
    dst.pushBack(5);  // 1 2 3 4 5
    EXPECT_EQ(seq(dst), "12345");
    std::string rev;
    for (auto it = dst.rbegin(); it != dst.rend(); ++it)
    {
        rev += static_cast<char>('0' + *it);
    }
    EXPECT_EQ(rev, "54321");
    EXPECT_EQ(dst.front(), 1);
    EXPECT_EQ(dst.back(), 5);
}

TEST(ListTest, SwapBothNonEmptyWithEmptyAndTwoEmpty)
{
    List<int> x{1, 2};
    List<int> y{3, 4, 5};
    swap(x, y);
    EXPECT_EQ(seq(x), "345");
    EXPECT_EQ(seq(y), "12");
    EXPECT_EQ(x.front(), 3);
    EXPECT_EQ(x.back(), 5);

    List<int> full{1, 2};
    List<int> empty;
    swap(full, empty);
    EXPECT_TRUE(full.empty());
    EXPECT_EQ(seq(empty), "12");
    EXPECT_EQ(empty.back(), 2);

    List<int> e1, e2;
    swap(e1, e2);
    EXPECT_TRUE(e1.empty());
    EXPECT_TRUE(e2.empty());
}

TEST(ListTest, ForwardAndReverseIteration)
{
    List<int> l{1, 2, 3, 4};
    EXPECT_EQ(seq(l), "1234");
    std::string rev;
    for (auto it = l.rbegin(); it != l.rend(); ++it)
    {
        rev += static_cast<char>('0' + *it);
    }
    EXPECT_EQ(rev, "4321");
}

TEST(ListTest, SpliceWholeSingleRangeWithSizeAccounting)
{
    // whole
    {
        List<int> a{1, 4};
        List<int> b{2, 3};
        auto pos = a.begin();
        ++pos; // at 4
        a.splice(pos, b);
        EXPECT_EQ(seq(a), "1234");
        EXPECT_EQ(a.size(), 4u);
        EXPECT_TRUE(b.empty());
        EXPECT_EQ(b.size(), 0u);
    }
    // single
    {
        List<int> a{1, 3};
        List<int> b{9, 2, 9};
        auto bit = b.begin();
        ++bit; // at 2
        auto pos = a.begin();
        ++pos;                 // at 3
        a.splice(pos, b, bit); // move 2 from b into a before 3
        EXPECT_EQ(seq(a), "123");
        EXPECT_EQ(a.size(), 3u);
        EXPECT_EQ(b.size(), 2u);
        EXPECT_EQ(seq(b), "99");
    }
    // range
    {
        List<int> a{1, 5};
        List<int> b{2, 3, 4, 8};
        auto first = b.begin(); // at 2
        auto last  = first;
        ++last;
        ++last;
        ++last; // at 8 (exclusive)
        auto pos = a.begin();
        ++pos;                         // at 5
        a.splice(pos, b, first, last); // move 2,3,4
        EXPECT_EQ(seq(a), "12345");
        EXPECT_EQ(a.size(), 5u);
        EXPECT_EQ(seq(b), "8");
        EXPECT_EQ(b.size(), 1u);
    }
}

TEST(ListTest, SplicedIteratorStaysValid)
{
    List<int> a{1, 3};
    List<int> b{2};
    auto bit = b.begin(); // points at 2
    auto pos = a.begin();
    ++pos; // at 3
    a.splice(pos, b, bit);
    // The iterator obtained from b still dereferences to the same element, now in a.
    EXPECT_EQ(*bit, 2);
    EXPECT_EQ(seq(a), "123");
}

TEST(ListTest, RemoveAndRemoveIfReturnCount)
{
    List<int> l{1, 2, 2, 3, 2, 4};
    EXPECT_EQ(l.remove(2), 3u);
    EXPECT_EQ(seq(l), "134");
    EXPECT_EQ(l.removeIf([](int x)
                         { return x % 2 == 1; }),
              2u); // remove 1,3
    EXPECT_EQ(seq(l), "4");
}

TEST(ListTest, UniqueCollapsesConsecutive)
{
    List<int> l{1, 1, 2, 3, 3, 3, 2};
    EXPECT_EQ(l.unique(), 3u);
    EXPECT_EQ(seq(l), "1232");

    // The retained anchor does NOT advance on a removal: after dropping 6 (pred(5,6)), the
    // next compare is pred(5,7) which is false, so 7 stays. -> {5,7}, one removed.
    List<int> m{5, 6, 7};
    EXPECT_EQ(m.unique([](int a, int b)
                       { return b - a == 1; }),
              1u);
    EXPECT_EQ(seq(m), "57");
}

TEST(ListTest, MergeIsStable)
{
    List<int> a{1, 3, 5};
    List<int> b{2, 4, 6};
    a.merge(b);
    EXPECT_EQ(seq(a), "123456");
    EXPECT_TRUE(b.empty());

    // stability: equal keys keep this-before-other order.
    List<Pair<int, int>> x{{1, 0}, {2, 1}};
    List<Pair<int, int>> y{{1, 2}, {2, 3}};
    auto byFirst = [](Pair<int, int> const& p, Pair<int, int> const& q)
    {
        return p.first < q.first;
    };
    x.merge(y, byFirst);
    std::vector<int> order;
    for (auto const& p : x)
    {
        order.push_back(p.second);
    }
    EXPECT_EQ(order, (std::vector<int>{0, 2, 1, 3})); // key1: 0 then 2; key2: 1 then 3
}

TEST(ListTest, SortCorrectAndStable)
{
    List<int> l{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
    l.sort();
    EXPECT_EQ(collect(l), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    EXPECT_EQ(l.size(), 10u);

    List<int> already{1, 2, 3};
    already.sort();
    EXPECT_EQ(collect(already), (std::vector<int>{1, 2, 3}));

    List<int> rev{3, 2, 1};
    rev.sort([](int a, int b)
             { return a > b; }); // descending
    EXPECT_EQ(collect(rev), (std::vector<int>{3, 2, 1}));

    // stability on (key, origIndex): equal keys must keep ascending origIndex.
    List<Pair<int, int>> p{{2, 0}, {1, 1}, {2, 2}, {1, 3}, {2, 4}};
    p.sort([](Pair<int, int> const& a, Pair<int, int> const& b)
           { return a.first < b.first; });
    std::vector<int> idx;
    for (auto const& e : p)
    {
        idx.push_back(e.second);
    }
    EXPECT_EQ(idx, (std::vector<int>{1, 3, 0, 2, 4}));
}

TEST(ListTest, SortIsAllocationFree)
{
    int allocs = 0;
    List<int, CountingAllocator> l{CountingAllocator{&allocs}};
    for (int i = 10; i > 0; --i)
    {
        l.pushBack(i);
    }
    int const before = allocs;
    EXPECT_EQ(before, 10);
    l.sort();
    EXPECT_EQ(allocs, before); // sort relinks only -- no node allocated
    EXPECT_EQ(collect(l), (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
}

TEST(ListTest, ReverseMethod)
{
    List<int> l{1, 2, 3, 4};
    l.reverse();
    EXPECT_EQ(seq(l), "4321");
    EXPECT_EQ(l.front(), 4);
    EXPECT_EQ(l.back(), 1);

    List<int> single{5};
    single.reverse();
    EXPECT_EQ(seq(single), "5");

    List<int> empty;
    empty.reverse();
    EXPECT_TRUE(empty.empty());
}

TEST(ListTest, NoLeakAcrossAllPaths)
{
    EXPECT_EQ(Tracked::sAlive, 0);
    {
        List<Tracked> l;
        for (int i = 0; i < 5; ++i)
        {
            l.emplaceBack(i);
        }
        EXPECT_EQ(Tracked::sAlive, 5);
        l.popBack();
        l.popFront();
        EXPECT_EQ(Tracked::sAlive, 3);
        auto it = l.begin();
        ++it;
        l.erase(it);
        EXPECT_EQ(Tracked::sAlive, 2);

        List<Tracked> copy = l; // deep copy
        EXPECT_EQ(Tracked::sAlive, 4);
        List<Tracked> moved = static_cast<List<Tracked>&&>(copy);
        EXPECT_EQ(Tracked::sAlive, 4); // move transfers, no new elements

        l.clear();
        EXPECT_EQ(Tracked::sAlive, 2);
    }
    EXPECT_EQ(Tracked::sAlive, 0); // all destructors ran
}

TEST(ListTest, EmptyAllocatorAddsNoBytes)
{
    // anchor (2 ptrs) + size counter, with the empty allocator folded away by EBO.
    static_assert(sizeof(List<int, EmptyAlloc>) == 2 * sizeof(void*) + sizeof(usize));
    SUCCEED();
}

#ifndef NDEBUG
TEST(ListDeathTest, PopAndAccessOnEmptyAbort)
{
    using LI = List<int>;
    EXPECT_DEATH(
        {
            LI l;
            l.popBack();
        },
        "");
    EXPECT_DEATH(
        {
            LI l;
            l.popFront();
        },
        "");
    EXPECT_DEATH(
        {
            LI l;
            (void)l.front();
        },
        "");
    EXPECT_DEATH(
        {
            LI l;
            (void)l.back();
        },
        "");
}
#endif
