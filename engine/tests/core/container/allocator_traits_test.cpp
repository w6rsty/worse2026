#include <gtest/gtest.h>

#include <string>

import worse.core.basic_type;
import worse.core.container.allocator;
import worse.core.container.allocator_traits;

using namespace worse;
using namespace worse::core::container;

using Traits = AllocatorTraits<Allocator>;

// --- policy is detected from the allocator's nested types --------------------
TEST(AllocatorTraitsTest, Policy)
{
    // Allocator declares `using IsAlwaysEqual = std::true_type;`
    EXPECT_TRUE(Traits::isAlwaysEqual);
    // It declares none of the propagate-* traits -> standard default (false).
    EXPECT_FALSE(Traits::propagateOnContainerCopyAssignment);
    EXPECT_FALSE(Traits::propagateOnContainerMoveAssignment);
    EXPECT_FALSE(Traits::propagateOnContainerSwap);

    Allocator a{"a"};
    Allocator b{"b"};
    // Always-equal -> equal() short-circuits to true regardless of name.
    EXPECT_TRUE(Traits::equal(a, b));

    // selectOnContainerCopyConstruction defaults to a plain copy.
    Allocator copy = Traits::selectOnContainerCopyConstruction(a);
    EXPECT_STREQ(copy.getName(), "a");
}

// --- raw allocate / deallocate roundtrip through the traits ------------------
TEST(AllocatorTraitsTest, AllocateDeallocate)
{
    Allocator a;

    void* p = Traits::allocate(a, 64 * sizeof(u32), alignof(u32));
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<usize>(p) % alignof(u32), 0u);
    Traits::deallocate(a, p, 64 * sizeof(u32), alignof(u32));

    // Over-aligned + offset overload also routes correctly.
    void* q = Traits::allocate(a, 128, 64 /*align*/, 0 /*offset*/);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(reinterpret_cast<usize>(q) % 64u, 0u);
    Traits::deallocate(a, q, 128, 64);
}

// --- construct / destroy drive element lifetime (allocator stays raw) --------
TEST(AllocatorTraitsTest, ConstructDestroy)
{
    Allocator a;

    auto* p = static_cast<std::string*>(
        Traits::allocate(a, sizeof(std::string), alignof(std::string)));
    ASSERT_NE(p, nullptr);

    Traits::construct(a, p, "hello world, this is long enough to heap-allocate");
    EXPECT_EQ(*p, "hello world, this is long enough to heap-allocate");
    Traits::destroy(a, p); // must run ~string (no leak)

    Traits::deallocate(a, p, sizeof(std::string), alignof(std::string));
}
