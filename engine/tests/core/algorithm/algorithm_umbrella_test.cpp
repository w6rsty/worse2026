#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.utility;
// Single umbrella import must surface every algorithm submodule's exports.
import worse.core.algorithm.algorithm;

using namespace worse;
using namespace worse::core;

// One symbol from each submodule, reached only through the umbrella re-export.
TEST(AlgorithmUmbrellaTest, ReExportsEverySubmodule)
{
    int a[6] = {5, 2, 4, 1, 3, 6};

    sort(a, a + 6); // sort
    EXPECT_TRUE(isSorted(a, a + 6));

    EXPECT_TRUE(binarySearch(a, a + 6, 4)); // binary_search
    EXPECT_EQ(find(a, a + 6, 3) - a, 2);    // nonmodifying

    reverse(a, a + 6); // modifying
    EXPECT_EQ(a[0], 6);

    makeHeap(a, a + 6); // heap
    EXPECT_TRUE(isHeap(a, a + 6));
}
