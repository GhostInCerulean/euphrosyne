cmake_minimum_required(VERSION 3.22)
project(euphrosyne)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(EUPHROSYNE_BUILD_TESTS "Build tests" ON)
option(EUPHROSYNE_BUILD_EXAMPLE "Build the example" ON)
option(EUPHROSYNE_BUILD_BENCHMARKS "Build benchmarks" ON)

# option(EUPHROSYNE_ENABLE_AVX2 "Compile everything for AVX2 + FMA CPUs" ON)

# Cache CPM packages
#if(NOT DEFINED CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
#    if(WIN32)
#        set(CPM_SOURCE_CACHE "$ENV{USERPROFILE}/.cache/CPM" CACHE PATH "CPM source cache")
#    else()
#        set(CPM_SOURCE_CACHE "$ENV{HOME}/.cache/CPM" CACHE PATH "CPM source cache")
#    endif()
#endif()

include(cmake/CPM.cmake)
include(cmake/deps.cmake)
include(cmake/shaders.cmake)

add_subdirectory(euphrosyne)
add_subdirectory(cooker)

if (EUPHROSYNE_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

if (EUPHROSYNE_BUILD_EXAMPLE)
    add_subdirectory(example)
    add_subdirectory(example_mod)
endif ()

if (EUPHROSYNE_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif ()