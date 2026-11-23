#pragma once

#ifdef MAP_TYPE
#undef MAP_TYPE
#endif

#include "EngineFactoryVk.h"
#include "RefCntAutoPtr.hpp"
#include "SDL3/SDL_video.h"
#include "euphrosyne/dev/logging.hpp"

#include <euphrosyne/pch.hpp>

namespace euphrosyne
{

EUPHROSYNE_DECLARE_LOGGER(renderer);

class Renderer
{
    SDL_Window* window_;
    Diligent::IEngineFactory *engine_factory_;
    Diligent::IEngineFactoryVk *engine_factory_vk_;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> render_device_;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> device_context_;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swap_chain_;

  public:
    explicit Renderer(SDL_Window *window);
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void Resize();
    void BeginFrame(float4 color);
    void EndFrame();
};
} // namespace euphrosyne
