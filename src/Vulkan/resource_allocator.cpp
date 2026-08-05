#include "resource_allocator.hpp"

#include "log.hpp"

#include <cstring>

namespace poki {

void ResourceAllocator::init(const ResourceAllocatorInitInfo& createInfo) {
    m_allocator      = vma::raii::createAllocator(createInfo.instance, createInfo.device, createInfo.allocInfo);
    m_device         = &createInfo.device;
    m_physicalDevice = &createInfo.physicalDevice;

    LOGI("ResourceAllocator created successfully");
}
    
vma::raii::Buffer ResourceAllocator::createBuffer(const vk::BufferCreateInfo& bufferCreateInfo,
                                                  const vma::AllocationCreateInfo& allocationCreateInfo,
                                                  const void* data) 
{
    vma::AllocationInfo allocInfo;
    auto buffer = m_allocator.createBuffer(bufferCreateInfo, allocationCreateInfo, &allocInfo);
    std::memcpy(allocInfo.pMappedData, data, bufferCreateInfo.size);
    m_allocator.flushAllocations(*buffer.getAllocation(), {0}, bufferCreateInfo.size);
    return buffer;
}


}; // namespace poki
