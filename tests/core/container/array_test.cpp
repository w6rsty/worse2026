#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.container.array;
import worse.core.container.allocator;

using namespace worse;
using namespace worse::core::container;

TEST(ArrayTest, Template)
{
    struct Foo
    {
        u32 a;
        f32 b;
    };

    struct EmptyAlloc
    {
    };

    struct Alloc
    {
        u32 counter;
        u32 pointer;
    };

    struct Type
    {
        Array<Foo, EmptyAlloc> _1;
        Array<Foo, Alloc> _3;

        Array<Foo> _4;
    };

    static_assert(sizeof(Type::_1) == (sizeof(Foo*) * 3));
    static_assert(sizeof(Type::_3) == (sizeof(Foo*) * 3 + sizeof(Alloc)));
}
