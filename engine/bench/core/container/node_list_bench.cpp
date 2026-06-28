// Node-list micro-benchmarks: worse::core::container list family vs the std equivalents, to
// validate the engine containers are at least as fast as the STL (and to show the zero-heap
// fixed_list win). Implementation + main live in bench_main.cpp.
#include <nanobench.h>

#include "benches.hpp"

#include <forward_list>
#include <list>
#include <vector>

#include <EASTL/list.h>
#include <EASTL/slist.h>

import worse.core.basic_type;
import worse.core.container.list;
import worse.core.container.forward_list;
import worse.core.container.fixed_list;
import worse.core.container.fixed_slist;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    constexpr int kN = 4096;

    // Thin always-equal allocator that calls global new/delete directly (inlinable, no
    // tracking seam) -- isolates the CONTAINER's per-node cost from the default Allocator's
    // tracking-hook overhead, for an apples-to-apples comparison against std::list.
    struct ThinAlloc
    {
        using IsAlwaysEqual = std::true_type;
        void* allocate(usize n, usize) noexcept { return ::operator new(n); }
        void deallocate(void* p, usize, usize) noexcept { ::operator delete(p); }
        friend bool operator==(ThinAlloc, ThinAlloc) noexcept { return true; }
    };

    // Deterministic pseudo-random ints (LCG) so every run sorts the identical sequence and
    // the numbers are reproducible -- no <random>, no global state.
    std::vector<int> const& randomData()
    {
        static std::vector<int> const data = []
        {
            std::vector<int> v;
            v.reserve(kN);
            u64 s = 0x9E3779B97F4A7C15ull;
            for (int i = 0; i < kN; ++i)
            {
                s = s * 6364136223846793005ull + 1442695040888963407ull;
                v.push_back(static_cast<int>(s >> 33));
            }
            return v;
        }();
        return data;
    }
} // namespace

void benchNodeLists(ankerl::nanobench::Bench& bench)
{
    namespace nb = ankerl::nanobench;

    auto const& data = randomData();

    // --- doubly-linked: push_back x N (both allocate per node) ------------------
    bench.title("worse node lists vs std");
    bench.run("List<int> pushBack x4096",
              [&]
              {
                  List<int> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.pushBack(i);
                  }
                  nb::doNotOptimizeAway(l.size());
              });
    bench.run("std::list<int> push_back x4096",
              [&]
              {
                  std::list<int> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.push_back(i);
                  }
                  nb::doNotOptimizeAway(l.size());
              });
    bench.run("List<int,ThinAlloc> pushBack x4096 (thin alloc)",
              [&]
              {
                  List<int, ThinAlloc> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.pushBack(i);
                  }
                  nb::doNotOptimizeAway(l.size());
              });

    bench.run("eastl::list<int> push_back x4096",
              [&]
              {
                  eastl::list<int> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.push_back(i);
                  }
                  nb::doNotOptimizeAway(l.size());
              });

    // --- zero-heap inline pool vs heap std::list -------------------------------
    bench.run("FixedList<int,4096> pushBack x4096 (zero alloc)",
              [&]
              {
                  FixedList<int, kN> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.pushBack(i);
                  }
                  nb::doNotOptimizeAway(l.size());
              });

    // --- singly-linked: push_front x N -----------------------------------------
    bench.run("ForwardList<int> pushFront x4096",
              [&]
              {
                  ForwardList<int> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.pushFront(i);
                  }
                  nb::doNotOptimizeAway(l.size());
              });
    bench.run("std::forward_list<int> push_front x4096",
              [&]
              {
                  std::forward_list<int> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.push_front(i);
                  }
                  nb::doNotOptimizeAway(l.empty());
              });
    bench.run("eastl::slist<int> push_front x4096",
              [&]
              {
                  eastl::slist<int> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.push_front(i);
                  }
                  nb::doNotOptimizeAway(l.empty());
              });
    bench.run("FixedSList<int,4096> pushFront x4096 (zero alloc)",
              [&]
              {
                  FixedSList<int, kN> l;
                  for (int i = 0; i < kN; ++i)
                  {
                      l.pushFront(i);
                  }
                  nb::doNotOptimizeAway(l.size());
              });

    // --- iterate + sum (cache-chase cost) --------------------------------------
    {
        List<int> wl(data.data(), data.data() + kN);
        std::list<int> sl(data.begin(), data.end());
        bench.run("List<int> iterate sum (4096)",
                  [&]
                  {
                      i64 acc = 0;
                      for (int v : wl)
                      {
                          acc += v;
                      }
                      nb::doNotOptimizeAway(acc);
                  });
        bench.run("std::list<int> iterate sum (4096)",
                  [&]
                  {
                      i64 acc = 0;
                      for (int v : sl)
                      {
                          acc += v;
                      }
                      nb::doNotOptimizeAway(acc);
                  });
    }

    // --- sort 4096 pseudo-random (rebuild each iter; both pay it equally) -------
    bench.run("List<int> sort (4096 random)",
              [&]
              {
                  List<int> l(data.data(), data.data() + kN);
                  l.sort();
                  nb::doNotOptimizeAway(l.front());
              });
    bench.run("std::list<int> sort (4096 random)",
              [&]
              {
                  std::list<int> l(data.begin(), data.end());
                  l.sort();
                  nb::doNotOptimizeAway(l.front());
              });
    bench.run("eastl::list<int> sort (4096 random)",
              [&]
              {
                  eastl::list<int> l(data.begin(), data.end());
                  l.sort();
                  nb::doNotOptimizeAway(l.front());
              });
    bench.run("ForwardList<int> sort (4096 random)",
              [&]
              {
                  ForwardList<int> l(data.data(), data.data() + kN);
                  l.sort();
                  nb::doNotOptimizeAway(l.front());
              });
    bench.run("std::forward_list<int> sort (4096 random)",
              [&]
              {
                  std::forward_list<int> l(data.begin(), data.end());
                  l.sort();
                  nb::doNotOptimizeAway(l.front());
              });
    bench.run("eastl::slist<int> sort (4096 random)",
              [&]
              {
                  eastl::slist<int> l(data.begin(), data.end());
                  l.sort();
                  nb::doNotOptimizeAway(l.front());
              });
}
