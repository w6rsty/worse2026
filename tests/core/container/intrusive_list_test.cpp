#include <gtest/gtest.h>

#include <string>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.container.iterator;
import worse.core.container.intrusive_list;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    // The list never allocates: the link pointers are embedded in Widget. Test code owns
    // the Widget storage (here, automatic/stack) and keeps it alive across membership.
    struct Widget : IntrusiveListNode
    {
        int v    = 0;
        Widget() = default;
        explicit Widget(int x) : v(x) {}
    };

    using List = IntrusiveList<Widget>;

    template <typename L>
    std::string seq(L const& list)
    {
        std::string s;
        for (auto const& w : list)
        {
            s += static_cast<char>('0' + w.v);
        }
        return s;
    }
} // namespace

TEST(IntrusiveListTest, IteratorModelsBidirectional)
{
    static_assert(BidirectionalIterator<List::Iterator>);
    static_assert(BidirectionalIterator<List::ConstIterator>);
    static_assert(IsSame<decltype(*List::ConstIterator{}), Widget const&>);
    SUCCEED();
}

TEST(IntrusiveListTest, PushBackFrontAndAccess)
{
    Widget a{1}, b{2}, c{3};
    List l;
    EXPECT_TRUE(l.empty());
    l.pushBack(b);  // 2
    l.pushBack(c);  // 2 3
    l.pushFront(a); // 1 2 3
    EXPECT_EQ(l.size(), 3u);
    EXPECT_EQ(l.front().v, 1);
    EXPECT_EQ(l.back().v, 3);
    EXPECT_EQ(seq(l), "123");
}

TEST(IntrusiveListTest, PopFrontBack)
{
    Widget a{1}, b{2}, c{3};
    List l;
    l.pushBack(a);
    l.pushBack(b);
    l.pushBack(c);
    l.popFront(); // 2 3
    EXPECT_EQ(l.front().v, 2);
    l.popBack(); // 2
    EXPECT_EQ(l.back().v, 2);
    EXPECT_EQ(l.size(), 1u);
}

TEST(IntrusiveListTest, ForwardAndReverseIteration)
{
    Widget a{1}, b{2}, c{3}, d{4};
    List l;
    l.pushBack(a);
    l.pushBack(b);
    l.pushBack(c);
    l.pushBack(d);

    EXPECT_EQ(seq(l), "1234");

    std::string rev;
    for (auto it = l.rbegin(); it != l.rend(); ++it)
    {
        rev += static_cast<char>('0' + it->v);
    }
    EXPECT_EQ(rev, "4321");
}

TEST(IntrusiveListTest, InsertBeforeIterator)
{
    Widget a{1}, c{3}, b{2};
    List l;
    l.pushBack(a);
    l.pushBack(c); // 1 3
    auto pos = l.begin();
    ++pos;                      // points at 3
    auto it = l.insert(pos, b); // 1 2 3
    EXPECT_EQ(it->v, 2);
    EXPECT_EQ(seq(l), "123");
    EXPECT_EQ(l.size(), 3u);
}

TEST(IntrusiveListTest, EraseReturnsNextAndElementSurvives)
{
    Widget a{1}, b{2}, c{3};
    List l;
    l.pushBack(a);
    l.pushBack(b);
    l.pushBack(c);

    auto it = l.begin();
    ++it;                    // at 2
    auto next = l.erase(it); // removes 2, returns iterator to 3
    EXPECT_EQ(next->v, 3);
    EXPECT_EQ(seq(l), "13");
    EXPECT_EQ(l.size(), 2u);
    // The list does not destroy elements: b is still a valid object.
    EXPECT_EQ(b.v, 2);
    // Its links were nulled on unlink.
    EXPECT_EQ(static_cast<IntrusiveListNode&>(b).mpNext, nullptr);
}

TEST(IntrusiveListTest, RemoveAndIteratorTo)
{
    Widget a{1}, b{2}, c{3};
    List l;
    l.pushBack(a);
    l.pushBack(b);
    l.pushBack(c);

    l.remove(b); // O(1) unlink of a known member
    EXPECT_EQ(seq(l), "13");

    auto it = List::iteratorTo(c);
    EXPECT_EQ(it->v, 3);
    l.erase(it);
    EXPECT_EQ(seq(l), "1");
    EXPECT_EQ(l.size(), 1u);
}

TEST(IntrusiveListTest, ConstIteration)
{
    Widget a{4}, b{5}, c{6};
    List l;
    l.pushBack(a);
    l.pushBack(b);
    l.pushBack(c);

    List const& cl = l;
    int sum        = 0;
    for (Widget const& w : cl)
    {
        sum += w.v;
    }
    EXPECT_EQ(sum, 15);
    EXPECT_EQ(cl.front().v, 4);
    EXPECT_EQ(cl.back().v, 6);
}

TEST(IntrusiveListTest, MoveTransfersChainAndReseats)
{
    Widget a{1}, b{2}, c{3};
    List src;
    src.pushBack(a);
    src.pushBack(b);
    src.pushBack(c);

    List dst = static_cast<List&&>(src);
    EXPECT_TRUE(src.empty());
    EXPECT_EQ(src.size(), 0u);
    EXPECT_EQ(dst.size(), 3u);
    EXPECT_EQ(seq(dst), "123");
    // Re-seated: boundary nodes route through dst's anchor, so front/back are intact.
    EXPECT_EQ(dst.front().v, 1);
    EXPECT_EQ(dst.back().v, 3);

    // Move-assign onto a populated list drops its old contents and adopts dst's.
    Widget x{7};
    List other;
    other.pushBack(x);
    other = static_cast<List&&>(dst);
    EXPECT_EQ(seq(other), "123");
    EXPECT_TRUE(dst.empty());
}

TEST(IntrusiveListTest, SwapBothNonEmpty)
{
    Widget a{1}, b{2};
    Widget c{3}, d{4}, e{5};
    List x;
    x.pushBack(a);
    x.pushBack(b);
    List y;
    y.pushBack(c);
    y.pushBack(d);
    y.pushBack(e);

    swap(x, y);
    EXPECT_EQ(seq(x), "345");
    EXPECT_EQ(x.size(), 3u);
    EXPECT_EQ(seq(y), "12");
    EXPECT_EQ(y.size(), 2u);
    // Boundary integrity after re-seat.
    EXPECT_EQ(x.front().v, 3);
    EXPECT_EQ(x.back().v, 5);
}

TEST(IntrusiveListTest, SwapWithEmpty)
{
    Widget a{1}, b{2};
    List full;
    full.pushBack(a);
    full.pushBack(b);
    List empty;

    swap(full, empty);
    EXPECT_TRUE(full.empty());
    EXPECT_EQ(full.size(), 0u);
    EXPECT_EQ(seq(empty), "12");
    EXPECT_EQ(empty.size(), 2u);

    // Swapping two empties stays consistent.
    List e1, e2;
    swap(e1, e2);
    EXPECT_TRUE(e1.empty());
    EXPECT_TRUE(e2.empty());
}

TEST(IntrusiveListTest, ClearDetachesAll)
{
    Widget a{1}, b{2}, c{3};
    List l;
    l.pushBack(a);
    l.pushBack(b);
    l.pushBack(c);

    l.clear();
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.size(), 0u);
    EXPECT_EQ(l.begin(), l.end());
    // Cleared nodes are detached (links nulled), so they can be re-used elsewhere.
    EXPECT_EQ(static_cast<IntrusiveListNode&>(a).mpNext, nullptr);

    // Re-usable after clear.
    l.pushBack(a);
    EXPECT_EQ(seq(l), "1");
}
