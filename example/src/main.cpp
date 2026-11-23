#include "SDL3/SDL_hints.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_oldnames.h"
#include "euphrosyne/rendering/renderer.hpp"
#include "example_game.hpp"

int main(int argc, char **argv)
{
    SDL_Init(SDL_INIT_VIDEO);
    auto window = SDL_CreateWindow("Dial", 1280, 720, SDL_WINDOW_RESIZABLE);
    auto renderer = euphrosyne::Renderer(window);
    auto running = true;

    while (running)
    {
        auto event = SDL_Event();
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                renderer.Resize();
                break;
            default:
                break;
            }
        }
    }
}