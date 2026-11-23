#include "renderer.hpp"

namespace euphrosyne
{
Diligent::NativeWindow GetNativeWindow(SDL_Window *window)
{
    auto props = SDL_GetWindowProperties(window);
    if (!props)
        throw runtime_error("Failed to get window properties");

    auto native_window = Diligent::NativeWindow();

#if defined(PLATFORM_LINUX)
    auto driver_raw = SDL_GetCurrentVideoDriver();
    if (!driver_raw)
        throw runtime_error("Failed to get current video driver");

    auto driver = string_view(driver_raw);

    if (driver == "x11")
    {
        native_window.WindowId = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        native_window.pDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        if (!native_window.WindowId || !native_window.pDisplay)
            throw runtime_error("Failed to get X11 native props");
    }
    else if (driver == "wayland")
    {
        native_window.pDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        native_window.pWaylandSurface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);

        if (!native_window.pDisplay || !native_window.pWaylandSurface)
            throw runtime_error("Failed to get Wayland native props");
    }
    else
    {
        throw runtime_error("Unsupported linux video driver");
    }
#elifdef PLATFORM_WIN32
    nw.hWnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!nw.hWnd)
        throw std::runtime_error("No HWND from SDL");
#else
#error "Add a NativeWindow path for the platform
#endif
}

static uint2 GetPixelSize(SDL_Window* window)
{
    auto w = int32();
    auto h = int32();
    if (!SDL_GetWindowSizeInPixels(window, &w, &h))
        throw runtime_error("Failed to get window size");
    return {w, h};
}

Renderer::Renderer(SDL_Window *window) : window_(window)
{
    auto factory = Diligent::GetEngineFactoryVk();
    if (!factory)
        throw runtime_error("Failed to get Vulkan engine factory");
    engine_factory_ = factory;
    engine_factory_vk_ = factory;

    auto engine_ci = Diligent::EngineVkCreateInfo();
#ifdef DILIGENT_DEBUG
    engine_ci.EnableValidation = true;
#endif
    factory->CreateDeviceAndContextsVk(engine_ci, &render_device_, &device_context_);
    if (!render_device_ || !device_context_)
        throw runtime_error("Failed to create device and context");

    auto swapchain_desc = Diligent::SwapChainDesc();
    auto pixel_size = GetPixelSize(window);
    swapchain_desc.Width = pixel_size.x;
    swapchain_desc.Height = pixel_size.y;
    swapchain_desc.ColorBufferFormat = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
    swapchain_desc.DepthBufferFormat = Diligent::TEX_FORMAT_D32_FLOAT;
    swapchain_desc.BufferCount = 3;

    factory->CreateSwapChainVk(render_device_, device_context_, swapchain_desc, GetNativeWindow(window), &swap_chain_);
    if (!swap_chain_)
        throw runtime_error("Failed to create swap chain");
}

Renderer::~Renderer()
{
    if (device_context_)
    {
        device_context_->Flush();
        device_context_->FinishFrame();
    }
    if (render_device_)
        render_device_->IdleGPU();
}

void Renderer::Resize()
{
    auto pixel_size = GetPixelSize(window_);
    swap_chain_->Resize(pixel_size.x, pixel_size.y);
}

void Renderer::BeginFrame(float4 color)
{
    auto ctx = device_context_.RawPtr();
    auto rtv = swap_chain_->GetCurrentBackBufferRTV();
    auto dsv = swap_chain_->GetDepthBufferDSV();

    ctx->SetRenderTargets(1, &rtv, dsv, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    ctx->ClearRenderTarget(rtv, &color.rgba, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    ctx->ClearDepthStencil(dsv, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void Renderer::EndFrame()
{
    swap_chain_->Present();
}
} // euphrosyne