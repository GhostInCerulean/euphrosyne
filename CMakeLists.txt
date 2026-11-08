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

option(EUPHROSYNE_ENABLE_AVX2 "Compile everything for AVX2 + FMA CPUs" ON)

if (EUPHROSYNE_ENABLE_AVX2 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|x64")
    if (MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/arch:AVX2>)
    else ()
        add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-mavx2>
                $<$<COMPILE_LANGUAGE:C,CXX>:-mfma>
                $<$<COMPILE_LANGUAGE:C,CXX>:-mf16c>
                $<$<COMPILE_LANGUAGE:C,CXX>:-mbmi2>)
    endif ()
endif ()

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