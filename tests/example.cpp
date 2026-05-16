#include <gtest/gtest.h>

import worse;

TEST(WcoroTest, Add)
{
    EXPECT_EQ(worse::add(2, 3), 5);
}
