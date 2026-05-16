cmake_minimum_required(VERSION 3.28)
project(wcoro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE WORSE_MODULES CONFIGURE_DEPENDS
    src/*.cppm)

file(GLOB_RECURSE WORSE_SOURCES CONFIGURE_DEPENDS
    src/*.cpp)

add_library(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${WORSE_SOURCES}
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
        FILES ${WORSE_MODULES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

option(WORSE_BUILD_TESTS "Build tests" ON)

if(WORSE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()