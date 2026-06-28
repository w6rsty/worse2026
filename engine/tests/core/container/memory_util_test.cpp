#include <gtest/gtest.h>

#include "worse/core/macro.hpp"

#include <cstddef> // std::byte
#include <new>     // placement new (test setup only)

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.container.allocator;
import worse.core.container.memory_util;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    // Instrumented element type, parameterized by Tag so each variant has its own
    // independent counters. Non-trivially-copyable (user move/copy/dtor) so it does
    // NOT auto-qualify as trivially relocatable -- one variant is opted in below.
    template <int Tag>
    struct BasicTracked
    {
        static int ctor;
        static int copy;
        static int move;
        static int dtor;

        int v = 0;

        BasicTracked() { ++ctor; }
        explicit BasicTracked(int x) : v(x) { ++ctor; }
        BasicTracked(BasicTracked const& o) : v(o.v) { ++copy; }
        BasicTracked(BasicTracked&& o) noexcept : v(o.v)
        {
            o.v = -1;
            ++move;
        }
        BasicTracked& operator=(BasicTracked const&) = default;
        BasicTracked& operator=(BasicTracked&&)      = default;
        ~BasicTracked() { ++dtor; }

        static void reset() { ctor = copy = move = dtor = 0; }
    };
    template <int Tag>
    int BasicTracked<Tag>::ctor = 0;
    template <int Tag>
    int BasicTracked<Tag>::copy = 0;
    template <int Tag>
    int BasicTracked<Tag>::move = 0;
    template <int Tag>
    int BasicTracked<Tag>::dtor = 0;

    using Plain = BasicTracked<0>; // not relocatable -> move+destroy loop
    using Reloc = BasicTracked<1>; // relocatable     -> memcpy fast path

    // Raw aligned storage for N objects of T (uninitialized).
    template <typename T, usize N>
    struct RawStorage
    {
        alignas(T) std::byte bytes[sizeof(T) * N];
        T* ptr() { return reinterpret_cast<T*>(bytes); }
    };
} // namespace

WE_DECLARE_TRIVIALLY_RELOCATABLE(Reloc);

TEST(MemoryUtilTest, TraitSanity)
{
    static_assert(!IsTriviallyCopyable<Plain>);
    static_assert(!IsTriviallyRelocatable<Plain>);
    static_assert(IsTriviallyRelocatable<Reloc>); // opted in via the macro
}

TEST(MemoryUtilTest, DestroyRangeRunsDtorsForNonTrivial)
{
    Allocator alloc;
    RawStorage<Plain, 3> s;
    Plain* p = s.ptr();
    for (int i = 0; i < 3; ++i)
    {
        ::new (static_cast<void*>(p + i)) Plain(i);
    }
    Plain::reset();
    destroyRange(alloc, p, p + 3);
    EXPECT_EQ(Plain::dtor, 3);
}

TEST(MemoryUtilTest, DestroyRangeIsNoOpForTrivial)
{
    Allocator alloc;
    int buf[4] = {1, 2, 3, 4};
    // Trivially destructible -> compiled-out no-op, values untouched.
    destroyRange(alloc, buf, buf + 4);
    EXPECT_EQ(buf[0], 1);
    EXPECT_EQ(buf[3], 4);
}

TEST(MemoryUtilTest, ValueConstructZeroesTrivial)
{
    Allocator alloc;
    RawStorage<int, 4> s;
    int* p = s.ptr();
    p[0] = p[1] = p[2] = p[3] = 0xBADBAD;
    uninitializedValueConstruct(alloc, p, p + 4);
    EXPECT_EQ(p[0], 0);
    EXPECT_EQ(p[3], 0);
}

TEST(MemoryUtilTest, FillAndFillN)
{
    Allocator alloc;
    RawStorage<Plain, 5> s;
    Plain* p = s.ptr();

    Plain::reset();
    uninitializedFill(alloc, p, p + 3, Plain{42});
    EXPECT_EQ(Plain::copy, 3);
    EXPECT_EQ(p[0].v, 42);
    EXPECT_EQ(p[2].v, 42);

    Plain* end = uninitializedFillN(alloc, p + 3, 2, Plain{7});
    EXPECT_EQ(end, p + 5);
    EXPECT_EQ(p[4].v, 7);

    destroyRange(alloc, p, p + 5);
}

TEST(MemoryUtilTest, UninitializedCopyMemcpyFastPath)
{
    Allocator alloc;
    int src[4] = {5, 6, 7, 8};
    RawStorage<int, 4> d;
    int* dst = d.ptr();
    int* end = uninitializedCopy(alloc, src, src + 4, dst);
    EXPECT_EQ(end, dst + 4);
    EXPECT_EQ(dst[0], 5);
    EXPECT_EQ(dst[3], 8);
}

