set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FETCHCONTENT_QUIET OFF)

CPMAddPackage(
        NAME nvrhi
        URL https://github.com/NVIDIA-RTX/NVRHI/archive/bf0bbd9b666a7297492beb1999a4ddc0dfded143.tar.gz
        OPTIONS
        "NVRHI_WITH_VULKAN ON"
        "NVRHI_BUILD_SHARED OFF"
)
CPMAddPackage(
        NAME flecs
        VERSION 4.1.6
        URL https://github.com/SanderMertens/flecs/archive/refs/tags/v4.1.6.tar.gz
        OPTIONS
        "FLECS_SHARED ON"
        "FLECS_STATIC OFF"
)
CPMAddPackage(
        NAME vk-bootstrap
        VERSION 1.4.366
        URL https://github.com/charles-lunarg/vk-bootstrap/archive/refs/tags/v1.4.366.tar.gz
)
CPMAddPackage(
        NAME Boost
        VERSION 1.92.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-cmake.tar.gz
)
CPMAddPackage(
        NAME CLI11
        VERSION 2.7.2
        URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.7.2.tar.gz
        OPTIONS
        "CLI11_PRECOMPILED ON"
)
CPMAddPackage(
        NAME SDL3
        VERSION 3.4.18
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.18.tar.gz
        OPTIONS
        "SDL_SHARED ON"
        "SDL_STATIC OFF"
        "SDL_TEST_LIBRARY OFF"
        "SDL_TESTS OFF"
        "SDL_INSTALL OFF"
)
CPMAddPackage(
        NAME glaze
        VERSION 9.1.0
        URL https://github.com/stephenberry/glaze/archive/refs/tags/v9.1.0.tar.gz
)
CPMAddPackage(
        NAME flux
        URL https://github.com/tcbrindle/flux/archive/307be642aad317c4d13427950c1822587b10edbf.tar.gz
)
CPMAddPackage(
        NAME quill
        VERSION 13.1.0
        URL https://github.com/odygrd/quill/archive/refs/tags/v13.1.0.tar.gz
)
CPMAddPackage(
        NAME soloud
        URL https://github.com/jarikomppa/soloud/archive/e82fd32c1f62183922f08c14c814a02b58db1873.tar.gz
        DOWNLOAD_ONLY YES
)
CPMAddPackage(
        NAME dylib
        VERSION 3.0.1
        URL https://github.com/martin-olivier/dylib/archive/refs/tags/v3.0.1.tar.gz
)
CPMAddPackage(
        NAME hlslpp
        VERSION 3.9
        URL https://github.com/redorav/hlslpp/archive/refs/tags/3.9.tar.gz
        DOWNLOAD_ONLY YES
)

if (hlslpp_ADDED AND NOT TARGET hlslpp::hlslpp)
    add_library(hlslpp INTERFACE)
    add_library(hlslpp::hlslpp ALIAS hlslpp)
    target_include_directories(hlslpp SYSTEM INTERFACE ${hlslpp_SOURCE_DIR}/include)
endif ()
CPMAddPackage(
        NAME googletest
        VERSION 1.15.2
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
CPMAddPackage(
        NAME benchmark
        VERSION 1.10.0
        URL https://github.com/google/benchmark/archive/refs/tags/v1.10.0.tar.gz
)
CPMAddPackage(
        NAME glm
        VERSION 1.0.3
        URL https://github.com/g-truc/glm/archive/refs/tags/1.0.3.tar.gz
        OPTIONS
        "GLM_BUILD_TESTS OFF"
        "GLM_BUILD_LIBRARY OFF"
        "GLM_ENABLE_CXX_17 ON"
)

set(SOLOUD_STATIC ON CACHE BOOL "" FORCE)
set(SOLOUD_STATIC ON CACHE BOOL "" FORCE)
set(SOLOUD_BACKEND_SDL2 OFF CACHE BOOL "" FORCE)
set(SOLOUD_BACKEND_MINIAUDIO ON CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(${soloud_SOURCE_DIR}/contrib ${soloud_BINARY_DIR})