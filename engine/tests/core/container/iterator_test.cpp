#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.container.iterator;

using namespace worse;
using namespace worse::core;

namespace
{
    // A deliberately bidirectional-only iterator (advertises BidirectionalIteratorTag)
    // so we exercise the non-random-access path of distance/advance and the concept
    // hierarchy's upper bound.
    struct BidiIter
    {
        int* p = nullptr;

        using IteratorCategory = BidirectionalIteratorTag;
        using ValueType        = int;
        using DifferenceType   = isize;
        using Pointer          = int*;
        using Reference        = int&;

        Reference operator*() const { return *p; }
        BidiIter& operator++()
        {
            ++p;
            return *this;
        }
        BidiIter& operator--()
        {
            --p;
            return *this;
        }
        bool operator==(BidiIter const& o) const { return p == o.p; }
        bool operator!=(BidiIter const& o) const { return p != o.p; }
    };
} // namespace

TEST(IteratorTest, RawPointerTraits)
{
    using Traits = IteratorTraits<int*>;
    static_assert(IsSame<Traits::IteratorCategory, ContiguousIteratorTag>);
    static_assert(IsSame<Traits::ValueType, int>);
    static_assert(IsSame<Traits::DifferenceType, isize>);
    static_assert(IsSame<Traits::Pointer, int*>);
    static_assert(IsSame<Traits::Reference, int&>);
    // cv stripped from the value type.
    static_assert(IsSame<IteratorTraits<int const*>::ValueType, int>);
    SUCCEED();
}

TEST(IteratorTest, ConceptHierarchy)
{
    // A raw pointer satisfies every level up to contiguous.
    static_assert(InputIterator<int*>);
    static_assert(ForwardIterator<int*>);
    static_assert(BidirectionalIterator<int*>);
    static_assert(RandomAccessIterator<int*>);
    static_assert(ContiguousIterator<int*>);

    // The bidirectional mock stops at bidirectional.
    static_assert(InputIterator<BidiIter>);
    static_assert(BidirectionalIterator<BidiIter>);
    static_assert(!RandomAccessIterator<BidiIter>);
    static_assert(!ContiguousIterator<BidiIter>);

    // A non-iterator fails the concept (rather than hard-erroring).
    static_assert(!InputIterator<int>);
    SUCCEED();
}

TEST(IteratorTest, DistanceRandomAccessAndWalk)
{
    int a[5] = {10, 20, 30, 40, 50};
    // random-access path
    EXPECT_EQ(distance(a, a + 5), 5);
    EXPECT_EQ(distance(a + 1, a + 4), 3);
    // O(n) walk path via the bidirectional mock
    EXPECT_EQ(distance(BidiIter{a}, BidiIter{a + 5}), 5);
}

TEST(IteratorTest, AdvanceNextPrev)
{
    int a[5] = {10, 20, 30, 40, 50};

    int* it = a;
    advance(it, 3);
    EXPECT_EQ(*it, 40);
    advance(it, -2);
    EXPECT_EQ(*it, 20);

    EXPECT_EQ(*next(a), 20);
    EXPECT_EQ(*next(a, 4), 50);
    EXPECT_EQ(*prev(a + 4), 40);
    EXPECT_EQ(*prev(a + 4, 4), 10);

    // bidirectional walk path
    BidiIter b{a};
    advance(b, 2);
    EXPECT_EQ(*b, 30);
    advance(b, -1);
    EXPECT_EQ(*b, 20);
}

TEST(IteratorTest, ReverseIteratorOverArray)
{
    int a[4] = {1, 2, 3, 4};
    ReverseIterator<int*> rbegin{a + 4};
    ReverseIterator<int*> rend{a};

    // dereference reads the element before base() -> last element first.
    EXPECT_EQ(*rbegin, 4);
    EXPECT_EQ(rbegin.base(), a + 4);

    // forward iteration walks the array in reverse.
    int expected[4] = {4, 3, 2, 1};
    int i           = 0;
    for (auto it = rbegin; it != rend; ++it, ++i)
    {
        EXPECT_EQ(*it, expected[i]);
    }
    EXPECT_EQ(i, 4);
}

TEST(IteratorTest, ReverseIteratorRandomAccess)
{
    int a[5] = {1, 2, 3, 4, 5};
    ReverseIterator<int*> rbegin{a + 5};

    EXPECT_EQ(rbegin[0], 5);
    EXPECT_EQ(rbegin[2], 3);
    EXPECT_EQ(*(rbegin + 2), 3);
    EXPECT_EQ(*(2 + rbegin), 3); // n + it
    EXPECT_EQ(*(rbegin - -1), 4);

    auto a2 = makeReverseIterator(a + 5);
    auto b2 = makeReverseIterator(a + 1);
    // distance between reverse iterators (operands swap internally).
    EXPECT_EQ(b2 - a2, 4);
    EXPECT_TRUE(a2 < b2);
    EXPECT_TRUE(b2 > a2);
    EXPECT_TRUE(a2 != b2);

    // distance()/IteratorTraits also work on reverse iterators.
    EXPECT_EQ(distance(a2, b2), 4);
    static_assert(RandomAccessIterator<ReverseIterator<int*>>);
}

TEST(IteratorTest, ReverseIteratorConstexpr)
{
    constexpr auto check = []
    {
        int a[3]                = {7, 8, 9};
        ReverseIterator<int*> r = ReverseIterator<int*>(a + 3);
        return *r; // 9
    };
    static_assert(check() == 9);
    SUCCEED();
}
