// Single owner of the nanobench implementation + the benchmark entry point. Each suite
// lives in its own TU and is invoked here against one shared Bench so the relative-speed
// column compares across the whole run. Run: ./build/<preset>/bench/wcontainer_bench
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include "benches.hpp"

#include <cstddef>
#include <new>

// EASTL requires the application to supply these two operator new[] overloads (it never calls
// the standard ones). Forward them to global new so EASTL containers allocate normally.
void* operator new[](std::size_t size, char const*, int, unsigned, char const*, int)
{
    return ::operator new(size);
}
void* operator new[](
    std::size_t size, std::size_t alignment, std::size_t, char const*, int, unsigned, char const*, int)
{
    return ::operator new(size, std::align_val_t(alignment));
}

int main()
{
    namespace nb = ankerl::nanobench;

    nb::Bench bench;
    bench.relative(true).minEpochIterations(500);

    benchArrays(bench);
    benchNodeLists(bench);
    benchStdCompare(bench);
    benchAlgorithms(bench);

    return 0;
}
