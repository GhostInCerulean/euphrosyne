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