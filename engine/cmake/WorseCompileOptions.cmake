# Two INTERFACE carrier libraries every engine library links:
#   worse::build_options  (PUBLIC)  -- propagating: std level, WE_* defines, sanitizer/LTO
#   worse::warnings       (PRIVATE) -- non-propagating: warning level, -Werror
# A library that needs extra symbols/flags just adds them on its own target on top of these.
include_guard(GLOBAL)

# === worse::build_options =================================================
add_library(worse_build_options INTERFACE)
add_library(worse::build_options ALIAS worse_build_options)

target_compile_features(worse_build_options INTERFACE cxx_std_20)

worse_platform_definitions(WORSE_PLATFORM_DEFS)
target_compile_definitions(worse_build_options INTERFACE
    ${WORSE_PLATFORM_DEFS}
    WE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    WE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    WE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    $<$<CONFIG:Debug>:WE_BUILD_DEBUG>
    $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:WE_BUILD_RELEASE>
    $<$<BOOL:${WORSE_ENABLE_ASSERTS}>:WE_ENABLE_ASSERTS>
    $<$<BOOL:${WORSE_FORCE_SCALAR_SIMD}>:WE_SIMD_FORCE_SCALAR>)

# Sanitizers need matching compile AND link flags. The WORSE_ENABLE_SANITIZERS option is the
# gate (no per-config gate): the `asan` preset drives a RelWithDebInfo build because Windows ASan
# is incompatible with the debug CRT (/MDd); RelWithDebInfo keeps /MD + line info.
if(WORSE_ENABLE_SANITIZERS)
    set(_worse_san -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_compile_options(worse_build_options INTERFACE
        $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:${_worse_san}>)
    target_link_options(worse_build_options INTERFACE
        $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:${_worse_san}>)

    # On Windows the clang ASan runtime is a DLL (no static variant) that must sit next to each
    # executable, or it dies at startup with 0xc0000135 (incl. gtest's build-time discovery run).
    # Locate it so worse_add_test/_bench can stage it; consumed in WorseHelpers.cmake.
    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-resource-dir
            OUTPUT_VARIABLE _worse_resdir
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        file(TO_CMAKE_PATH
            "${_worse_resdir}/lib/windows/clang_rt.asan_dynamic-x86_64.dll" _worse_asan_dll)
        if(EXISTS "${_worse_asan_dll}")
            set(WORSE_SANITIZER_RUNTIME_DLLS "${_worse_asan_dll}"
                CACHE INTERNAL "Sanitizer runtime DLLs to stage next to executables")
        else()
            message(WARNING
                "WORSE_ENABLE_SANITIZERS=ON but the ASan runtime DLL was not found at "
                "${_worse_asan_dll}; sanitized executables may fail to start (0xc0000135).")
        endif()
    endif()
endif()

# LTO/IPO: probe support once, enable for the optimized configs only.
if(WORSE_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _worse_ipo_ok OUTPUT _worse_ipo_msg)
    if(_worse_ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE        ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    else()
        message(WARNING "WORSE_ENABLE_LTO=ON but IPO is unsupported: ${_worse_ipo_msg}")
    endif()
endif()

# === worse::warnings ======================================================
add_library(worse_warnings INTERFACE)
add_library(worse::warnings ALIAS worse_warnings)

target_compile_options(worse_warnings INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4;/permissive->
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall;-Wextra;-Wpedantic>
    $<$<AND:$<BOOL:${WORSE_ENABLE_WERROR}>,$<CXX_COMPILER_ID:MSVC>>:/WX>
    $<$<AND:$<BOOL:${WORSE_ENABLE_WERROR}>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Werror>)
