#pragma once
// Inspired by nvpro_core2 nvvk

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>

#include <cstdint>

namespace poki {

//-----------------------------------------------------------------
// A queue is a sequence of commands that are executed in order.
// The queue is used to submit command buffers to the GPU.
// The family index is used to identify the queue family (graphic, compute, transfer, ...) .
// The queue index is used to identify the queue in the family, multiple queues can be in the same family.
//-----------------------------------------------------------------
struct QueueInfo {
    // ~ is bitwise NOT (~0U equiv to UINT32_MAX)
    uint32_t familyIndex{~0U};      // Family index of the queue
    uint32_t queueIndex{~0U};       // Index of the queue in the family
    vk::raii::Queue queue{nullptr}; // The queue object
};

//-----------------------------------------------------------------
// A buffer is a region of memory used to store data.
// It's used to store vertex data, index data, uniform data, and other types of data.
// There is a vma::raii::Buffer that represents the buffer
// vma::raii::Buffer is a special handle variants combine resource and allocation in a single RAII object:
// vma::raii::Buffer buffer = ...;
// const vk::raii::Buffer& vkbuf = buffer;
// const vma::raii::Allocation& allocation = buffer.getAllocation();
//-----------------------------------------------------------------
struct Buffer {
    vma::raii::Buffer buffer{nullptr};
    vk::DeviceSize    bufferSize{};
    // vk::DeviceAddress address{}; // Address of the buffer in the shader
    void*             mapping;
};

//-----------------------------------------------------------------
// An image is a region of memory used to store image data.
// It is used to store texture data, framebuffer data, and other types of data
// vma::raii::Image represents the image, it functions the same as vma::raii::Buffer
//-----------------------------------------------------------------
struct Image {
    vma::raii::Image    image{nullptr};
    vk::Format          format{vk::Format::eUndefined};
    vk::Extent3D        extent{};
    uint32_t            mipLevels{1};
    uint32_t            arrayLayers{1};

    // `sampler` may exist, NOT managed by `poki::ResourceAllocator`
    // Note: We don't use vk::raii::Sampler here because Sampler can be reuse by multiple image
    vk::Sampler         sampler{nullptr};
    // `imageView` may exist, managed by `poki::ResourceAllocator`
    vk::raii::ImageView imageView{nullptr};
    vk::ImageLayout     layout{vk::ImageLayout::eUndefined};
};

}; // namespace poki
