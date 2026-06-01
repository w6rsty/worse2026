#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.utility;
import worse.core.container.array;
import worse.core.container.priority_queue;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    // Counts live instances so we can assert the adapter leaks nothing. Ordered by `v`.
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
        friend bool operator<(Tracked const& a, Tracked const& b) { return a.v < b.v; }
        static void reset() { alive = 0; }
    };
    int Tracked::alive = 0;
} // namespace

TEST(PriorityQueueTest, DefaultIsMaxHeap)
{
    PriorityQueue<int> pq;
    EXPECT_TRUE(pq.empty());
    for (int x : {3, 1, 4, 1, 5, 9, 2, 6})
    {
        pq.push(x);
    }
    EXPECT_EQ(pq.size(), 8u);
    EXPECT_EQ(pq.top(), 9);

    int prev = 1000;
    while (!pq.empty())
    {
        EXPECT_LE(pq.top(), prev); // non-increasing pop order
        prev = pq.top();
        pq.pop();
    }
    EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueTest, GreaterGivesMinHeap)
{
    PriorityQueue<int, Array<int>, Greater<>> pq;
    for (int x : {3, 1, 4, 1, 5, 9, 2, 6})
    {
        pq.push(x);
    }
    EXPECT_EQ(pq.top(), 1);

    int prev = -1;
    while (!pq.empty())
    {
        EXPECT_GE(pq.top(), prev); // non-decreasing pop order
        prev = pq.top();
        pq.pop();
    }
}

TEST(PriorityQueueTest, Emplace)
{
    PriorityQueue<Tracked> pq;
    pq.emplace(5);
    pq.emplace(2);
    pq.emplace(8);
    EXPECT_EQ(pq.top().v, 8);
    pq.pop();
    EXPECT_EQ(pq.top().v, 5);
}

TEST(PriorityQueueTest, RangeConstructorHeapifies)
{
    int data[] = {7, 2, 9, 0, 4, 4, 11, 3};
    PriorityQueue<int> pq(data, data + 8);
    EXPECT_EQ(pq.size(), 8u);
    EXPECT_EQ(pq.top(), 11);

    int sorted[8];
    int i = 0;
    while (!pq.empty())
    {
        sorted[i++] = pq.top();
        pq.pop();
    }
    for (int k = 1; k < 8; ++k)
    {
        EXPECT_GE(sorted[k - 1], sorted[k]);
    }
}

TEST(PriorityQueueTest, InitializerListConstructor)
{
    PriorityQueue<int> pq{1, 8, 3, 8, 2};
    EXPECT_EQ(pq.size(), 5u);
    EXPECT_EQ(pq.top(), 8);
    pq.pop();
    EXPECT_EQ(pq.top(), 8); // the duplicate top
}

TEST(PriorityQueueTest, SeedFromContainerMove)
{
    Array<int> seed{5, 1, 9, 3};
    PriorityQueue<int> pq(Less<>{}, static_cast<Array<int>&&>(seed));
    EXPECT_EQ(pq.size(), 4u);
    EXPECT_EQ(pq.top(), 9);
    EXPECT_TRUE(seed.empty()); // container was moved-from
}

TEST(PriorityQueueTest, ClearAndSwap)
{
    PriorityQueue<int> a{1, 2, 3};
    PriorityQueue<int> b{10, 20};
    swap(a, b);
    EXPECT_EQ(a.top(), 20);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(b.top(), 3);
    EXPECT_EQ(b.size(), 3u);

    a.clear();
    EXPECT_TRUE(a.empty());
}

TEST(PriorityQueueTest, NoLeaks)
{
    Tracked::reset();
    {
        PriorityQueue<Tracked> pq;
        for (int i = 0; i < 32; ++i)
        {
            pq.emplace((i * 7) % 17);
        }
        for (int i = 0; i < 10; ++i)
        {
            pq.pop();
        }
        PriorityQueue<Tracked> copy = pq; // copy ctor (implicit, via container)
        EXPECT_GT(Tracked::alive, 0);
        (void)copy;
    }
    EXPECT_EQ(Tracked::alive, 0); // balanced ctors/dtors
}
