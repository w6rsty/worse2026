#include <gtest/gtest.h>

#include "worse/core/macro.hpp"

TEST(MacrosTest, VerifyPassesWhenTrue)
{
    WE_VERIFY(true);
    WE_ASSERT(true);
    SUCCEED();
}

TEST(MacrosTest, VerifyAbortsWhenFalse)
{
    // Failure aborts silently; the empty matcher accepts any (no) output.
    EXPECT_DEATH(WE_VERIFY(false), "");
}

TEST(MacrosTest, AssertAbortsWhenFalse)
{
    // In this debug build WE_ASSERT maps to WE_VERIFY.
    EXPECT_DEATH(WE_ASSERT(false), "");
}
