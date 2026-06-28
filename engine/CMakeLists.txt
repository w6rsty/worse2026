cmake_minimum_required(VERSION 3.28)

project(worse
    VERSION 0.1.0
    DESCRIPTION "worse game engine"
    LANGUAGES CXX)

# --- Global language / build policy ---------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(WORSE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# --- Capability layer (order matters: options -> platform -> flags -> helpers)
include(WorseOptions)         # every option()
include(WorsePlatform)        # worse_platform_definitions()
include(WorseCompileOptions)  # worse::build_options / worse::warnings
include(WorseHelpers)         # worse_add_library / worse_add_test / worse_add_bench

# --- Libraries ------------------------------------------------------------
add_subdirectory(src/worse)

# --- Tests ----------------------------------------------------------------
if(WORSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# --- Benchmarks -----------------------------------------------------------
# Requires the vcpkg "bench" manifest feature (nanobench + EASTL):
#   cmake --preset debug -DWORSE_BUILD_BENCH=ON -DVCPKG_MANIFEST_FEATURES=bench
if(WORSE_BUILD_BENCH)
    add_subdirectory(bench)
endif()

# --- Documentation --------------------------------------------------------
if(WORSE_BUILD_DOCS)
    include(WorseDocs)
endif()
