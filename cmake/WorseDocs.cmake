# Doxygen HTML docs (vendored Doxygen Awesome CSS under docs/doxygen-awesome-css/).
#   cmake --preset debug -DWORSE_BUILD_DOCS=ON
#   cmake --build build/debug --target docs   # output: docs/html/
include_guard(GLOBAL)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
else()
    message(WARNING
        "WORSE_BUILD_DOCS=ON but Doxygen was not found; the 'docs' target is unavailable. "
        "Install it (e.g. `brew install doxygen`).")
endif()
