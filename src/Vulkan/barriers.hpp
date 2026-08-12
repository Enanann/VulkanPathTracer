#pragma once

#include "resources.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <tuple>

namespace poki {

/*--
When the exact masks aren't critical, you can replace stage and access flags with
INFER_PIPELINE_STAGE_PARAM and INFER_ACCESS_FLAG_PARAM to infer them from the layout and stage, respectively. (like nvvk's)
--*/

// Maps image layouts to appropriate pipeline stages and access flags
// Used for synchronizing image state transitions in the pipeline
[[nodiscard]] constexpr std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2> inferPipelineStageAccessTuple(vk::ImageLayout layout) {
    switch(layout) {
        case vk::ImageLayout::eUndefined: {
            return std::make_tuple(vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone);
        }
        case vk::ImageLayout::eColorAttachmentOptimal: {
            return std::make_tuple(
                vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
                vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
            );
        }
        case vk::ImageLayout::eShaderReadOnlyOptimal: {
            return std::make_tuple(
                vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader 
                    | vk::PipelineStageFlagBits2::ePreRasterizationShaders | vk::PipelineStageFlagBits2::eAllCommands, 
                vk::AccessFlagBits2::eShaderRead
            );
        }
        case vk::ImageLayout::eTransferSrcOptimal: {
            return std::make_tuple(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead);
        }
        case vk::ImageLayout::eTransferDstOptimal: {
            return std::make_tuple(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite);
        }
        case vk::ImageLayout::eGeneral: {
            return std::make_tuple(
                vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::ePreRasterizationShaders 
                    | vk::PipelineStageFlagBits2::eAllCommands | vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite
                    | vk::AccessFlagBits2::eTransferWrite
            );
        }
        case vk::ImageLayout::ePresentSrcKHR: {
            return std::make_tuple(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone);
        }
        case vk::ImageLayout::eDepthAttachmentOptimal:
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        case vk::ImageLayout::eDepthReadOnlyOptimal: {
            return std::make_tuple(
                vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite
            );
        }
        case vk::ImageLayout::eAttachmentOptimal: {
            return std::make_tuple(
                vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests
                    | vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite
                    | vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
            );
        }
        default: {
            assert(false && "Unsupported layout transition");
            return std::make_tuple(vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
        }
    }
}

constexpr vk::PipelineStageFlags2 INFER_PIPELINE_STAGE_PARAM{~0ULL};
constexpr vk::AccessFlags2        INFER_ACCESS_FLAG_PARAM{~0ULL};

struct ImageMemoryBarrierParams {
    vk::PipelineStageFlags2   srcStageMask  = INFER_PIPELINE_STAGE_PARAM; // infer from oldLayout
    vk::PipelineStageFlags2   dstStageMask  = INFER_PIPELINE_STAGE_PARAM; // infer from newLayout
    vk::AccessFlags2          srcAccessMask = INFER_ACCESS_FLAG_PARAM;    // infer from oldLayout
    vk::AccessFlags2          dstAccessMask = INFER_ACCESS_FLAG_PARAM;    // infer from newLayout
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
