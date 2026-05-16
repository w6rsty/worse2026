find_package(GTest CONFIG REQUIRED)

file(GLOB_RECURSE TEST_SOURCES *.cpp)

add_executable(wcoro_tests ${TEST_SOURCES})

target_link_libraries(wcoro_tests
    PRIVATE
        wcoro
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(wcoro_tests)
