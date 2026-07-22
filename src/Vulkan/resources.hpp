#pragma once
// Inspired by nvpro_core2 nvvk

#include <vulkan/vulkan_raii.hpp>

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

}; // namespace poki
