#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.utility;
import worse.core.algorithm.modifying;

using namespace worse;
using namespace worse::core;

namespace
{
    // Move-assignment marks its source so we can prove `move` used move-assign,
    // not copy. Anonymous-namespace type => ADL never reaches std (no ambiguity
    // with std::move's 3-iterator overload).
    struct MoveMark
    {
        int v          = 0;
        bool movedFrom = false;
        MoveMark()     = default;
        explicit MoveMark(int x) : v(x) {}
        MoveMark(MoveMark const&)            = default;
        MoveMark& operator=(MoveMark const&) = default;
        MoveMark& operator=(MoveMark&& o) noexcept
        {
            v           = o.v;
            o.movedFrom = true;
            return *this;
        }
    };
} // namespace

TEST(ModifyingTest, CopyForward)
{
    int src[5] = {1, 2, 3, 4, 5};
    int dst[5] = {0, 0, 0, 0, 0};
    int* end   = copy(src, src + 5, dst);
    EXPECT_EQ(end, dst + 5);
    for (int i = 0; i < 5; ++i)
    {
        EXPECT_EQ(dst[i], i + 1);
    }
}

TEST(ModifyingTest, CopyBackwardOverlap)
{
    // Shift [0,4) right by 1 within the same buffer; backward copy handles the overlap.
    int a[6] = {1, 2, 3, 4, 0, 0};
    copyBackward(a, a + 4, a + 5);
    EXPECT_EQ(a[1], 1);
    EXPECT_EQ(a[4], 4);
    EXPECT_EQ(a[0], 1); // source head left intact by the shift
}

TEST(ModifyingTest, MoveUsesMoveAssign)
{
    MoveMark src[3] = {MoveMark{1}, MoveMark{2}, MoveMark{3}};
    MoveMark dst[3];
    move(src, src + 3, dst);
    EXPECT_EQ(dst[0].v, 1);
    EXPECT_EQ(dst[2].v, 3);
    EXPECT_TRUE(src[0].movedFrom); // proves move-assignment, not copy
    EXPECT_TRUE(src[2].movedFrom);
}

TEST(ModifyingTest, MoveBackward)
{
    int a[6] = {1, 2, 3, 4, 0, 0};
    moveBackward(a, a + 4, a + 6);
    EXPECT_EQ(a[2], 1);
    EXPECT_EQ(a[5], 4);
}

TEST(ModifyingTest, FillAndFillN)
{
    int a[5] = {0, 0, 0, 0, 0};
    fill(a, a + 3, 7);
    EXPECT_EQ(a[0], 7);
    EXPECT_EQ(a[2], 7);
    EXPECT_EQ(a[3], 0);

    int* end = fillN(a, 5, 9);
    EXPECT_EQ(end, a + 5);
    EXPECT_EQ(a[4], 9);
}

TEST(ModifyingTest, SwapRanges)
{
    int a[3] = {1, 2, 3};
    int b[3] = {4, 5, 6};
    swapRanges(a, a + 3, b);
    EXPECT_EQ(a[0], 4);
    EXPECT_EQ(b[0], 1);
}

TEST(ModifyingTest, ReverseEvenAndOdd)
{
    int even[4] = {1, 2, 3, 4};
    reverse(even, even + 4);
    EXPECT_EQ(even[0], 4);
    EXPECT_EQ(even[3], 1);

    int odd[5] = {1, 2, 3, 4, 5};
    reverse(odd, odd + 5);
    EXPECT_EQ(odd[0], 5);
    EXPECT_EQ(odd[2], 3); // middle fixed
    EXPECT_EQ(odd[4], 1);
}

TEST(ModifyingTest, Rotate)
{
    int a[6] = {1, 2, 3, 4, 5, 6};
    int* ret = rotate(a, a + 2, a + 6); // bring index 2 to front
    EXPECT_EQ(a[0], 3);
    EXPECT_EQ(a[3], 6);
    EXPECT_EQ(a[4], 1);
    EXPECT_EQ(a[5], 2);
    EXPECT_EQ(ret - a, 4); // old first (1) now at index 4
}

TEST(ModifyingTest, RemoveAndRemoveIf)
{
    int a[7] = {1, 2, 3, 2, 4, 2, 5};
    int* end = remove(a, a + 7, 2);
    EXPECT_EQ(end - a, 4); // {1,3,4,5}
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[1], 3);
    EXPECT_EQ(a[2], 4);
    EXPECT_EQ(a[3], 5);

    int b[6]  = {1, 2, 3, 4, 5, 6};
    int* end2 = removeIf(b, b + 6, [](int x)
                         { return x % 2 == 0; });
    EXPECT_EQ(end2 - b, 3); // {1,3,5}
    EXPECT_EQ(b[0], 1);
    EXPECT_EQ(b[2], 5);
}

TEST(ModifyingTest, Unique)
{
    int a[8] = {1, 1, 2, 3, 3, 3, 4, 4};
    int* end = unique(a, a + 8);
    EXPECT_EQ(end - a, 4); // {1,2,3,4}
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[1], 2);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a[3], 4);
}

TEST(ModifyingTest, ReplaceAndReplaceIf)
{
    int a[5] = {1, 2, 1, 3, 1};
    replace(a, a + 5, 1, 9);
    EXPECT_EQ(a[0], 9);
    EXPECT_EQ(a[2], 9);
    EXPECT_EQ(a[4], 9);
    EXPECT_EQ(a[1], 2);

    int b[5] = {1, 2, 3, 4, 5};
    replaceIf(b, b + 5, [](int x)
              { return x > 3; },
              0);
    EXPECT_EQ(b[3], 0);
    EXPECT_EQ(b[4], 0);
    EXPECT_EQ(b[2], 3);
}

TEST(ModifyingTest, Constexpr)
{
    // Exercises the constexpr (loop) path of copy/fill/reverse.
    constexpr auto build = []
    {
        int a[5] = {0, 0, 0, 0, 0};
        fill(a, a + 5, 1);
        int src[5] = {5, 4, 3, 2, 1};
        copy(src, src + 5, a);
        reverse(a, a + 5);
        return a[0]; // after reverse of {5,4,3,2,1} -> {1,2,3,4,5}
    };
    static_assert(build() == 1);
    SUCCEED();
}
