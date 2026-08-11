#include "barriers.hpp"

namespace poki {

[[nodiscard]] constexpr vk::ImageMemoryBarrier2 makeImageMemoryBarrier(const ImageMemoryBarrierParams &params) {
    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask  = params.srcStageMask,
        .srcAccessMask = params.srcAccessMask,
        .dstStageMask  = params.dstStageMask,
        .dstAccessMask = params.dstAccessMask,
        .oldLayout     = params.oldLayout,
        .newLayout     = params.newLayout,
        .srcQueueFamilyIndex = params.srcQueueFamilyIndex,
        .dstQueueFamilyIndex = params.dstQueueFamilyIndex,
        .image = params.image,
        .subresourceRange = params.subresourceRange
    };

    return barrier;
}

void cmdImageMemoryBarrier(vk::raii::CommandBuffer& cmd, const ImageMemoryBarrierParams& params) {
    auto barrier{makeImageMemoryBarrier(params)};

    vk::DependencyInfo dependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier
    };
    cmd.pipelineBarrier2(dependencyInfo);
}

void cmdImageMemoryBarrier(vk::raii::CommandBuffer& cmd, poki::Image& image, const ImageMemoryBarrierParams& params) {
    ImageMemoryBarrierParams local = params;
    local.image = image.image;
    local.oldLayout = image.layout;

    cmdImageMemoryBarrier(cmd, local);

    image.layout = params.newLayout;
}

}; // namespace poki
