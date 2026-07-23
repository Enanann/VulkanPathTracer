#include "swapchain.hpp"
#include "log.hpp"

#include <vulkan/vulkan_to_string.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace poki {

void Swapchain::init(const SwapchainInitInfo& initInfo, vk::raii::SurfaceKHR&& surface) {
    m_physicalDevice = &initInfo.physicalDevice;
    m_device         = &initInfo.device;
    m_queue          = initInfo.queue;
    m_surface        = std::move(surface);
    m_framesInFlight = initInfo.framesInFlight;

    // Checking for presentation support
    vk::Bool32 supportPresents{m_physicalDevice->getSurfaceSupportKHR(m_queue.familyIndex, *m_surface)};
    if (supportPresents != vk::True) {
        throw std::runtime_error("Selected queue family doesn't have support for presentation");
    }
}

void Swapchain::initResources(GLFWwindow* window) {
    m_availableSurfaceFormats = m_physicalDevice->getSurfaceFormats2KHR({.surface = m_surface});
    std::vector<vk::PresentModeKHR> availablePresentModes{m_physicalDevice->getSurfacePresentModesKHR(m_surface)};
    vk::SurfaceCapabilities2KHR capabilities{m_physicalDevice->getSurfaceCapabilities2KHR({.surface = m_surface})}; 

    uint32_t imageCount = capabilities.surfaceCapabilities.minImageCount + 1;
    // maxImageCount = 0 meant that there're unlimited images
    imageCount = (capabilities.surfaceCapabilities.maxImageCount > 0 && imageCount > capabilities.surfaceCapabilities.maxImageCount) 
                    ? capabilities.surfaceCapabilities.maxImageCount 
                    : imageCount;

    m_imageCount = imageCount;
    m_surfaceFormat = chooseSurfaceFormat(m_availableSurfaceFormats);
    m_swapExtent    = chooseSwapExtent(capabilities, window);

    // Create the swapchain 
    vk::SwapchainCreateInfoKHR createInfo {
        .flags            = vk::SwapchainCreateFlagsKHR(),
        .surface          = m_surface,
        .minImageCount    = imageCount,
        .imageFormat      = m_surfaceFormat.surfaceFormat.format,
        .imageColorSpace  = m_surfaceFormat.surfaceFormat.colorSpace,
        .imageExtent      = m_swapExtent,
        .imageArrayLayers = 1,
        .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform     = capabilities.surfaceCapabilities.currentTransform,
        .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode      = chooseSwapPresentMode(availablePresentModes),
        .clipped          = true,
        .oldSwapchain     = nullptr
    };
    m_swapchain = vk::raii::SwapchainKHR(*m_device, createInfo);

    // Retrieve the swapchain images
    std::vector<vk::Image> swapImages = m_swapchain.getImages();
    for (size_t i{0}; i < swapImages.size(); ++i) {
        const vk::ImageViewCreateInfo imageViewCreateInfo {
            .image            = swapImages[i],
            .viewType         = vk::ImageViewType::e2D,
            .format           = m_surfaceFormat.surfaceFormat.format,
            // .components       =,
            .subresourceRange = {
                .aspectMask     = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };

        m_images.emplace_back(Swapchain::Image{
            swapImages[i], 
            vk::raii::ImageView(*m_device, imageViewCreateInfo), 
            vk::raii::Semaphore(*m_device, vk::SemaphoreCreateInfo{})
        });
    }

    // Initialize the frame resources
    for (uint32_t i{0}; i < m_framesInFlight; ++i) {
        m_frameResources.emplace_back(vk::raii::Semaphore(*m_device, vk::SemaphoreCreateInfo{}));
    }

    LOGI("Swapchain created successfully ({} images)", imageCount);
    LOGI(" - Swapchain image format: {}", vk::to_string(getSwapchainImageFormat()));
}

void Swapchain::reinitResources(GLFWwindow* window) {
    // Handle window minimization
    int width{};
    int height{};
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Clean up the swapchain
    m_queue.queue.waitIdle(); // wait for all frames to finish rendering to recreate the swapchain
    m_images.clear();
    m_frameResources.clear();

    // Rebuild the swapchain
    m_needRebuild = false;
    // We need to explicitly delete the old vk::raii::SwapchainKHR because initResources() doesn't do it fast enough 
    m_swapchain.clear();
    initResources(window);
}

[[nodiscard]] std::pair<vk::Result, uint32_t> Swapchain::acquireNextImage(uint32_t frameIndex) {
    try {
        auto [result, imageIndex] = m_swapchain.acquireNextImage(std::numeric_limits<uint64_t>::max(), *m_frameResources[frameIndex].imageAvailableSemaphore, VK_NULL_HANDLE);
        return {result, imageIndex};
    } catch (const vk::OutOfDateKHRError&) { // Catch vk::OutOfDateKHRError since Vulkan count this as a throw
        return {vk::Result::eErrorOutOfDateKHR, 0};
    }
}

[[nodiscard]] vk::Result Swapchain::presentFrame(uint32_t imageIndex) {
    try {
        vk::PresentInfoKHR presentInfo{
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &*m_images[imageIndex].renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &*m_swapchain,
            .pImageIndices = &imageIndex
        };
    
        auto result = m_queue.queue.presentKHR(presentInfo);
        return result;
    } catch (const vk::OutOfDateKHRError&) { // Catch vk::OutOfDateKHRError since Vulkan count this as a throw
        return vk::Result::eErrorOutOfDateKHR;
    }
}

vk::SurfaceFormat2KHR Swapchain::chooseSurfaceFormat(const std::vector<vk::SurfaceFormat2KHR>& availableSurfaceFormats) {
    assert(!availableSurfaceFormats.empty());
    for (const auto& format : availableSurfaceFormats) {
        if (format.surfaceFormat.format == vk::Format::eB8G8R8A8Srgb
            && format.surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }
    return availableSurfaceFormats[0];
}

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes, bool vSync) {
    auto supports = [&availablePresentModes](vk::PresentModeKHR mode) {
        return std::ranges::contains(availablePresentModes, mode);
    };
    
    if (vSync) {
        if (supports(m_preferredVsyncOnMode)) {
            return m_preferredVsyncOnMode;
        }
    } else {
        if (supports(m_preferredVsyncOffMode)) {
            return m_preferredVsyncOffMode;
        }
    }
    if (supports(vk::PresentModeKHR::eImmediate)) return vk::PresentModeKHR::eImmediate;
    if (supports(vk::PresentModeKHR::eMailbox)) return vk::PresentModeKHR::eMailbox;
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseSwapExtent(const vk::SurfaceCapabilities2KHR& capabilities, GLFWwindow* window) {
    if (capabilities.surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.surfaceCapabilities.currentExtent;
    }

    int width{};
    int height{};
    glfwGetFramebufferSize(window, &width, &height);

    return {
        .width  = std::clamp<uint32_t>(width,
                                      capabilities.surfaceCapabilities.minImageExtent.width,
                                      capabilities.surfaceCapabilities.maxImageExtent.width),
        .height = std::clamp<uint32_t>(height,
                                       capabilities.surfaceCapabilities.minImageExtent.height,
                                       capabilities.surfaceCapabilities.maxImageExtent.height)
    };
}

}; // namespace poki
