cmake_minimum_required(VERSION 3.22)
project(euphrosyne)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

option(EUPHROSYNE_BUILD_EUPHROSYNE "Build the engine (duh)" ON)
option(EUPHROSYNE_BUILD_COOKER "Build cooker" ON)

option(EUPHROSYNE_BUILD_TESTS "Build tests" ON)
option(EUPHROSYNE_BUILD_EXAMPLE "Build the example" ON)
option(EUPHROSYNE_BUILD_BENCHMARKS "Build benchmarks" ON)

include(cmake/CPM.cmake)
include(cmake/deps.cmake)
include(cmake/shaders.cmake)

if (EUPHROSYNE_BUILD_EUPHROSYNE)
    add_subdirectory(euphrosyne)
endif ()

if (EUPHROSYNE_BUILD_COOKER)
    add_subdirectory(cooker)
endif ()

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