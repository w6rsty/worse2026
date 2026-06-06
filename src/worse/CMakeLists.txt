# Engine source root. Each subdirectory here is one subsystem library (core today; RHI /
# render / ... as siblings later). The `worse` target is a pure facade: it owns no sources,
# it just aggregates the enabled subsystems so an application links a single `worse::worse`
# and imports `worse.core` / `worse.<...>` directly.

find_package(glfw3 CONFIG REQUIRED)

add_subdirectory(core)
add_subdirectory(application)

# --- worse: top-level engine facade (INTERFACE — aggregates subsystems, compiles nothing) ---
add_library(worse INTERFACE)
add_library(worse::worse ALIAS worse)

target_link_libraries(worse INTERFACE
    worse::core
    worse::application
)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE worse)

# `app` is hand-rolled (not via worse_add_test/_bench), so it must stage its own runtime deps.
# In a shared build (e.g. the user-debug preset, WORSE_BUILD_SHARED=ON) app.exe depends on
# worse_core.dll / worse_application.dll / glfw3.dll, none of which sit next to the exe by
# default — so launching it (e.g. via the VSCode CMake Tools "launch" button) fails with
# "glfw3.dll not found". Copy every dependent DLL next to app.exe, mirroring the test/bench
# helpers. Both calls are no-ops outside their config (static build / non-sanitizer).
_worse_stage_runtime_dlls(app)
_worse_stage_sanitizer_runtime(app)