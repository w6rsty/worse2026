# Core tests, split per sub-area into independent executables: editing one test only relinks
# that suite, not the whole core test surface. ctest entries are prefixed (e.g. "core_math.")
# so `ctest -R core_container` runs just that group.
file(GLOB CORE_MISC_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB CONTAINER_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/container/*.cpp)
file(GLOB ALGORITHM_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/algorithm/*.cpp)
file(GLOB MATH_TESTS      CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/math/*.cpp)

worse_add_test(core_container SOURCES ${CONTAINER_TESTS} LINK worse::core)
worse_add_test(core_algorithm SOURCES ${ALGORITHM_TESTS} LINK worse::core)
worse_add_test(core_math      SOURCES ${MATH_TESTS}      LINK worse::core)
worse_add_test(core           SOURCES ${CORE_MISC_TESTS} LINK worse::core)
