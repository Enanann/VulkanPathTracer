#pragma once

#include "resources.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <tuple>

namespace poki {

/*--
When the exact masks aren't critical, you can replace stage and access flags with
INFER_PIPELINE_STAGE_PARAM and INFER_ACCESS_FLAG_PARAM to infer them from the layout and stage, respectively. (like nvvk's)
--*/

/*--
// Automatically infers appropriate access masks from pipeline stage flags
// `read` parameter determines read (true) or write (false) operations
// Used to simplify barrier creation when exact access masks aren't critical
--*/
[[nodiscard]] constexpr vk::AccessFlags2 inferAccessMaskFromStage(vk::PipelineStageFlagBits2 stage, bool read) {
    vk::AccessFlags2 access {0};

    if ((stage & (vk::PipelineStageFlagBits2::eAllCommands | vk::PipelineStageFlagBits2::eAllGraphics)) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eMemoryRead : vk::AccessFlagBits2::eMemoryWrite;
    }

    // Handle each possible stage bit
    if ((stage 
            & (vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eVertexShader
                | vk::PipelineStageFlagBits2::eMeshShaderEXT | vk::PipelineStageFlagBits2::eRayTracingShaderKHR | vk::PipelineStageFlagBits2::eTaskShaderEXT
                | vk::PipelineStageFlagBits2::ePreRasterizationShaders | vk::PipelineStageFlagBits2::eTessellationControlShader
                | vk::PipelineStageFlagBits2::eTessellationEvaluationShader | vk::PipelineStageFlagBits2::eGeometryShader)) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eUniformRead : vk::AccessFlagBits2::eShaderWrite;
    }

    if ((stage & vk::PipelineStageFlagBits2::eHost) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eHostRead : vk::AccessFlagBits2::eHostWrite;
    }

    if ((stage & vk::PipelineStageFlagBits2::eTransfer) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eTransferRead : vk::AccessFlagBits2::eTransferWrite;
    }

    if ((stage & vk::PipelineStageFlagBits2::eVertexAttributeInput) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= vk::AccessFlagBits2::eVertexAttributeRead;
    }
    if ((stage & vk::PipelineStageFlagBits2::eIndexInput) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= vk::AccessFlagBits2::eIndexRead;
    }
    if ((stage & vk::PipelineStageFlagBits2::eDrawIndirect) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= vk::AccessFlagBits2::eIndirectCommandRead;
    }

    if ((stage & (vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests)) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eDepthStencilAttachmentRead : vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
    }
    if ((stage & vk::PipelineStageFlagBits2::eColorAttachmentOutput) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eColorAttachmentRead : vk::AccessFlagBits2::eColorAttachmentWrite;
    }

    if ((stage & vk::PipelineStageFlagBits2::eCommandPreprocessEXT) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eCommandPreprocessReadEXT : vk::AccessFlagBits2::eCommandPreprocessWriteEXT;
    }

    if ((stage & vk::PipelineStageFlagBits2::eFragmentShadingRateAttachmentKHR) != static_cast<vk::PipelineStageFlags2>(0)) {

        access |= vk::AccessFlagBits2::eFragmentShadingRateAttachmentReadKHR;
    }

    if ((stage & vk::PipelineStageFlagBits2::eVideoDecodeKHR) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eVideoDecodeReadKHR : vk::AccessFlagBits2::eVideoDecodeWriteKHR;
    }
    if ((stage & vk::PipelineStageFlagBits2::eVideoEncodeKHR) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eVideoEncodeReadKHR : vk::AccessFlagBits2::eVideoEncodeWriteKHR;
    }

    if ((stage & vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eAccelerationStructureReadKHR : vk::AccessFlagBits2::eAccelerationStructureWriteKHR;
    }
    if ((stage & vk::PipelineStageFlagBits2::eAccelerationStructureCopyKHR) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= vk::AccessFlagBits2::eIndirectCommandRead;
    }

    if ((stage & vk::PipelineStageFlagBits2::eRayTracingShaderKHR) != static_cast<vk::PipelineStageFlags2>(0)) {
        access |= read ? vk::AccessFlagBits2::eAccelerationStructureReadKHR : static_cast<vk::AccessFlags2>(0);
    }

    assert((access != static_cast<vk::AccessFlags2>(0) || stage == vk::PipelineStageFlagBits2::eNone) && "Missing stage implementation");
    return access;
}

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

struct BufferMemoryBarrierParams {
    vk::PipelineStageFlags2   srcStageMask;
    vk::PipelineStageFlags2   dstStageMask;
    vk::AccessFlags2          srcAccessMask;
    vk::AccessFlags2          dstAccessMask;
    uint32_t                  srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    uint32_t                  dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    vk::Buffer                buffer = VK_NULL_HANDLE;
    vk::DeviceSize            offset = 0;
    vk::DeviceSize            size = vk::WholeSize;
};

[[nodiscard]] constexpr vk::BufferMemoryBarrier2 makeBufferMemoryBarrier(const BufferMemoryBarrierParams& params);

void cmdBufferMemoryBarrier(vk::raii::CommandBuffer& cmd, const BufferMemoryBarrierParams& params);

}; // namespace poki
