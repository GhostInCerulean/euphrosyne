#pragma once
#include "VkBootstrap.h"
#include "nvrhi/nvrhi.h"
#include "nvrhi/vulkan.h"

#include <SDL3/SDL_video.h>

namespace euphrosyne
{

class GpuContext
{
    SDL_Window *window_;
    vkb::Instance instance_;
    VkSurfaceKHR surface_;
    vkb::PhysicalDevice physical_device_;
    vkb::Device vkb_device_;
    VkQueue graphics_queue_;
    nvrhi::vulkan::DeviceHandle vk_device_;
    nvrhi::DeviceHandle device_;

  public:
    explicit GpuContext(SDL_Window *window, bool validation = true);

    nvrhi::IDevice *Device() const;
    nvrhi::vulkan::IDevice *VulkanDevice() const;

    VkDevice VkDevice() const;
    VkSurfaceKHR Surface() const;
    VkQueue GraphicqQueue() const;
    vkb::Device &VkbDevice();

    void WaitIdle();
};

} // namespace euphrosyne
