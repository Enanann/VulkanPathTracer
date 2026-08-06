#pragma once

#include "resources.hpp"

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>
#include <vk_mem_alloc_structs.hpp>

namespace poki {

struct ResourceAllocatorInitInfo {
    const vk::raii::Instance&       instance;
    const vk::raii::Device&         device;
    const vk::raii::PhysicalDevice& physicalDevice;
    vma::AllocatorCreateInfo        allocInfo;
};

class ResourceAllocator {
public:
    ResourceAllocator() = default;

    void init(const ResourceAllocatorInitInfo& createInfo);

    const vma::raii::Allocator& getAllocator() const noexcept {return m_allocator;}

    ////////////////////////////////////

    // Create a vma::raii::Buffer and mapped it
    void createBuffer(poki::Buffer&                    buffer,
                      const vk::BufferCreateInfo&      bufferCreateInfo,
                      const vma::AllocationCreateInfo& allocationCreateInfo);

    ////////////////////////////////////
    // Only legal for mapped buffers

    // Returns true if the buffer's memory properties has no vk::MemoryPropertyFlagBits::eHostCoherent
    // If `true` then must flush (GPU sees what CPU wrote) or invalidate (CPU sees what GPU wrote) the buffer manually 
    bool isNonCoherentlyMapped(const poki::Buffer& buffer) const;

    // Calls `vkFlushMappedMemoryRanges` via VMA only if the buffer is non-coherent, else, do nothing
    void flushBuffer(const poki::Buffer& buffer, vk::DeviceSize offset = 0, vk::DeviceSize size = vk::WholeSize);

    // Calls `vkInvalidateMappedMemoryRanges` via VMA only if the buffer is non-coherent, else, do nothing
    void invalidateBuffer(const poki::Buffer& buffer, vk::DeviceSize offset = 0, vk::DeviceSize size = vk::WholeSize);

private:
    vma::raii::Allocator m_allocator{nullptr};

    const vk::raii::Device*         m_device;
    const vk::raii::PhysicalDevice* m_physicalDevice;
};
    
}; // namespace poki
