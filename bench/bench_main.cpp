// Single owner of the nanobench implementation + the benchmark entry point. Each suite
// lives in its own TU and is invoked here against one shared Bench so the relative-speed
// column compares across the whole run. Run: ./build/<preset>/bench/wcontainer_bench
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include "benches.hpp"

int main()
{
    namespace nb = ankerl::nanobench;

    nb::Bench bench;
    bench.relative(true).minEpochIterations(500);

    benchArrays(bench);
    benchNodeLists(bench);
    benchStdCompare(bench);

    return 0;
}
