# Core subsystem benchmarks. By design a SINGLE binary: bench_main.cpp drives every suite
# against one shared nanobench::Bench so the relative-speed column compares across the whole
# run, and it is the sole owner of ANKERL_NANOBENCH_IMPLEMENT + EASTL's operator new[] hooks.
# The bench/ root is on the include path so suites share `benches.hpp`.
set(BENCH_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
file(GLOB_RECURSE CORE_BENCH_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

worse_add_bench(core
    SOURCES  ${BENCH_ROOT}/bench_main.cpp ${CORE_BENCH_SOURCES}
    LINK     worse::core nanobench::nanobench EASTL
    INCLUDES ${BENCH_ROOT})
