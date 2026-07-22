#include "semaphore.hpp"

namespace poki {

TimelineSemaphore::TimelineSemaphore(const vk::raii::Device& device, uint64_t initialValue) {
    vk::SemaphoreTypeCreateInfo typeCreateInfo{
        .semaphoreType = vk::SemaphoreType::eTimeline,
        .initialValue  = initialValue
    };
    vk::SemaphoreCreateInfo createInfo{
        .pNext = &typeCreateInfo
    };

    m_semaphore  = vk::raii::Semaphore(device, createInfo);
    m_value      = initialValue;
}

}; // namespace poki
