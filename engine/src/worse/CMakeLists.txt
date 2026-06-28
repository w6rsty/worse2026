# Engine source root. Each subdirectory here is one subsystem library (core today; RHI /
# render / ... as siblings later). The `worse` target below is the top-level facade that
# aggregates the enabled subsystems so an application links a single `worse`.

if(WORSE_BUILD_CORE)
    add_subdirectory(core)
endif()

# --- worse: top-level engine facade (owns the `worse` module) -------------
add_library(worse ${WORSE_LIB_TYPE})
add_library(worse::worse ALIAS worse)

target_sources(worse
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${WORSE_SOURCE_DIR}
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/worse.cppm)

target_include_directories(worse
    PUBLIC $<BUILD_INTERFACE:${WORSE_SOURCE_DIR}>)

target_link_libraries(worse
    PUBLIC  worse::build_options
    PRIVATE worse::warnings)

# Aggregate whichever subsystems are enabled (PUBLIC so `import worse.core.*` flows through).
if(TARGET worse::core)
    target_link_libraries(worse PUBLIC worse::core)
endif()
