set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FETCHCONTENT_QUIET OFF)

CPMAddPackage(
        NAME nvrhi
        GITHUB_REPOSITORY NVIDIA-RTX/NVRHI
        GIT_TAG bf0bbd9b666a7297492beb1999a4ddc0dfded143
        OPTIONS
            "NVRHI_WITH_VULKAN ON"
)
CPMAddPackage("gh:SanderMertens/flecs@4.1.6")
CPMAddPackage("gh:charles-lunarg/vk-bootstrap@1.4.366")
CPMAddPackage(
        NAME Boost
        VERSION 1.92.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-cmake.tar.xz
        URL_HASH SHA256=9bed76128d4e46755dbe818487788c6fceb6f72b378f4daa49b7e1e600d9088d
)
CPMAddPackage(
        NAME CLI11
        VERSION 1.92.0
        URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.7.2.tar.gz
        OPTIONS
            "CLI11_PRECOMPILED ON"
)
CPMAddPackage(
        NAME              SDL3
        GITHUB_REPOSITORY libsdl-org/SDL
        GIT_TAG           release-3.4.18
        OPTIONS
            "SDL_SHARED OFF"
            "SDL_STATIC ON"
            "SDL_TEST_LIBRARY OFF"
            "SDL_TESTS OFF"
            "SDL_INSTALL OFF"
)

CPMAddPackage("gh:stephenberry/glaze@9.1.0")
CPMAddPackage("gh:tcbrindle/flux#307be642aad317c4d13427950c1822587b10edbf")