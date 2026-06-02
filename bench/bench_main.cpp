// Single owner of the nanobench implementation + the benchmark entry point. Each suite
// lives in its own TU and is invoked here against one shared Bench so the relative-speed
// column compares across the whole run. Run: ./build/<preset>/bench/wcontainer_bench
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>

#include "benches.hpp"

#include <cstddef>
#include <cstdlib> // std::getenv   -- WCB_OUT machine-readable emit
#include <fstream> // std::ofstream  -- WCB_OUT machine-readable emit
#include <new>
#include <sstream> // std::i/ostringstream -- WCB_OUT per-suite CSV merge
#include <string>

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

    // Machine-readable emit for CI: when WCB_OUT is set, append each suite's rows as nanobench
    // CSV to that path. We MUST render after each suite -- Bench::title() does mResults.clear()
    // when the title changes, so the next suite wipes the prior one's rows; only the last suite
    // would survive to the end. Each render restarts the CSV header, so we keep the first header
    // and drop the rest. The CI compare keys on (title, name) since a few row names repeat across
    // suites (e.g. "Array<int> push x4096 (reserve)"). The human markdown table still prints to
    // stdout from nanobench as each suite completes; this only adds a file.
    std::ofstream csv;
    bool csvHeaderWritten = false;
    if (char const* out = std::getenv("WCB_OUT"))
    {
        csv.open(out);
    }
    auto dumpCsv = [&]
    {
        if (!csv.is_open())
        {
            return;
        }
        std::ostringstream rendered;
        nb::render(nb::templates::csv(), bench, rendered);
        std::istringstream lines(rendered.str());
        std::string line;
        bool isHeader = true;
        while (std::getline(lines, line))
        {
            if (isHeader)
            {
                isHeader = false;
                if (csvHeaderWritten)
                {
                    continue; // drop the repeated header from later suites
                }
                csvHeaderWritten = true;
            }
            csv << line << '\n';
        }
    };

    benchArrays(bench);
    dumpCsv();
    benchNodeLists(bench);
    dumpCsv();
    benchStdCompare(bench);
    dumpCsv();
    benchAlgorithms(bench);
    dumpCsv();

    return 0;
}
