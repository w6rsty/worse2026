#include <gtest/gtest.h>

// Pull the ENTIRE container library through the single umbrella import: this test exists to
// prove every submodule's exported names are visible via `worse.core.container.container`.
import worse.core.container.container;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

TEST(ContainerUmbrellaTest, EverySubmoduleReachableThroughUmbrella)
{
    // contiguous
    Array<int> arr;
    arr.pushBack(1);
    StaticArray<int, 2> sa{1, 2};
    FixedArray<int, 2> fa{3, 4};

    // adapters / flat / intrusive
    PriorityQueue<int> pq;
    pq.push(5);
    FlatSet<int> fs;
    fs.insert(7);
    FlatMap<int, int> fm;
    fm[1] = 2;

    // hash
    UnorderedSet<int> us;
    us.insert(9);
    UnorderedMap<int, int> um;
    um[1] = 2;

    // node lists
    List<int> li{1, 2};
    ForwardList<int> fl{3, 4};
    FixedList<int, 4> fli{5, 6};
    FixedSList<int, 4> fsl{7, 8};

    EXPECT_EQ(arr.size(), 1u);
    EXPECT_EQ(sa.size(), 2u);
    EXPECT_EQ(fa.size(), 2u);
    EXPECT_EQ(pq.top(), 5);
    EXPECT_TRUE(fs.contains(7));
    EXPECT_EQ(fm.at(1), 2);
    EXPECT_TRUE(us.contains(9));
    EXPECT_EQ(um.at(1), 2);
    EXPECT_EQ(li.front(), 1);
    EXPECT_EQ(fl.front(), 3);
    EXPECT_EQ(fli.front(), 5);
    EXPECT_EQ(fsl.front(), 7);

    // free functions from the infra modules are reachable too (iterator).
    static_assert(BidirectionalIterator<List<int>::Iterator>);
    SUCCEED();
}
