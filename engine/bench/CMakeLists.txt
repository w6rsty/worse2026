# Micro-benchmarks. nanobench + EASTL come from the vcpkg "bench" manifest feature:
#   cmake --preset debug -DWORSE_BUILD_BENCH=ON -DVCPKG_MANIFEST_FEATURES=bench
#   cmake --build build/debug --target bench_core
#   ./build/debug/bench/bench_core
find_package(nanobench CONFIG REQUIRED)
find_package(EASTL CONFIG REQUIRED) # 3-way comparison target (worse vs std vs EASTL)

if(WORSE_BUILD_BENCH_CORE AND TARGET worse::core)
    add_subdirectory(core)
endif()
