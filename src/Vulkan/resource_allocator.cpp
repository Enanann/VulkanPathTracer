#include "resource_allocator.hpp"

#include "log.hpp"

namespace poki {

void ResourceAllocator::init(const ResourceAllocatorInitInfo& createInfo) {
    m_allocator      = vma::raii::createAllocator(createInfo.instance, createInfo.device, createInfo.allocInfo);
    m_device         = &createInfo.device;
    m_physicalDevice = &createInfo.physicalDevice;

    LOGI("ResourceAllocator created successfully");
}
    
poki::Buffer ResourceAllocator::createBuffer(const vk::BufferCreateInfo&      bufferCreateInfo,
                                             const vma::AllocationCreateInfo& allocationCreateInfo) const
{
    vma::AllocationInfo allocInfoOut;
    poki::Buffer buffer;
    buffer.buffer = m_allocator.createBuffer(bufferCreateInfo, allocationCreateInfo, &allocInfoOut);
    buffer.bufferSize = bufferCreateInfo.size;
    buffer.mapping = allocInfoOut.pMappedData;
    
    // Get the GPU address of the buffer (idk for what)
    // const vk::BufferDeviceAddressInfo info {
    //     .buffer = buffer.buffer
    // };
    // buffer.address = m_device->getBufferAddress(info);

    return buffer;
}

bool ResourceAllocator::isNonCoherentlyMapped(const poki::Buffer& buffer) const {
    assert(buffer.mapping);
    vk::MemoryPropertyFlags memFlags = buffer.buffer.getAllocation().getMemoryProperties();
    return !(memFlags & vk::MemoryPropertyFlagBits::eHostCoherent);
}

void ResourceAllocator::flushBuffer(const poki::Buffer& buffer, vk::DeviceSize offset /*= 0*/, vk::DeviceSize size /*= vk::WholeSize*/) {
    if (isNonCoherentlyMapped(buffer)) {
        m_allocator.flushAllocations(*buffer.buffer.getAllocation(), offset, size);
    }
}

void ResourceAllocator::invalidateBuffer(const poki::Buffer& buffer, vk::DeviceSize offset /*= 0*/, vk::DeviceSize size /*= vk::WholeSize*/) {
    if (isNonCoherentlyMapped(buffer)) {
        m_allocator.invalidateAllocations(*buffer.buffer.getAllocation(), offset, size);
    }
}


}; // namespace poki
