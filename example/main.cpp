#include "euphrosyne/formats/archive.hpp"
#include "euphrosyne/rendering/gpu_context.hpp"
#include "euphrosyne/rendering/swapchain.hpp"
#include "euphrosyne/utils/platform/paths.hpp"

#include <SDL3/SDL.h>

int main() {
    auto window = SDL_CreateWindow("dial", 1280, 720, SDL_WINDOW_VULKAN);
    auto gpu = euphrosyne::GpuContext(window);
    auto sc = euphrosyne::Swapchain(gpu);

    auto event = SDL_Event();
    while (SDL_PollEvent(&event))
    {
        sc.Present(0);
    }
}