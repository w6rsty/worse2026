cmake_minimum_required(VERSION 3.28)
project(worse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE WORSE_MODULES CONFIGURE_DEPENDS
    src/*.cppm)

file(GLOB_RECURSE WORSE_HEADERS CONFIGURE_DEPENDS
    src/*.hpp)

file(GLOB_RECURSE WORSE_SOURCES CONFIGURE_DEPENDS
    src/*.cpp)

add_library(${PROJECT_NAME})

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${PROJECT_NAME} PUBLIC WORSE_PLATFORM_WINDOWS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(${PROJECT_NAME} PUBLIC WORSE_PLATFORM_MACOS)
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    target_compile_definitions(${PROJECT_NAME} PUBLIC WE_ARCH_AMD64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    target_compile_definitions(${PROJECT_NAME} PUBLIC WE_ARCH_AARCH64)
endif()

target_sources(${PROJECT_NAME}
    PRIVATE
        ${WORSE_HEADERS}
        ${WORSE_SOURCES}
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
        FILES ${WORSE_MODULES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

option(WORSE_BUILD_TESTS "Build tests" ON)

if(WORSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Optional, off by default. Requires the vcpkg "bench" manifest feature for nanobench:
#   cmake --preset debug -DWORSE_BUILD_BENCH=ON -DVCPKG_MANIFEST_FEATURES=bench
option(WORSE_BUILD_BENCH "Build container benchmarks (nanobench)" OFF)

if(WORSE_BUILD_BENCH)
    add_subdirectory(bench)
endif()

# Optional, off by default. Generates HTML API docs with Doxygen (+ vendored Doxygen
# Awesome CSS under docs/doxygen-awesome-css/). Requires `doxygen` on PATH:
#   cmake --preset debug -DWORSE_BUILD_DOCS=ON
#   cmake --build build/debug --target docs   # output: docs/html/
option(WORSE_BUILD_DOCS "Build Doxygen documentation" OFF)

if(WORSE_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    else()
        message(WARNING "WORSE_BUILD_DOCS=ON but Doxygen was not found; the 'docs' target is unavailable. Install it (e.g. `brew install doxygen`).")
    endif()
endif()