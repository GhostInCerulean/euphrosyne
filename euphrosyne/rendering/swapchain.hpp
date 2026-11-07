#pragma once
#include "gpu_context.hpp"

namespace euphrosyne
{

class Swapchain
{
    GpuContext& ctx_;
    vkb::Swapchain swapchain_;
    vector<nvrhi::TextureHandle> textures_;
    vector<nvrhi::FramebufferHandle> framebuffers_;
    VkSemaphore acquire_sem_;
    VkSemaphore render_done_sem_;
public:
    explicit Swapchain(GpuContext& ctx);
    ~Swapchain();

    bool Recreate(uint32 width, uint32 height);
    bool Acquire(uint32& image_index);
    bool Present(uint32 image_index);
};

} // namespace euphrosyne
