// NVRHI + Vulkan + SDL3: opens a window and draws a triangle.

// vulkan.hpp dynamic dispatch storage -- must appear in exactly one .cpp
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

#include <nvrhi/nvrhi.h>
#include <nvrhi/vulkan.h>
#include <nvrhi/validation.h>

#include <cstdint>
#include <cstdlib>
#include <vector>

#include "triangle.vert.h"   // const uint32_t g_triangle_vert[]
#include "triangle.frag.h"   // const uint32_t g_triangle_frag[]

#ifdef NDEBUG
constexpr bool kDebug = false;
#else
constexpr bool kDebug = true;
#endif

#define FATAL(...) do { SDL_Log("FATAL: " __VA_ARGS__); std::exit(1); } while (0)

template <class T>
static T must(vkb::Result<T> r, const char* what) {
    if (!r) { SDL_Log("FATAL: %s: %s", what, r.error().message().c_str()); std::exit(1); }
    return r.value();
}

struct MessageCallback : nvrhi::IMessageCallback {
    void message(nvrhi::MessageSeverity severity, const char* text) override {
        switch (severity) {
            case nvrhi::MessageSeverity::Info:    SDL_Log("[nvrhi] %s", text); break;
            case nvrhi::MessageSeverity::Warning: SDL_Log("[nvrhi][warn] %s", text); break;
            default:                              SDL_Log("[nvrhi][error] %s", text); break;
        }
    }
};

static nvrhi::Format toNvrhiFormat(VkFormat f) {
    switch (f) {
        case VK_FORMAT_B8G8R8A8_UNORM: return nvrhi::Format::BGRA8_UNORM;
        case VK_FORMAT_R8G8B8A8_UNORM: return nvrhi::Format::RGBA8_UNORM;
        default: FATAL("unsupported swapchain format %d", (int)f);
    }
}

