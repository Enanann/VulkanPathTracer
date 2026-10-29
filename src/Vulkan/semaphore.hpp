#pragma once

#include "config.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <array>

namespace poki {

// Simple wrapper for timeline semaphore
// ONLY ONE timeline semaphore should be used for multiple frames in flight
class TimelineSemaphore {
public:
    TimelineSemaphore() = default;
    TimelineSemaphore(const vk::raii::Device& device, uint64_t initialValue = 0);

    // Calculate the exact timeline value the CPU needs to wait for
    // uint64_t getWaitValue(uint32_t framesInFlight) const {
    //     return m_value >= framesInFlight ? (m_value - framesInFlight + 1) : 0;
    // }
    uint64_t getWaitValue(uint32_t framesInfFlightIndex) const {
        return m_frameCompletionValues[framesInfFlightIndex];
    }

    uint64_t getCurrentValue() const {return m_value;}

    // Increment time and returns the value GPU should signal next
    uint64_t nextSignalValue() {
        return ++m_value;
    }

    const vk::raii::Semaphore& get() const noexcept {return m_semaphore;}
    
    std::array<uint64_t, MAX_FRAMES_IN_FLIGHT>& getFrameCompletionArray() noexcept {return m_frameCompletionValues;}

private:
    vk::raii::Semaphore m_semaphore{nullptr};
    uint64_t            m_value{0};
    std::array<uint64_t, MAX_FRAMES_IN_FLIGHT> m_frameCompletionValues = {}; // track the timeline value for last graphics submission's completion
};

}; // namespace poki 
