find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

# One test subdirectory per subsystem; each is independently gated (modular selection).
if(WORSE_BUILD_TESTS_CORE AND TARGET worse::core)
    add_subdirectory(core)
endif()