int main() {
    // ---------------------------------------------------------------- SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) FATAL("SDL_Init: %s", SDL_GetError());
    if (!SDL_Vulkan_LoadLibrary(nullptr)) FATAL("SDL_Vulkan_LoadLibrary: %s", SDL_GetError());

    SDL_Window* window = SDL_CreateWindow("NVRHI + Vulkan + SDL3", 1280, 720,
                                          SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) FATAL("SDL_CreateWindow: %s", SDL_GetError());

    // ---------------------------------------------------- Vulkan instance
    Uint32 sdlExtCount = 0;
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&sdlExtCount);
    if (!sdlExts) FATAL("SDL_Vulkan_GetInstanceExtensions: %s", SDL_GetError());

    std::vector<const char*> instanceExts(sdlExts, sdlExts + sdlExtCount);

    vkb::InstanceBuilder ib;
    ib.set_app_name("nvrhi-sdl3-vulkan").require_api_version(1, 2, 0);
    for (const char* e : instanceExts) ib.enable_extension(e);
    if (kDebug) {
        ib.request_validation_layers().use_default_debug_messenger();
        instanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);  // vk-bootstrap enables it; tell NVRHI
    }
    vkb::Instance vkbInst = must(ib.build(), "create instance");

    // vulkan.hpp dynamic dispatcher
    VULKAN_HPP_DEFAULT_DISPATCHER.init((PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr());
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Instance(vkbInst.instance));

    // ------------------------------------------------------------ Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window, vkbInst.instance, nullptr, &surface))
        FATAL("SDL_Vulkan_CreateSurface: %s", SDL_GetError());

    // ------------------------------------------------------------- Device
    std::vector<const char*> deviceExts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vkb::PhysicalDeviceSelector selector(vkbInst);
    vkb::PhysicalDevice vkbPhys = must(
        selector.set_surface(surface).add_required_extensions(deviceExts).select(),
        "select physical device");
    SDL_Log("GPU: %s", vkbPhys.name.c_str());

    vkb::Device vkbDev = must(vkb::DeviceBuilder(vkbPhys).build(), "create device");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Device(vkbDev.device));

    VkQueue  gfxQueue = must(vkbDev.get_queue(vkb::QueueType::graphics), "graphics queue");
    uint32_t gfxIndex = must(vkbDev.get_queue_index(vkb::QueueType::graphics), "graphics queue index");

    // -------------------------------------------------------------- NVRHI
    MessageCallback messageCb;

    nvrhi::vulkan::DeviceDesc dd;
    dd.errorCB                = &messageCb;
    dd.instance               = vk::Instance(vkbInst.instance);
    dd.physicalDevice         = vk::PhysicalDevice(vkbPhys.physical_device);
    dd.device                 = vk::Device(vkbDev.device);
    dd.graphicsQueue          = vk::Queue(gfxQueue);
    dd.graphicsQueueIndex     = (int)gfxIndex;
    dd.instanceExtensions     = instanceExts.data();
    dd.numInstanceExtensions  = instanceExts.size();
    dd.deviceExtensions       = deviceExts.data();
    dd.numDeviceExtensions    = deviceExts.size();

    // Keep the Vulkan-specific handle: semaphore interop lives on it, not on the
    // generic nvrhi::IDevice (and not on the validation wrapper).
    nvrhi::vulkan::DeviceHandle vkDevice = nvrhi::vulkan::createDevice(dd);
    nvrhi::DeviceHandle device = kDebug ? nvrhi::validation::createValidationLayer(vkDevice)
                                        : nvrhi::DeviceHandle(vkDevice);

    // ---------------------------------------------------------- Swapchain
    auto& d = VULKAN_HPP_DEFAULT_DISPATCHER;   // raw vk* entry points

    struct Backbuffers {
        vkb::Swapchain                         sc{};
        std::vector<nvrhi::TextureHandle>      textures;
        std::vector<nvrhi::FramebufferHandle>  framebuffers;
        std::vector<VkSemaphore>               presentSems;  // one per swapchain image
    } bb;

    auto destroyBackbuffers = [&] {
        device->waitForIdle();
        bb.framebuffers.clear();
        bb.textures.clear();
        device->runGarbageCollection();
        for (VkSemaphore s : bb.presentSems) d.vkDestroySemaphore(vkbDev.device, s, nullptr);
        bb.presentSems.clear();
    };

    // Returns false if the window currently has zero size (minimized).
    auto createBackbuffers = [&]() -> bool {
        int w = 0, h = 0;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        if (w == 0 || h == 0) return false;

        vkb::Swapchain old = bb.sc;
        vkb::SwapchainBuilder sb(vkbDev);
        sb.set_desired_format({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
          .add_fallback_format({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)  // needed for clearTexture
          .set_desired_extent((uint32_t)w, (uint32_t)h);
        if (old.swapchain != VK_NULL_HANDLE) sb.set_old_swapchain(old);

        bb.sc = must(sb.build(), "create swapchain");
        if (old.swapchain != VK_NULL_HANDLE) vkb::destroy_swapchain(old);

        std::vector<VkImage> images = must(bb.sc.get_images(), "get swapchain images");
        for (VkImage img : images) {
            nvrhi::TextureDesc td;
            td.width           = bb.sc.extent.width;
            td.height          = bb.sc.extent.height;
            td.format          = toNvrhiFormat(bb.sc.image_format);
            td.debugName       = "Swapchain image";
            td.isRenderTarget  = true;
            td.initialState    = nvrhi::ResourceStates::Present;
            td.keepInitialState = true;  // NVRHI transitions back to Present after use

            nvrhi::TextureHandle tex = device->createHandleForNativeTexture(
                nvrhi::ObjectTypes::VK_Image, nvrhi::Object(img), td);
            bb.textures.push_back(tex);
            bb.framebuffers.push_back(device->createFramebuffer(
                nvrhi::FramebufferDesc().addColorAttachment(tex)));

            VkSemaphoreCreateInfo sci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
            VkSemaphore sem = VK_NULL_HANDLE;
            d.vkCreateSemaphore(vkbDev.device, &sci, nullptr, &sem);
            bb.presentSems.push_back(sem);
        }
        return true;
    };

    bool haveBackbuffers = createBackbuffers();
    if (!haveBackbuffers) FATAL("window has zero size at startup");

    // -------------------------------------------------- Shaders + pipeline
    nvrhi::ShaderDesc vsd;
    vsd.shaderType = nvrhi::ShaderType::Vertex;
    vsd.entryName  = "main";
    vsd.debugName  = "triangle.vert";
    nvrhi::ShaderHandle vs = device->createShader(vsd, g_triangle_vert, sizeof(g_triangle_vert));

    nvrhi::ShaderDesc psd;
    psd.shaderType = nvrhi::ShaderType::Pixel;
    psd.entryName  = "main";
    psd.debugName  = "triangle.frag";
    nvrhi::ShaderHandle ps = device->createShader(psd, g_triangle_frag, sizeof(g_triangle_frag));

    nvrhi::GraphicsPipelineDesc pd;
    pd.VS       = vs;
    pd.PS       = ps;
    pd.primType = nvrhi::PrimitiveType::TriangleList;
    pd.renderState.depthStencilState.depthTestEnable = false;
    pd.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    // No input layout (vertices come from gl_VertexIndex) and no binding layouts.
    nvrhi::GraphicsPipelineHandle pipeline =
        device->createGraphicsPipeline(pd, bb.framebuffers[0]->getFramebufferInfo());

    nvrhi::CommandListHandle cmd = device->createCommandList();

    // Semaphore handed to NVRHI's queue so rendering waits for image acquisition.
    VkSemaphore acquireSem = VK_NULL_HANDLE;
    { VkSemaphoreCreateInfo sci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
      d.vkCreateSemaphore(vkbDev.device, &sci, nullptr, &acquireSem); }

    // ---------------------------------------------------------- Main loop
    bool running = true;
    bool needRecreate = false;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
            if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) needRecreate = true;
        }

        if (needRecreate || !haveBackbuffers) {
            destroyBackbuffers();
            haveBackbuffers = createBackbuffers();   // false while minimized
            needRecreate = false;
            if (!haveBackbuffers) { SDL_Delay(16); continue; }
        }

        // 1) acquire
        uint32_t idx = 0;
        VkResult r = d.vkAcquireNextImageKHR(vkbDev.device, bb.sc.swapchain, UINT64_MAX,
                                             acquireSem, VK_NULL_HANDLE, &idx);
        if (r == VK_ERROR_OUT_OF_DATE_KHR) { needRecreate = true; continue; }
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) FATAL("vkAcquireNextImageKHR failed (%d)", (int)r);
        if (r == VK_SUBOPTIMAL_KHR) needRecreate = true;   // rebuild after this frame

        // 2) record + submit; the submit waits on acquireSem
        vkDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, vk::Semaphore(acquireSem), 0);

        nvrhi::IFramebuffer* fb = bb.framebuffers[idx];
        const auto& fbInfo = fb->getFramebufferInfo();

        cmd->open();
        cmd->clearTextureFloat(bb.textures[idx], nvrhi::AllSubresources,
                               nvrhi::Color(0.08f, 0.10f, 0.16f, 1.0f));

        nvrhi::GraphicsState gs;
        gs.pipeline    = pipeline;
        gs.framebuffer = fb;
        gs.viewport.addViewportAndScissorRect(
            nvrhi::Viewport((float)fbInfo.width, (float)fbInfo.height));
        cmd->setGraphicsState(gs);

        nvrhi::DrawArguments args;
        args.vertexCount = 3;
        cmd->draw(args);
        cmd->close();

        device->executeCommandList(cmd);

        // 3) signal presentSem after that work, flush the pending signal, present
        vkDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, vk::Semaphore(bb.presentSems[idx]), 0);
        vkDevice->executeCommandLists(nullptr, 0);

        VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores    = &bb.presentSems[idx];
        pi.swapchainCount     = 1;
        pi.pSwapchains        = &bb.sc.swapchain;
        pi.pImageIndices      = &idx;
        r = d.vkQueuePresentKHR(gfxQueue, &pi);
        if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) needRecreate = true;
        else if (r != VK_SUCCESS) FATAL("vkQueuePresentKHR failed (%d)", (int)r);

        // Simple and safe: one frame in flight. Replace with a fence/semaphore ring later.
        device->waitForIdle();
        device->runGarbageCollection();
    }

    // ------------------------------------------------------------ Cleanup
    device->waitForIdle();
    destroyBackbuffers();
    d.vkDestroySemaphore(vkbDev.device, acquireSem, nullptr);

    cmd = nullptr; pipeline = nullptr; vs = nullptr; ps = nullptr;
    device->runGarbageCollection();
    device = nullptr;
    vkDevice = nullptr;

    vkb::destroy_swapchain(bb.sc);
    vkb::destroy_device(vkbDev);
    SDL_Vulkan_DestroySurface(vkbInst.instance, surface, nullptr);
    vkb::destroy_instance(vkbInst);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}