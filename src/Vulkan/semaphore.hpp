#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace poki {

// Simple wrapper for timeline semaphore
// ONLY ONE timeline semaphore should be used for multiple frames in flight
class TimelineSemaphore {
public:
    TimelineSemaphore() = default;
    TimelineSemaphore(const vk::raii::Device& device, uint64_t initialValue = 0);

    // Calculate the exact timeline value the CPU needs to wait for
    uint64_t getWaitValue(uint32_t framesInFlight) const {
        return m_value >= framesInFlight ? (m_value - framesInFlight + 1) : 0;
    }

    uint64_t getCurrentValue() const {return m_value;}

    // Increment time and returns the value GPU should signal next
    uint64_t nextSignalValue() {
        return ++m_value;
    }

    const vk::raii::Semaphore& get() const noexcept {return m_semaphore;}

private:
    vk::raii::Semaphore m_semaphore{nullptr};
    uint64_t            m_value{0};
};

}; // namespace poki 
