#include <gtest/gtest.h>

#include "worse/core/macro.hpp"

#include <string>

import worse.core.basic_type;
import worse.core.type_traits;

using namespace worse;
using namespace worse::core;

namespace
{
    // Not trivially copyable (user move ctor + dtor), but it only owns a raw pointer,
    // so relocating it IS a byte copy — we opt it in below.
    struct Relocatable
    {
        int* p        = nullptr;
        Relocatable() = default;
        Relocatable(Relocatable&& o) noexcept : p(o.p) { o.p = nullptr; }
        Relocatable& operator=(Relocatable&&) noexcept { return *this; }
        ~Relocatable() {}
    };

    // No comparison operators at all (aggregates get none implicitly in C++20).
    struct NoOps
    {
        int x;
    };
} // namespace

// Opt-in must sit at namespace scope; the type is reachable here.
WE_DECLARE_TRIVIALLY_RELOCATABLE(Relocatable);

TEST(TypeTraitsTest, Transformations)
{
    static_assert(IsSame<RemoveCvRef<int const&>, int>);
    static_assert(IsSame<RemoveConst<int const>, int>);
    static_assert(IsSame<RemovePointer<int*>, int>);
    static_assert(IsSame<Decay<int const&>, int>);
    static_assert(IsSame<Conditional<true, int, f32>, int>);
    static_assert(IsSame<Conditional<false, int, f32>, f32>);
    SUCCEED();
}

TEST(TypeTraitsTest, TrivialityValues)
{
    static_assert(IsTriviallyCopyable<int>);
    static_assert(IsTriviallyCopyable<f32>);
    static_assert(!IsTriviallyCopyable<std::string>);
    static_assert(IsTriviallyDestructible<int>);
    static_assert(!IsTriviallyDestructible<std::string>);
    static_assert(IsIntegral<u32>);
    static_assert(IsFloating<f64>);
    static_assert(IsPointer<int*>);
    static_assert(IsEmpty<NoOps> == false); // has a data member
    SUCCEED();
}

TEST(TypeTraitsTest, IsTriviallyRelocatableDefaults)
{
    static_assert(IsTriviallyRelocatable<int>);
    static_assert(IsTriviallyRelocatable<f32>);
    static_assert(IsTriviallyRelocatable<int*>);
    static_assert(IsTriviallyRelocatable<NoOps>); // trivially copyable -> auto-qualifies
    // Not trivially copyable and not declared -> not relocatable.
    static_assert(!IsTriviallyRelocatable<std::string>);
    SUCCEED();
}

TEST(TypeTraitsTest, DeclareTriviallyRelocatable)
{
    // Proves the default WOULD be false (it is not trivially copyable)...
    static_assert(!IsTriviallyCopyable<Relocatable>);
    // ...and that the WE_DECLARE_TRIVIALLY_RELOCATABLE opt-in flipped it to true,
    // with cv-qualifiers stripped by the trait.
    static_assert(IsTriviallyRelocatable<Relocatable>);
    static_assert(IsTriviallyRelocatable<Relocatable const>);
    static_assert(TriviallyRelocatable<Relocatable>); // concept form
    SUCCEED();
}

TEST(TypeTraitsTest, Concepts)
{
    static_assert(EqualityComparable<int>);
    static_assert(EqualityComparable<std::string>);
    static_assert(LessThanComparable<int>);
    static_assert(!EqualityComparable<NoOps>);
    static_assert(!LessThanComparable<NoOps>);

    auto const hash = [](int v)
    {
        return static_cast<usize>(v);
    };
    static_assert(HashFor<decltype(hash), int>);

    auto const less = [](int a, int b)
    {
        return a < b;
    };
    static_assert(CompareFor<decltype(less), int>);

    auto const pred = [](int v)
    {
        return v > 0;
    };
    static_assert(PredicateFor<decltype(pred), int>);
    SUCCEED();
}
