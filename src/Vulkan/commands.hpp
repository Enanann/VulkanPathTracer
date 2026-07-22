#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace poki {

// Helper to create a transient command pool (command buffer allocated from this pool will be short-lived)
vk::raii::CommandPool createTransientCommandPool(const vk::raii::Device& device, uint32_t queueFamilyIndex);

// Helper to create a temporary command buffer (single time command buffer) and begin it
// Use to record the commands to upload data, or transition images (Must not be used during render loop)
vk::raii::CommandBuffer createSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& cmdPool);

// Helper to end command buffer, submit on the provided queue, wait for completion, and free the command buffer within the provided pool
void engSingleTimeCommands(vk::raii::CommandBuffer& cmd, const vk::raii::Device& device, const vk::raii::Queue& queue);


}; // namespace poki
