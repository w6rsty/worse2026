# Optional micro-benchmark target for the container library. Only configured when
# WORSE_BUILD_BENCH=ON (see the root CMakeLists), and nanobench is only fetched when the
# vcpkg manifest feature is active:
#
#   cmake --preset debug -DWORSE_BUILD_BENCH=ON -DVCPKG_MANIFEST_FEATURES=bench
#   cmake --build build/debug --target wcontainer_bench
#   ./build/debug/bench/wcontainer_bench
#
# The benches are built -O2 -DNDEBUG so the container TEMPLATES (instantiated in the
# bench TU) are optimized and the WE_ASSERT bounds checks compile out -- otherwise the
# numbers would reflect debug codegen. For end-to-end release numbers, also configure
# the library itself with a release preset.
find_package(nanobench CONFIG REQUIRED)

file(GLOB_RECURSE BENCH_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(wcontainer_bench ${BENCH_SOURCES})

target_link_libraries(wcontainer_bench
    PRIVATE
        worse
        nanobench::nanobench
)

target_compile_options(wcontainer_bench
    PRIVATE
        $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-O2>
)

target_compile_definitions(wcontainer_bench PRIVATE NDEBUG)
