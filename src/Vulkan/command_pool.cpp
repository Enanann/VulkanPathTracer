#include "command_pool.hpp"
#include "log.hpp"

namespace poki {

void ManagedCommandPools::init(const poki::ManagedCommandPoolsInitInfo& initInfo) {
    m_device           = &initInfo.device;
    m_queueFamilyIndex = initInfo.queueFamilyIndex;

    vk::CommandPoolCreateInfo poolInfo {
        .flags            = initInfo.flags,
        .queueFamilyIndex = m_queueFamilyIndex
    };

    for (uint32_t i{0}; i < initInfo.maxPoolCount; ++i) {
        m_managedPools.emplace_back(ManagedCommandPools::ManageCommandPool{
            .pool = vk::raii::CommandPool(*m_device, poolInfo),
            .acquisitionIndex = 0
        });
    }
    
    LOGI("ManagedCommandPools created successfully");
}

vk::raii::CommandBuffer& ManagedCommandPools::acquireCommandBuffer(uint32_t explicitIndex) {
    ManagedCommandPools::ManageCommandPool& entry{m_managedPools[explicitIndex]};

    // If the thread requests more buffers than we have pre-allocated for this frame, grow the vector
    if (entry.acquisitionIndex >= static_cast<uint32_t>(entry.buffers.size())) {
        vk::CommandBufferAllocateInfo allocInfo {
            .commandPool        = *entry.pool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        vk::raii::CommandBuffers newCmd{*m_device, allocInfo};
        entry.buffers.push_back(std::move(newCmd.front()));
    }

    // return the next available buffer and increment the qcquisition tracker
    return entry.buffers[entry.acquisitionIndex++];
}

void ManagedCommandPools::resetPool(uint32_t explicitIndex) {
    ManagedCommandPools::ManageCommandPool& entry{m_managedPools[explicitIndex]};

    // Reset the underlying memory pool
    entry.pool.reset({});

    // Reset the acquisition tracker. 
    // The vk::raii::CommandBuffer objects in the vector remain valid handles
    // but their state is reset to 'Initial'
    entry.acquisitionIndex = 0;
}

}; // namespace poki
