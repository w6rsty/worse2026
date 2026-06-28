# Host platform / architecture detection. Emits a list of WE_* preprocessor symbols that the
# worse::build_options interface target carries PUBLICly -- macros do NOT propagate through
# `import`, so every TU that preprocesses them (incl. consumers' global module fragments that
# #include "worse/core/macro.hpp") must receive these defines directly.
include_guard(GLOBAL)

function(worse_platform_definitions out_var)
    set(defs "")

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        list(APPEND defs WE_PLATFORM_WINDOWS)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        list(APPEND defs WE_PLATFORM_MACOS)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND defs WE_PLATFORM_LINUX)
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
        list(APPEND defs WE_ARCH_AMD64)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
        list(APPEND defs WE_ARCH_AARCH64)
    endif()

    set(${out_var} "${defs}" PARENT_SCOPE)
endfunction()
