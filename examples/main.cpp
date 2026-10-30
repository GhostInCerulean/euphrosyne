#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <nvrhi/vulkan.h>
#include <nvrhi/validation.h>
#include <vector>
#include <cstdio>
#include <vulkan/vulkan_hpp_macros.hpp>

#include "vulkan/vulkan_handles.hpp"

struct Swap {
    vkb::Swapchain sc;
    std::vector<VkImage> images;
    std::vector<nvrhi::TextureHandle> textures;
    std::vector<nvrhi::FramebufferHandle> fbs;
};

static Swap createSwap(vkb::Device& dev, nvrhi::IDevice* nv, VkSwapchainKHR old, int w, int h)
{
    Swap s;
    s.sc = vkb::SwapchainBuilder{dev}
        .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(w, h)
        .set_old_swapchain(old)
        .build().value();
    s.images = s.sc.get_images().value();

    auto td = nvrhi::TextureDesc()
        .setDimension(nvrhi::TextureDimension::Texture2D)
        .setWidth(s.sc.extent.width).setHeight(s.sc.extent.height)
        .setFormat(nvrhi::Format::BGRA8_UNORM)
        .setIsRenderTarget(true)
        .setInitialState(nvrhi::ResourceStates::Present)
        .setKeepInitialState(true);

    for (VkImage img : s.images) {
        auto tex = nv->createHandleForNativeTexture(
            nvrhi::ObjectTypes::VK_Image, nvrhi::Object(img), td);
        s.textures.push_back(tex);
        s.fbs.push_back(nv->createFramebuffer(
            nvrhi::FramebufferDesc().addColorAttachment(tex)));
    }
    return s;
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow("nvrhi + sdl3", 1280, 720,
                                       SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    // --- Instance ---
    Uint32 n = 0;
    const char* const* sdlExts = SDL_Vulkan_GetInstanceExtensions(&n);
    std::vector<const char*> instExts(sdlExts, sdlExts + n);

    vkb::InstanceBuilder ib;
    ib.set_app_name("app").require_api_version(1, 3, 0)
      .request_validation_layers(true).use_default_debug_messenger();
    for (auto e : instExts) ib.enable_extension(e);
    vkb::Instance inst = ib.build().value();

    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(win, inst.instance, nullptr, &surface)) {
        std::printf("surface: %s\n", SDL_GetError());
        return 1;
    }

    // --- Device ---
    vkb::PhysicalDevice phys = vkb::PhysicalDeviceSelector{inst}
        .set_surface(surface).set_minimum_version(1, 3).select().value();
    vkb::Device dev = vkb::DeviceBuilder{phys}.build().value();

    VkQueue gfxQueue = dev.get_queue(vkb::QueueType::graphics).value();
    VkQueue presentQueue = dev.get_queue(vkb::QueueType::present).value();
    uint32_t gfxFamily = dev.get_queue_index(vkb::QueueType::graphics).value();

    // --- NVRHI ---
    VULKAN_HPP_DEFAULT_DISPATCHER.init(inst.fp_vkGetInstanceProcAddr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Instance(inst.instance));
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Device(dev.device));

    const char* devExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    nvrhi::vulkan::DeviceDesc dd;
    dd.instance = inst.instance;
    dd.physicalDevice = phys.physical_device;
    dd.device = dev.device;
    dd.graphicsQueue = gfxQueue;
    dd.graphicsQueueIndex = int(gfxFamily);
    dd.instanceExtensions = instExts.data();
    dd.numInstanceExtensions = instExts.size();
    dd.deviceExtensions = devExts;
    dd.numDeviceExtensions = 1;

    nvrhi::DeviceHandle nv = nvrhi::vulkan::createDevice(dd);
#ifndef NDEBUG
    nv = nvrhi::validation::createValidationLayer(nv);
#endif

    // --- Swapchain + sync ---
    int w, h;
    SDL_GetWindowSizeInPixels(win, &w, &h);
    Swap swap = createSwap(dev, nv, VK_NULL_HANDLE, w, h);

    VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkSemaphore acquireSem, presentSem;
    vkCreateSemaphore(dev.device, &sci, nullptr, &acquireSem);
    vkCreateSemaphore(dev.device, &sci, nullptr, &presentSem);

    nvrhi::CommandListHandle cmd = nv->createCommandList();

    // --- Loop ---
    bool running = true, resize = false;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) resize = true;
        }

        SDL_GetWindowSizeInPixels(win, &w, &h);
        if (w == 0 || h == 0) { SDL_Delay(10); continue; }   // minimized

        if (resize) {
            nv->waitForIdle();
            swap.fbs.clear(); swap.textures.clear();
            nv->runGarbageCollection();
            VkSwapchainKHR old = swap.sc.swapchain;
            swap = createSwap(dev, nv, old, w, h);
            vkb::destroy_swapchain(/* old swapchain */ vkb::Swapchain{.device = dev.device, .swapchain = old});
            resize = false;
        }

        uint32_t idx;
        VkResult r = vkAcquireNextImageKHR(dev.device, swap.sc.swapchain, UINT64_MAX,
                                           acquireSem, VK_NULL_HANDLE, &idx);
        if (r == VK_ERROR_OUT_OF_DATE_KHR) { resize = true; continue; }

        nv->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, acquireSem, 0);

        cmd->open();
        cmd->clearTextureFloat(swap.textures[idx], nvrhi::AllSubresources,
                               nvrhi::Color(0.1f, 0.2f, 0.4f, 1.f));
        cmd->close();
        nv->executeCommandList(cmd);

        nv->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, presentSem, 0);
        nv->executeCommandLists(nullptr, 0);          // flush the pending signal

        VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &presentSem;
        pi.swapchainCount = 1;     pi.pSwapchains = &swap.sc.swapchain;
        pi.pImageIndices = &idx;
        r = vkQueuePresentKHR(presentQueue, &pi);
        if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) resize = true;

        nv->waitForIdle();            // naive: replace with frames-in-flight later
        nv->runGarbageCollection();
    }

    // --- Cleanup (order matters) ---
    nv->waitForIdle();
    cmd = nullptr; swap.fbs.clear(); swap.textures.clear();
    nv->runGarbageCollection();
    nv = nullptr;
    vkDestroySemaphore(dev.device, acquireSem, nullptr);
    vkDestroySemaphore(dev.device, presentSem, nullptr);
    vkb::destroy_swapchain(swap.sc);
    vkb::destroy_device(dev);
    vkDestroySurfaceKHR(inst.instance, surface, nullptr);
    vkb::destroy_instance(inst);
    SDL_DestroyWindow(win);
    SDL_Quit();
}