TEST(MemoryUtilTest, UninitializedMoveNonTrivial)
{
    Allocator alloc;
    RawStorage<Plain, 3> ss;
    RawStorage<Plain, 3> ds;
    Plain* src = ss.ptr();
    Plain* dst = ds.ptr();
    for (int i = 0; i < 3; ++i)
    {
        ::new (static_cast<void*>(src + i)) Plain(i + 1);
    }
    Plain::reset();
    uninitializedMove(alloc, src, src + 3, dst);
    EXPECT_EQ(Plain::move, 3); // move-constructed, not copied
    EXPECT_EQ(Plain::copy, 0);
    EXPECT_EQ(dst[0].v, 1);
    EXPECT_EQ(dst[2].v, 3);
    EXPECT_EQ(src[0].v, -1); // moved-from

    destroyRange(alloc, src, src + 3);
    destroyRange(alloc, dst, dst + 3);
}

TEST(MemoryUtilTest, RelocateNonTrivialMovesAndDestroys)
{
    Allocator alloc;
    RawStorage<Plain, 4> ss;
    RawStorage<Plain, 4> ds;
    Plain* src = ss.ptr();
    Plain* dst = ds.ptr();
    for (int i = 0; i < 4; ++i)
    {
        ::new (static_cast<void*>(src + i)) Plain(i + 1);
    }

    Plain::reset();
    Plain* end = relocate(alloc, src, src + 4, dst);
    // Non-relocatable path: each element move-constructed at dst and source destroyed.
    EXPECT_EQ(end, dst + 4);
    EXPECT_EQ(Plain::move, 4);
    EXPECT_EQ(Plain::dtor, 4); // sources destroyed by relocate
    EXPECT_EQ(dst[0].v, 1);
    EXPECT_EQ(dst[3].v, 4);

    destroyRange(alloc, dst, dst + 4); // only dst is live now
}

TEST(MemoryUtilTest, RelocateTrivialUsesMemcpyNoMovesNoDtors)
{
    Allocator alloc;
    RawStorage<Reloc, 4> ss;
    RawStorage<Reloc, 4> ds;
    Reloc* src = ss.ptr();
    Reloc* dst = ds.ptr();
    for (int i = 0; i < 4; ++i)
    {
        ::new (static_cast<void*>(src + i)) Reloc(i + 1);
    }

    Reloc::reset();
    Reloc* end = relocate(alloc, src, src + 4, dst);
    // memcpy fast path: NO move ctor, NO destructor calls during relocate.
    EXPECT_EQ(end, dst + 4);
    EXPECT_EQ(Reloc::move, 0);
    EXPECT_EQ(Reloc::dtor, 0);
    EXPECT_EQ(dst[0].v, 1); // bytes copied verbatim
    EXPECT_EQ(dst[3].v, 4);

    // dst owns the objects now; source bytes are abandoned (do NOT destroy them).
    destroyRange(alloc, dst, dst + 4);
    EXPECT_EQ(Reloc::dtor, 4);
}

TEST(MemoryUtilTest, RelocateBackwardTrivial)
{
    Allocator alloc;
    RawStorage<Reloc, 4> ss;
    RawStorage<Reloc, 4> ds;
    Reloc* src = ss.ptr();
    Reloc* dst = ds.ptr();
    for (int i = 0; i < 4; ++i)
    {
        ::new (static_cast<void*>(src + i)) Reloc(i + 1);
    }

    Reloc::reset();
    Reloc* begin = relocateBackward(alloc, src, src + 4, dst + 4);
    EXPECT_EQ(begin, dst);
    EXPECT_EQ(Reloc::move, 0);
    EXPECT_EQ(dst[0].v, 1);
    EXPECT_EQ(dst[3].v, 4);
    destroyRange(alloc, dst, dst + 4);
}

TEST(MemoryUtilTest, RelocateBackwardNonTrivial)
{
    Allocator alloc;
    RawStorage<Plain, 4> ss;
    RawStorage<Plain, 4> ds;
    Plain* src = ss.ptr();
    Plain* dst = ds.ptr();
    for (int i = 0; i < 4; ++i)
    {
        ::new (static_cast<void*>(src + i)) Plain(i + 1);
    }
    Plain::reset();
    Plain* begin = relocateBackward(alloc, src, src + 4, dst + 4);
    EXPECT_EQ(begin, dst);
    EXPECT_EQ(Plain::move, 4);
    EXPECT_EQ(Plain::dtor, 4);
    EXPECT_EQ(dst[0].v, 1);
    EXPECT_EQ(dst[3].v, 4);
    destroyRange(alloc, dst, dst + 4);
}
