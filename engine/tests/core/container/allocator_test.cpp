#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.container.allocator;

using namespace worse;
using namespace worse::core::container;

TEST(AllocatorTest, Default)
{
    Allocator alloc;

    struct Foo
    {
        u32 _1;
        f32 _2;
    };

    Foo* foo = reinterpret_cast<Foo*>(alloc.allocate(sizeof(Foo), alignof(Foo)));
    EXPECT_TRUE(foo != nullptr);

    alloc.deallocate(foo, sizeof(Foo), alignof(Foo));

    (void)sizeof(Foo const[3]);
}
