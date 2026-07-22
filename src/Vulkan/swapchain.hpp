#pragma once

#include "resources.hpp"

#include "GLFW/glfw3.h"
#include <cstdint>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace poki {

// Swapchain creation info
// physicalDevice: The physical device (GPU)
// device        : The logical device (interface to the GPU)
// queue         : The queue used to submit command buffers to the GPU, can be copy as vk::raii::Queue doesn't own the queue, logical device owns it
// framesInFlight: The number of frames in flight (2 or 3 is the best)
struct SwapchainInitInfo {
    const vk::raii::PhysicalDevice& physicalDevice;
    const vk::raii::Device&         device;
    QueueInfo                       queue{};
    uint32_t                        framesInFlight{2};
};

class Swapchain {
public:
    Swapchain() = default;

    void init(const SwapchainInitInfo& initInfo, vk::raii::SurfaceKHR&& surface);

    void initResources(GLFWwindow* window);

    [[nodiscard]] vk::Format      getSwapchainImageFormat() const noexcept {return m_surfaceFormat.surfaceFormat.format;}
    const vk::raii::SwapchainKHR& getSwapchainRAII() const noexcept {return m_swapchain;}
    vk::Image                     getImage(uint32_t imageIndex) const noexcept {return m_images[imageIndex].image;}
    const vk::raii::ImageView&    getImageView(uint32_t imageIndex) const noexcept {return m_images[imageIndex].imageView;}
    const vk::Extent2D&           getExtent() const noexcept {return m_swapExtent;}

    // Number of swapchain images
    uint32_t getImageCount() const {return m_imageCount;}

    // Number of frames in flight
    uint32_t getFramesInFlight() const {return m_framesInFlight;}

    // Get imageAvailableSemaphore 
    const vk::raii::Semaphore& getImageAvailableSemaphore(uint32_t frameIndex) {return m_frameResources[frameIndex].imageAvailableSemaphore;}

    // Get renderFinishedSemaphore
    const vk::raii::Semaphore& getRenderFinishedSemaphore(uint32_t imageIndex) {return m_images[imageIndex].renderFinishedSemaphore;}

    // Return a vk::Result and the image index of the next available presentable image for this frame
    // Signal the imageAvailableSemaphore of the current m_frameResources
    [[nodiscard]] std::pair<vk::Result, uint32_t> acquireNextImage(uint32_t frameIndex);

    // Presents the rendered image to the screen
    // The renderFinishedSemaphore ensures that the image is presented only after it finished rendering
    [[nodiscard]] vk::Result presentFrame(uint32_t imageIndex);

private:
    // Per-swapchain-image 
    struct Image {
        vk::Image image{nullptr};
        vk::raii::ImageView imageView{nullptr};
        vk::raii::Semaphore renderFinishedSemaphore{nullptr}; // Binary semaphore: Signeld when rendering done, waited on by present
    };

    // Per-frame-in-flight
    struct FrameResources {
        vk::raii::Semaphore imageAvailableSemaphore{nullptr}; // Binary semaphore: Signal by acquireNextImage()
    };

    // Choose the most common format (vk::Format::eB8G8R8A8Srgb)
    vk::SurfaceFormat2KHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormat2KHR>& availableSurfaceFormats);

    /*
     * Swapchain's present mode is chosen based on vSync
     * Use`m_preferredVsyncOnMode` when vSync=true and the mode is supported 
     * Use`m_preferredVsyncOffMode` when vSync=false and the mode is supported
     * Else, choose from the most prefferred to least:
     *    1. vk::PresentModeKHR::eImmediate (vSync=false) : Lowest latency, allows tearing
     *    2. vk::PresentModeKHR::eMailbox                 : Lowest latency without tearing
     *    3. vk::PresentModeKHR::eFifo                    : Always supported
    */    
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes, bool vSync = true);

    /*
     * Choose the swap extent (swapchain images' resolution)
     * If `currentExtent` is equal to UINT32_MAX, then we must choose the size ourself (e.g. using glfwGetFramebufferSize)
     * Else, use the actual window size
    */
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilities2KHR& capabilities, GLFWwindow* window);

private:
    vk::raii::SurfaceKHR   m_surface{nullptr};
    vk::raii::SwapchainKHR m_swapchain{nullptr};
    vk::SurfaceFormat2KHR  m_surfaceFormat{};
    vk::Extent2D           m_swapExtent{};

    std::vector<Image>          m_images{};              // Swapchain images and their views
    std::vector<FrameResources> m_frameResources{};      // Synchronization for each frame in flight

    const vk::raii::PhysicalDevice* m_physicalDevice;
    const vk::raii::Device*         m_device;         
    QueueInfo                       m_queue{};

    vk::PresentModeKHR                 m_preferredVsyncOnMode{vk::PresentModeKHR::eMailbox};    // use if available
    vk::PresentModeKHR                 m_preferredVsyncOffMode{vk::PresentModeKHR::eImmediate}; // use if available
    std::vector<vk::SurfaceFormat2KHR> m_availableSurfaceFormats{};

    uint32_t m_imageCount{};     // The swapchain's image count
    uint32_t m_framesInFlight{}; // Max frames in flight (<= m_imageCount)
};

}; // namespace poki
