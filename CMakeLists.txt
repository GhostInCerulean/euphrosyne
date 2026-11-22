cmake_minimum_required(VERSION 3.22)
project(euphrosyne)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

option(EUPHROSYNE_BUILD_TESTS "Build tests" ON)
option(EUPHROSYNE_BUILD_EXAMPLE "Build the example" ON)
option(EUPHROSYNE_BUILD_BENCHMARKS "Build benchmarks" ON)

option(EUPHROSYNE_ENABLE_AVX2 "Compile everything for AVX2 + FMA CPUs" ON)

# Handle AVX2
add_library(euphrosyne_cpu_flags INTERFACE)
if (EUPHROSYNE_ENABLE_AVX2 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|x64")
    if (MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(euphrosyne_cpu_flags INTERFACE $<$<COMPILE_LANGUAGE:C,CXX>:/arch:AVX2>)
    else ()
        target_compile_options(euphrosyne_cpu_flags INTERFACE
                $<$<COMPILE_LANGUAGE:C,CXX>:-mavx2>
                $<$<COMPILE_LANGUAGE:C,CXX>:-mfma>
                $<$<COMPILE_LANGUAGE:C,CXX>:-mf16c>
                $<$<COMPILE_LANGUAGE:C,CXX>:-mbmi2>)
    endif ()
endif ()

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