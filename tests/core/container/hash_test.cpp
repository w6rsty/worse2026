#include <gtest/gtest.h>

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.container.hash;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    enum class Color : unsigned
    {
        Red   = 0,
        Green = 1,
        Blue  = 2
    };

    // No Hash<> specialization exists for this -> Hash<Unsupported> stays incomplete.
    struct Unsupported
    {
        int x;
    };
} // namespace

TEST(HashTest, IntegralDeterministicAndDistinct)
{
    Hash<int> h;
    EXPECT_EQ(h(42), h(42)); // deterministic
    EXPECT_NE(h(1), h(2));
    EXPECT_NE(h(0), h(1));
    // Sequential keys must not collide in the low (bucket-index) bits after masking.
    constexpr usize mask = 1023; // a 1024-bucket table
    usize a              = h(100) & mask;
    usize b              = h(101) & mask;
    usize c              = h(102) & mask;
    EXPECT_FALSE(a == b && b == c);
}

TEST(HashTest, WideAndSignedIntegers)
{
    Hash<u64> hu;
    Hash<i64> hi;
    EXPECT_NE(hu(0u), hu(U64_MAX));
    EXPECT_NE(hi(-1), hi(1));
    EXPECT_EQ(hi(-5), hi(-5));
}

TEST(HashTest, EnumHashesEqualUnderlyingInteger)
{
    Hash<Color> hc;
    Hash<unsigned> hu;
    EXPECT_EQ(hc(Color::Red), hu(0u));
    EXPECT_EQ(hc(Color::Green), hu(1u));
    EXPECT_EQ(hc(Color::Blue), hu(2u));
    EXPECT_NE(hc(Color::Red), hc(Color::Blue));
}

TEST(HashTest, BoolAndChar)
{
    Hash<bool> hb;
    EXPECT_NE(hb(true), hb(false));

    Hash<char> hch;
    EXPECT_NE(hch('a'), hch('b'));
    EXPECT_EQ(hch('z'), hch('z'));
}

TEST(HashTest, PointerHashesByAddress)
{
    int a = 0;
    int b = 0;
    Hash<int*> hp;
    EXPECT_EQ(hp(&a), hp(&a));
    EXPECT_NE(hp(&a), hp(&b));
    EXPECT_EQ(hp(nullptr), hp(nullptr));
}

TEST(HashTest, CombineIsOrderSensitive)
{
    usize const ab = hashCombine(hashCombine(0, 1), 2);
    usize const ba = hashCombine(hashCombine(0, 2), 1);
    EXPECT_NE(ab, ba);
    EXPECT_EQ(hashCombine(7, 9), hashCombine(7, 9)); // deterministic
}

TEST(HashTest, BytesEqualAndDiffer)
{
    char const* s1 = "hello";
    char const* s2 = "hello";
    char const* s3 = "world";
    EXPECT_EQ(hashBytes(s1, 5), hashBytes(s2, 5));
    EXPECT_NE(hashBytes(s1, 5), hashBytes(s3, 5));
    EXPECT_NE(hashBytes(s1, 5), hashBytes(s1, 4)); // length matters
}

TEST(HashTest, FinalizeAvalanche)
{
    // fmix64(0) == 0 is a known fixed point of murmur3; that is fine -- what matters is
    // that NON-zero tiny inputs scatter into the full 64-bit range and stay distinct.
    EXPECT_EQ(hashFinalize(0), 0u);
    EXPECT_NE(hashFinalize(1), hashFinalize(2));
    EXPECT_GT(hashFinalize(1), 0xffffffffULL); // a tiny input populates the high half
    EXPECT_GT(hashFinalize(2) >> 32, 0u);
}

// Compile-time: the supported keys satisfy HashFor; an unsupported key does not
// (its Hash primary stays declared-but-undefined, so it has no operator()).
static_assert(HashFor<Hash<int>, int>);
static_assert(HashFor<Hash<unsigned>, unsigned>);
static_assert(HashFor<Hash<Color>, Color>);
static_assert(HashFor<Hash<int*>, int*>);
static_assert(!HashFor<Hash<Unsupported>, Unsupported>);
