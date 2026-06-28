// Algorithm micro-benchmarks: worse::core sort/stableSort vs std vs EASTL on a random int[].
// Each run rebuilds the array from the same random source (all three pay the copy equally) and
// sorts it. Implementation + main live in bench_main.cpp.
#include <nanobench.h>

#include "benches.hpp"

#include <algorithm> // std::sort, std::stable_sort
#include <vector>

#include <EASTL/sort.h>

import worse.core.basic_type;
import worse.core.algorithm.sort;

using namespace worse;
using namespace worse::core;

namespace
{
    constexpr int kN = 4096;

    std::vector<int> const& randomInts()
    {
        static std::vector<int> const data = []
        {
            std::vector<int> v;
            v.reserve(kN);
            u32 s = 0x2545F491u;
            for (int i = 0; i < kN; ++i)
            {
                s = s * 1664525u + 1013904223u;
                v.push_back(static_cast<int>(s % 100000u));
            }
            return v;
        }();
        return data;
    }
} // namespace

void benchAlgorithms(ankerl::nanobench::Bench& bench)
{
    namespace nb  = ankerl::nanobench;
    auto const& d = randomInts();

    bench.title("sort / stableSort (4096 random ints): worse vs std vs EASTL");

    // --- unstable sort ---------------------------------------------------------
    bench.run("worse::sort",
              [&]
              {
                  std::vector<int> a(d);
                  worse::core::sort(a.data(), a.data() + kN);
                  nb::doNotOptimizeAway(a[0]);
              });
    bench.run("std::sort",
              [&]
              {
                  std::vector<int> a(d);
                  std::sort(a.data(), a.data() + kN);
                  nb::doNotOptimizeAway(a[0]);
              });
    bench.run("eastl::sort",
              [&]
              {
                  std::vector<int> a(d);
                  eastl::sort(a.data(), a.data() + kN);
                  nb::doNotOptimizeAway(a[0]);
              });

    // --- stable sort -----------------------------------------------------------
    bench.run("worse::stableSort",
              [&]
              {
                  std::vector<int> a(d);
                  worse::core::stableSort(a.data(), a.data() + kN);
                  nb::doNotOptimizeAway(a[0]);
              });
    bench.run("std::stable_sort",
              [&]
              {
                  std::vector<int> a(d);
                  std::stable_sort(a.data(), a.data() + kN);
                  nb::doNotOptimizeAway(a[0]);
              });
    bench.run("eastl::stable_sort",
              [&]
              {
                  std::vector<int> a(d);
                  eastl::stable_sort(a.data(), a.data() + kN);
                  nb::doNotOptimizeAway(a[0]);
              });
}
