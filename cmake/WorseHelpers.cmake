# Factory functions that keep every library / test / bench definition to a single declarative
# call, so a new subsystem is "copy one worse_add_library() block and name its deps".
include_guard(GLOBAL)

# Stage the sanitizer runtime DLL(s) next to <target> (Windows/clang ASan) so the executable is
# self-contained. Added as a POST_BUILD step; call it BEFORE gtest_discover_tests so the DLL is
# in place when the build-time discovery run launches the exe. No-op everywhere else.
function(_worse_stage_sanitizer_runtime target)
    if(WORSE_SANITIZER_RUNTIME_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${WORSE_SANITIZER_RUNTIME_DLLS} $<TARGET_FILE_DIR:${target}>
            VERBATIM)
    endif()
endfunction()

# Stage the shared libraries <target> links against (worse_*.dll plus any shared deps) next to the
# executable so it runs from its own build dir. vcpkg's applocal step only deploys vcpkg DLLs, not
# our engine DLLs, so without this a SHARED build links fine but then fails at *runtime* -- and
# because gtest_discover_tests launches the exe at build time, that failure (STATUS_DLL_NOT_FOUND,
# 0xC0000135) surfaces as a build error. POST_BUILD + before discovery so the DLLs are in place.
# No-op for the static build (TARGET_RUNTIME_DLLS is empty / guard is false).
function(_worse_stage_runtime_dlls target)
    if(WORSE_BUILD_SHARED AND WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS)
    endif()
endfunction()

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

    # Shared build on Windows: non-inline functions defined in this library (e.g.
    # memory::handleAllocationFailure, out-of-line ctors) are ODR-used by templates that
    # instantiate in the *consumer*, so they must be exported from the DLL. Auto-generate the
    # export table from the object files instead of annotating every declaration with a
    # __declspec macro. NOTE: this covers functions only -- global *data* still needs
    # dllimport on the consumer side, which is why namespace constants are `inline constexpr`
    # (header-emitted, no import) rather than relying on this.
    if(WORSE_BUILD_SHARED AND WIN32)
        set_target_properties(${target} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()

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
    _worse_stage_sanitizer_runtime(${target})
    _worse_stage_runtime_dlls(${target})
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
    _worse_stage_sanitizer_runtime(${target})
    _worse_stage_runtime_dlls(${target})
endfunction()
