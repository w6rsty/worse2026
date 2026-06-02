// Head-to-head vs the STL for the contiguous + hash + ordered containers, to validate the
// engine containers are at least as fast as std (the explicit perf goal). Implementation +
// main live in bench_main.cpp.
#include <nanobench.h>

#include "benches.hpp"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

import worse.core.basic_type;
import worse.core.container.array;
import worse.core.container.unordered_map;
import worse.core.container.unordered_set;
import worse.core.container.flat_map;

using namespace worse;
using namespace worse::core;
using namespace worse::core::container;

namespace
{
    constexpr int kN = 4096;

    std::vector<int> const& keys()
    {
        static std::vector<int> const k = []
        {
            std::vector<int> v;
            v.reserve(kN);
            u64 s = 0xD1B54A32D192ED03ull;
            for (int i = 0; i < kN; ++i)
            {
                s = s * 6364136223846793005ull + 1442695040888963407ull;
                v.push_back(static_cast<int>(s >> 33));
            }
            return v;
        }();
        return k;
    }
} // namespace

void benchStdCompare(ankerl::nanobench::Bench& bench)
{
    namespace nb  = ankerl::nanobench;
    auto const& k = keys();

    // --- contiguous: Array vs std::vector --------------------------------------
    // NOTE: for a TRIVIALLY-copyable element with reserve + exact count, libc++ vectorizes the
    // push loop to ~memcpy speed; our emplaceBack routes the store through
    // AllocatorTraits::construct (allocator passed by ref), which the optimizer can't prove
    // non-aliasing, so it doesn't vectorize -- std::vector wins this specific microcase. The
    // gap closes for realistic (larger / non-trivial) payloads, which are memory-bound. Tracked
    // as a future allocator-seam optimization; the container layout itself is identical to std.
    bench.title("worse vs std (contiguous / hash / ordered)");
    bench.run("Array<int> push x4096 (reserve)",
              [&]
              {
                  Array<int> a;
                  a.reserve(kN);
                  for (int i = 0; i < kN; ++i)
                  {
                      a.pushBack(i);
                  }
                  nb::doNotOptimizeAway(a.data());
              });
    bench.run("std::vector<int> push x4096 (reserve)",
              [&]
              {
                  std::vector<int> a;
                  a.reserve(kN);
                  for (int i = 0; i < kN; ++i)
                  {
                      a.push_back(i);
                  }
                  nb::doNotOptimizeAway(a.data());
              });

    // --- hash map: insert ------------------------------------------------------
    bench.run("UnorderedMap<int,int> insert x4096",
              [&]
              {
                  UnorderedMap<int, int> m;
                  for (int i = 0; i < kN; ++i)
                  {
                      m[k[static_cast<usize>(i)]] = i;
                  }
                  nb::doNotOptimizeAway(m.size());
              });
    bench.run("std::unordered_map<int,int> insert x4096",
              [&]
              {
                  std::unordered_map<int, int> m;
                  for (int i = 0; i < kN; ++i)
                  {
                      m[k[static_cast<usize>(i)]] = i;
                  }
                  nb::doNotOptimizeAway(m.size());
              });

    // --- hash map: successful lookups ------------------------------------------
    {
        UnorderedMap<int, int> wm;
        std::unordered_map<int, int> sm;
        for (int i = 0; i < kN; ++i)
        {
            wm[k[static_cast<usize>(i)]] = i;
            sm[k[static_cast<usize>(i)]] = i;
        }
        bench.run("UnorderedMap<int,int> lookup x4096",
                  [&]
                  {
                      i64 acc = 0;
                      for (int i = 0; i < kN; ++i)
                      {
                          acc += wm.at(k[static_cast<usize>(i)]);
                      }
                      nb::doNotOptimizeAway(acc);
                  });
        bench.run("std::unordered_map<int,int> lookup x4096",
                  [&]
                  {
                      i64 acc = 0;
                      for (int i = 0; i < kN; ++i)
                      {
                          acc += sm.at(k[static_cast<usize>(i)]);
                      }
                      nb::doNotOptimizeAway(acc);
                  });
    }

    // --- hash set: insert ------------------------------------------------------
    bench.run("UnorderedSet<int> insert x4096",
              [&]
              {
                  UnorderedSet<int> s;
                  for (int i = 0; i < kN; ++i)
                  {
                      s.insert(k[static_cast<usize>(i)]);
                  }
                  nb::doNotOptimizeAway(s.size());
              });
    bench.run("std::unordered_set<int> insert x4096",
              [&]
              {
                  std::unordered_set<int> s;
                  for (int i = 0; i < kN; ++i)
                  {
                      s.insert(k[static_cast<usize>(i)]);
                  }
                  nb::doNotOptimizeAway(s.size());
              });

    // --- ordered map: FlatMap (sorted array) vs std::map (rb-tree) -------------
    // Different data structures; the flat container should win lookups (cache) and lose huge
    // random inserts (O(n) shift) -- documents the trade, not a like-for-like.
    {
        FlatMap<int, int> wm;
        std::map<int, int> sm;
        for (int i = 0; i < kN; ++i)
        {
            wm[k[static_cast<usize>(i)]] = i;
            sm[k[static_cast<usize>(i)]] = i;
        }
        bench.run("FlatMap<int,int> lookup x4096",
                  [&]
                  {
                      i64 acc = 0;
                      for (int i = 0; i < kN; ++i)
                      {
                          acc += wm.at(k[static_cast<usize>(i)]);
                      }
                      nb::doNotOptimizeAway(acc);
                  });
        bench.run("std::map<int,int> lookup x4096",
                  [&]
                  {
                      i64 acc = 0;
                      for (int i = 0; i < kN; ++i)
                      {
                          acc += sm.at(k[static_cast<usize>(i)]);
                      }
                      nb::doNotOptimizeAway(acc);
                  });
    }
}
