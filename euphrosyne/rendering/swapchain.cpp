#include "swapchain.hpp"

namespace euphrosyne
{
Swapchain::Swapchain(GpuContext &ctx) : ctx_(ctx)
{
    auto info = VkSemaphoreCreateInfo(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    vkCreateSemaphore(ctx_.VkDevice(), &info, nullptr, &acquire_sem_);
    vkCreateSemaphore(ctx_.VkDevice(), &info, nullptr, &render_done_sem_);
}

Swapchain::~Swapchain()
{
    ctx_.WaitIdle();
    framebuffers_.clear();
    textures_.clear();

    if (swapchain_.swapchain)
        vkb::destroy_swapchain(swapchain_);

    vkDestroySemaphore(ctx_.VkDevice(), acquire_sem_, nullptr);
    vkDestroySemaphore(ctx_.VkDevice(), render_done_sem_, nullptr);
}

bool Swapchain::Recreate(uint32 width, uint32 height)
{
    ctx_.WaitIdle();
    framebuffers_.clear();
    textures_.clear();

    auto swapchain_builder = vkb::SwapchainBuilder(ctx_.VkbDevice());
    swapchain_builder.set_desired_format({VK_FORMAT_B8G8R8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    if (swapchain_.swapchain)
        swapchain_builder.set_old_swapchain(swapchain_);

    auto result = swapchain_builder.build();
    if (!result)
        throw runtime_error("Failed to (re)create Vulkan Swapchain: " + result.error().message());

    if (swapchain_.swapchain)
        vkb::destroy_swapchain(swapchain_);

    swapchain_ = result.value();

    auto images = swapchain_.get_images();
    if (!images)
        throw runtime_error("Failed to get swapchain images");

    for (auto img : images.value())
    {
        auto texture_desc = nvrhi::TextureDesc();
        texture_desc.width = swapchain_.extent.width;
        texture_desc.height = swapchain_.extent.height;
        texture_desc.format = nvrhi::Format::BGRA8_UNORM;
        texture_desc.isRenderTarget = true;
        texture_desc.initialState = nvrhi::ResourceStates::Present;
        texture_desc.debugName = "Backbuffer";

        auto texture =
            ctx_.Device()->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, nvrhi::Object(img), texture_desc);
        textures_.push_back(texture);
        framebuffers_.push_back(ctx_.Device()->createFramebuffer(nvrhi::FramebufferDesc().addColorAttachment(texture)));
    }

    return true;
}

bool Swapchain::Acquire(uint32 &image_index)
{
    auto vk_res = vkAcquireNextImageKHR(ctx_.VkDevice(), swapchain_.swapchain, UINT64_MAX, acquire_sem_, VK_NULL_HANDLE,
                                        &image_index);
    return vk_res == VK_SUCCESS || vk_res == VK_SUBOPTIMAL_KHR;
}

bool Swapchain::Present(uint32 image_index)
{
    auto present_info = VkPresentInfoKHR(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_done_sem_;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_.swapchain;
    present_info.pImageIndices = &image_index;

    auto res = vkQueuePresentKHR(ctx_.GraphicqQueue(), &present_info);
    // todo replace with per frame fences / nvrhi event queries later
    vkQueueWaitIdle(ctx_.GraphicqQueue());
    return res == VK_SUCCESS;
}
} // namespace euphrosyne