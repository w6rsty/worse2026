# Factory functions that keep every library / test / bench definition to a single declarative
# call, so a new subsystem is "copy one worse_add_library() block and name its deps".
include_guard(GLOBAL)

# worse_add_library(<subsystem>
#     MODULES      <*.cppm ...>   # C++20 module interface units (the public module graph)
#     SOURCES      <*.cpp ...>    # implementation TUs                       (optional)
#     HEADERS      <*.hpp ...>    # private / textual headers                (optional)
#     PUBLIC_DEPS  <targets ...>  # propagated deps: consumers import their modules
#     PRIVATE_DEPS <targets ...>  # implementation-only deps
#     DEFINES      <defs ...>     # extra PUBLIC compile definitions for this library
#     OPTIONS      <opts ...>     # extra PRIVATE compile options for this library
# )
# Produces target `worse_<subsystem>` and alias `worse::<subsystem>`.
function(worse_add_library subsystem)
    cmake_parse_arguments(ARG ""
        "" "MODULES;SOURCES;HEADERS;PUBLIC_DEPS;PRIVATE_DEPS;DEFINES;OPTIONS" ${ARGN})

    set(target worse_${subsystem})
    add_library(${target} ${WORSE_LIB_TYPE})
    add_library(worse::${subsystem} ALIAS ${target})

    target_sources(${target}
        PRIVATE
            ${ARG_SOURCES}
            ${ARG_HEADERS}
        PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${WORSE_SOURCE_DIR}
            FILES ${ARG_MODULES})

    target_include_directories(${target}
        PUBLIC $<BUILD_INTERFACE:${WORSE_SOURCE_DIR}>)

    target_link_libraries(${target}
        PUBLIC  worse::build_options ${ARG_PUBLIC_DEPS}
        PRIVATE worse::warnings      ${ARG_PRIVATE_DEPS})

    if(ARG_DEFINES)
        target_compile_definitions(${target} PUBLIC ${ARG_DEFINES})
    endif()
    if(ARG_OPTIONS)
        target_compile_options(${target} PRIVATE ${ARG_OPTIONS})
    endif()
endfunction()

# worse_add_test(<suite> SOURCES <*.cpp ...> LINK <targets ...>)
# One executable per suite -> independent incremental builds; ctest entries prefixed "<suite>.".
function(worse_add_test suite)
    cmake_parse_arguments(ARG "" "" "SOURCES;LINK" ${ARGN})
    if(NOT ARG_SOURCES)
        return()
    endif()

    set(target test_${suite})
    add_executable(${target} ${ARG_SOURCES})
    target_link_libraries(${target}
        PRIVATE ${ARG_LINK} GTest::gtest_main worse::warnings)
    gtest_discover_tests(${target} TEST_PREFIX "${suite}.")
endfunction()

# worse_add_bench(<suite> SOURCES <*.cpp ...> LINK <targets ...> [INCLUDES <dirs ...>] [DEFINES ...])
# Benches build -O2 -DNDEBUG so the template-heavy engine code is optimized and WE_ASSERT bounds
# checks compile out -- otherwise the numbers reflect debug codegen.
function(worse_add_bench suite)
    cmake_parse_arguments(ARG "" "" "SOURCES;LINK;INCLUDES;DEFINES" ${ARGN})
    if(NOT ARG_SOURCES)
        return()
    endif()

    set(target bench_${suite})
    add_executable(${target} ${ARG_SOURCES})

    if(ARG_INCLUDES)
        target_include_directories(${target} PRIVATE ${ARG_INCLUDES})
    endif()

    target_link_libraries(${target} PRIVATE ${ARG_LINK})
    target_compile_definitions(${target} PRIVATE NDEBUG ${ARG_DEFINES})
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-O2>
        $<$<CXX_COMPILER_ID:MSVC>:/O2>)
endfunction()
