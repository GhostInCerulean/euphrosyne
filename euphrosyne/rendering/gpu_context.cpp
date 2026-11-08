#include "gpu_context.hpp"

#include <SDL3/SDL_vulkan.h>

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_hpp_macros.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace euphrosyne
{
GpuContext::GpuContext(SDL_Window *window, bool validation) : window_(window)
{
    SDL_Vulkan_LoadLibrary(nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()));

    auto sdl_ext_count = uint32(0);
    auto sdl_exts_raw = SDL_Vulkan_GetInstanceExtensions(&sdl_ext_count);
    auto sdl_exts = vector(sdl_exts_raw, sdl_exts_raw + sdl_ext_count);

    auto instance_builder = vkb::InstanceBuilder();
    instance_builder.set_app_name("euphrosyne").require_api_version(1, 3, 0);
    if (validation)
        instance_builder.request_validation_layers().use_default_debug_messenger();

    for (const auto extension : sdl_exts)
    {
        instance_builder.enable_extension(extension);
    }

    instance_ = instance_builder.build().value();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Instance(instance_.instance));

    if (!SDL_Vulkan_CreateSurface(window_, instance_.instance, nullptr, &surface_))
        throw runtime_error("Failed to create surface");

    auto device_exts = vector<const char *>{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    auto vk12_features = VkPhysicalDeviceVulkan12Features();
    vk12_features.bufferDeviceAddress = VK_TRUE;
    vk12_features.descriptorIndexing = VK_TRUE;
    vk12_features.timelineSemaphore = VK_TRUE;

    auto selector = vkb::PhysicalDeviceSelector(instance_);
    selector.set_surface(surface_).set_minimum_version(1, 3).set_required_features_12(vk12_features);
    ;

    for (const auto ext : device_exts)
        selector.add_required_extension(ext);

    physical_device_ = selector.select().value();
    vkb_device_ = vkb::DeviceBuilder(physical_device_).build().value();

    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Device(vkb_device_.device));

    graphics_queue_ = vkb_device_.get_queue(vkb::QueueType::graphics).value();
    auto graphics_index = vkb_device_.get_queue_index(vkb::QueueType::graphics).value();

    auto device_desc = nvrhi::vulkan::DeviceDesc();
    device_desc.errorCB = nullptr; // todo make one
    device_desc.instance = instance_.instance;
    device_desc.physicalDevice = physical_device_.physical_device;
    device_desc.device = vkb_device_.device;
    device_desc.graphicsQueue = graphics_queue_;
    device_desc.graphicsQueueIndex = int32(graphics_index);

    device_desc.instanceExtensions = sdl_exts.data();
    device_desc.numInstanceExtensions = sdl_exts.size();

    device_desc.deviceExtensions = device_exts.data();
    device_desc.numDeviceExtensions = device_exts.size();

    device_desc.bufferDeviceAddressSupported = true;

    vk_device_ = nvrhi::vulkan::createDevice(device_desc);
    device_ = nvrhi::DeviceHandle(vk_device_.Get());
}

nvrhi::IDevice *GpuContext::Device() const
{
    return device_.Get();
}

nvrhi::vulkan::IDevice *GpuContext::VulkanDevice() const
{
    return vk_device_.Get();
}

VkDevice GpuContext::VkDevice() const
{
    return vkb_device_.device;
}

VkSurfaceKHR GpuContext::Surface() const
{
    return surface_;
}

VkQueue GpuContext::GraphicqQueue() const
{
    return graphics_queue_;
}

vkb::Device &GpuContext::VkbDevice()
{
    return vkb_device_;
}

void GpuContext::WaitIdle()
{
    device_->waitForIdle();
}
} // namespace euphrosyne