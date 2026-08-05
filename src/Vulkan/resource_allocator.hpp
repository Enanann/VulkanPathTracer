#pragma once

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

    ////////////////////////////////////
    vma::raii::Buffer createBuffer(const vk::BufferCreateInfo&      bufferCreateInfo,
                                   const vma::AllocationCreateInfo& allocationCreateInfo,
                                   const void* data);

private:
    vma::raii::Allocator m_allocator{nullptr};

    const vk::raii::Device*         m_device;
    const vk::raii::PhysicalDevice* m_physicalDevice;
};
    
}; // namespace poki
