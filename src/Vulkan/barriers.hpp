#pragma once

#include "resources.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace poki {

// TODO:
/*--
When the exact masks aren't critical, you can replace stage and access flags with
INFER_BARRIER_PARAMS to infer them from the layout and stage, respectively. (like nvvk's)
--*/

struct ImageMemoryBarrierParams {
    vk::PipelineStageFlags2   srcStageMask;
    vk::AccessFlags2          srcAccessMask;
    vk::PipelineStageFlags2   dstStageMask;
    vk::AccessFlags2          dstAccessMask;
    vk::ImageLayout           oldLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout           newLayout = vk::ImageLayout::eUndefined;
    uint32_t                  srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    uint32_t                  dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    vk::Image                 image = VK_NULL_HANDLE;

    // Default for color attachments. MUST be overwritten for other transitions.
    vk::ImageSubresourceRange subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
};

[[nodiscard]] constexpr vk::ImageMemoryBarrier2 makeImageMemoryBarrier(const ImageMemoryBarrierParams& params);

// Helper function to transition an image from one layout to another
void cmdImageMemoryBarrier(vk::raii::CommandBuffer& cmd, const ImageMemoryBarrierParams& params);

void cmdImageMemoryBarrier(vk::raii::CommandBuffer& cmd, poki::Image& image, const ImageMemoryBarrierParams& params);

}; // namespace poki
