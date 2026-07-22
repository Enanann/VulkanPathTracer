#include "commands.hpp"

#include <cstdint>

namespace poki {

vk::raii::CommandPool createTransientCommandPool(const vk::raii::Device& device, uint32_t queueFamilyIndex) {
    vk::CommandPoolCreateInfo poolInfo{
        .flags            = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = queueFamilyIndex
    };
    return vk::raii::CommandPool(device, poolInfo);
}

vk::raii::CommandBuffer createSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& cmdPool) {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool        = *cmdPool,
        .level              = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    vk::raii::CommandBuffer cmd{std::move(vk::raii::CommandBuffers(device, allocInfo).front())};
    
    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    cmd.begin(beginInfo);

    return cmd;
}

void engSingleTimeCommands(vk::raii::CommandBuffer& cmd, const vk::raii::Device& device, const vk::raii::Queue& queue) {
    cmd.end();

    // Create a temporary fence for synchronization
    vk::FenceCreateInfo fenceCreateInfo;
    vk::raii::Fence fence(device, fenceCreateInfo);

    vk::CommandBufferSubmitInfo cmdSubmitInfo{
        .commandBuffer = *cmd,
        .deviceMask    = 0
    };
    vk::SubmitInfo2 submitInfo{
        .waitSemaphoreInfoCount   = 0,
        .pWaitSemaphoreInfos      = nullptr,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &cmdSubmitInfo,
        .signalSemaphoreInfoCount = 0,
        .pSignalSemaphoreInfos    = nullptr
    };
    
    // Submit to the queue, and tell it to signal the specific fence
    queue.submit2(submitInfo, *fence);

    // Block CPU until ONLY this specific submission is completed
    // vk::True  : Wait for all fences in the array (only one here)
    // UINT64_MAX: Wait forever (no timeout)
    vk::Result result{device.waitForFences(*fence, vk::True, UINT64_MAX)};
}

}; // namespace poki
