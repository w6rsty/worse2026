# Central registry of every build-system option. Convention: build-system identifiers are
# WORSE_*; the C++ preprocessor symbols they map to are WE_* (see WorseCompileOptions.cmake).
include_guard(GLOBAL)

# --- Build products -------------------------------------------------------
option(WORSE_BUILD_TESTS  "Build unit tests"                 ${PROJECT_IS_TOP_LEVEL})
option(WORSE_BUILD_BENCH  "Build micro-benchmarks"           OFF)
option(WORSE_BUILD_DOCS   "Build Doxygen documentation"      OFF)
option(WORSE_BUILD_SHARED "Build engine libraries as shared" OFF)

# --- Subsystem selection (grows with the engine: RHI / RENDER / ...) ------
option(WORSE_BUILD_CORE "Build the core subsystem" ON)

# --- Per-subsystem test / bench gates (modular selection) -----------------
option(WORSE_BUILD_TESTS_CORE "Build core tests"   ON)
option(WORSE_BUILD_BENCH_CORE "Build core benches" ON)

# --- Toolchain quality gates ----------------------------------------------
option(WORSE_ENABLE_WERROR     "Treat warnings as errors"           OFF)
option(WORSE_ENABLE_LTO        "Enable IPO/LTO for release configs"  OFF)
option(WORSE_ENABLE_SANITIZERS "Enable ASan+UBSan (Debug configs)"   OFF)

# --- Engine behaviour switches -> WE_* compile definitions ----------------
option(WORSE_ENABLE_ASSERTS    "Force WE_ASSERT on regardless of build type" OFF)
option(WORSE_FORCE_SCALAR_SIMD "Disable SSE/NEON, use scalar SIMD"           OFF)

# Static vs shared linkage resolved once, consumed by worse_add_library().
if(WORSE_BUILD_SHARED)
    set(WORSE_LIB_TYPE SHARED)
else()
    set(WORSE_LIB_TYPE STATIC)
endif()
