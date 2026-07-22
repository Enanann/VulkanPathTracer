#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace poki {

// ManagedCommandPools creation info
// device          : The logical device
// queueFamilyIndex: The queue family index
// flags           : Flags to create CommandPool (must not contains vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
// maxPoolCount    : Max pool count, should be equal to the frames in flight
struct ManagedCommandPoolsInitInfo {
    const vk::raii::Device&       device;
    uint32_t                      queueFamilyIndex;
    vk::CommandPoolCreateFlagBits flags{vk::CommandPoolCreateFlagBits::eTransient};
    uint32_t                      maxPoolCount;
};

// Manages command pools for a single queue family across multiple frames in flight
// The user is responsible for tracking completion and provide explicit pool index
class ManagedCommandPools {
public:
    ManagedCommandPools() = default;

    void init(const ManagedCommandPoolsInitInfo& initInfo);

    // Acquire a command buffer in the state 'Initial'
    // Inspired by nvvk's EXPLICIT_INDEX mode
    vk::raii::CommandBuffer& acquireCommandBuffer(uint32_t explicitIndex);

    // Call exactly once at the start of the frame for the current index
    // ONLY AFTER waiting for the previous frame to be completed
    void resetPool(uint32_t explicitIndex);
private:
    struct ManageCommandPool {
        vk::raii::CommandPool                pool{nullptr};
        std::vector<vk::raii::CommandBuffer> buffers;
        uint32_t                             acquisitionIndex{};
    };

    const vk::raii::Device*        m_device{nullptr};
    uint32_t                       m_queueFamilyIndex;
    std::vector<ManageCommandPool> m_managedPools;
};

}; // namespace poki
