// Micro-benchmarks for the contiguous containers. Single translation unit, so it also
// hosts the nanobench implementation. Run: ./build/<preset>/bench/wcontainer_bench
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include "worse/core/macro.hpp"

import worse.core.basic_type;
import worse.core.type_traits;
import worse.core.container.array;
import worse.core.container.fixed_array;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    // ~64-byte payload, non-trivially-copyable (user move ctor + dtor) so it does NOT
    // auto-qualify as relocatable. Tag 1 is opted into trivial relocation below, so the
    // two variants differ ONLY in which grow path the Array takes (memcpy vs move+dtor).
    template <int Tag>
    struct Payload
    {
        u64 data[8]             = {};
        Payload()               = default;
        Payload(Payload const&) = default;
        Payload(Payload&& o) noexcept
        {
            for (int i = 0; i < 8; ++i)
            {
                data[i] = o.data[i];
            }
        }
        Payload& operator=(Payload const&)     = default;
        Payload& operator=(Payload&&) noexcept = default;
        ~Payload() {}
    };
    using PlainPayload = Payload<0>;
    using RelocPayload = Payload<1>;
} // namespace

WE_DECLARE_TRIVIALLY_RELOCATABLE(RelocPayload);

int main()
{
    namespace nb = ankerl::nanobench;

    constexpr int N = 4096;

    nb::Bench bench;
    // Each run allocates/destroys thousands of elements, so a few hundred epoch
    // iterations is plenty to stabilize while keeping the whole suite sub-second.
    bench.title("worse::core::container").relative(true).minEpochIterations(500);

    // --- grow: trivial relocation (memcpy) vs move+destroy ---------------------
    bench.run("Array<RelocPayload> push x4096 (memcpy grow)",
              [&]
              {
                  Array<RelocPayload> a;
                  for (int i = 0; i < N; ++i)
                  {
                      a.emplaceBack();
                  }
                  nb::doNotOptimizeAway(a.data());
              });
    bench.run("Array<PlainPayload> push x4096 (move+dtor grow)",
              [&]
              {
                  Array<PlainPayload> a;
                  for (int i = 0; i < N; ++i)
                  {
                      a.emplaceBack();
                  }
                  nb::doNotOptimizeAway(a.data());
              });

    // --- effect of reserve -----------------------------------------------------
    bench.run("Array<int> push x4096 (no reserve)",
              [&]
              {
                  Array<int> a;
                  for (int i = 0; i < N; ++i)
                  {
                      a.pushBack(i);
                  }
                  nb::doNotOptimizeAway(a.data());
              });
    bench.run("Array<int> push x4096 (reserve)",
              [&]
              {
                  Array<int> a;
                  a.reserve(N);
                  for (int i = 0; i < N; ++i)
                  {
                      a.pushBack(i);
                  }
                  nb::doNotOptimizeAway(a.data());
              });

    // --- O(1) eraseUnsorted vs O(n) erase(begin) -------------------------------
    bench.run("Array<int> erase(begin) x256 (ordered, O(n))",
              [&]
              {
                  Array<int> a;
                  a.reserve(N);
                  for (int i = 0; i < N; ++i)
                  {
                      a.pushBack(i);
                  }
                  for (int i = 0; i < 256; ++i)
                  {
                      a.erase(a.begin());
                  }
                  nb::doNotOptimizeAway(a.data());
              });
    bench.run("Array<int> eraseUnsorted(begin) x256 (O(1))",
              [&]
              {
                  Array<int> a;
                  a.reserve(N);
                  for (int i = 0; i < N; ++i)
                  {
                      a.pushBack(i);
                  }
                  for (int i = 0; i < 256; ++i)
                  {
                      a.eraseUnsorted(a.begin());
                  }
                  nb::doNotOptimizeAway(a.data());
              });

    // --- zero-allocation inline storage ----------------------------------------
    bench.run("FixedArray<int,4096> push x4096 (zero alloc)",
              [&]
              {
                  FixedArray<int, N> a;
                  for (int i = 0; i < N; ++i)
                  {
                      a.pushBack(i);
                  }
                  nb::doNotOptimizeAway(a.data());
              });

    return 0;
}
