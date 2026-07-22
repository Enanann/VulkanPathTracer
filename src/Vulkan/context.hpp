#pragma once

#include "resources.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <string>
#include <vector>

namespace poki {

// TODO: Make this more flexible (as an input?)
using FeatureChain = vk::StructureChain<vk::PhysicalDeviceFeatures2,
                                        vk::PhysicalDeviceVulkan11Features,
                                        // vk::PhysicalDeviceVulkan12Features,
                                        vk::PhysicalDeviceVulkan13Features,
                                        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                                        vk::PhysicalDeviceTimelineSemaphoreFeatures>;

// Vulkan context creation info
// applicationName       : Application name 
// apiVersion            : Vulkan API version
// instanceExtensions    : Addition instance extensions beside glfw's
// deviceExtensions      : Device extensions
// requiredFeatures      : A vk::StructureChain for all the required features
// queues                : All desired queues (the present queue should be index 0 for now)
// enableValidationLayers: Enable Vulkan validation layers
struct ContextInitInfo {
    const char*                 applicationName    = "";
    uint32_t                    apiVersion         = vk::ApiVersion14;
    std::vector<const char*>    instanceExtensions = {};
    std::vector<const char*>    deviceExtensions   = {};
    FeatureChain                requiredFeatures;
    std::vector<vk::QueueFlags> queues             = {vk::QueueFlagBits::eGraphics};  
#ifdef NDEBUG
    bool enableValidationLayers = false;
#else 
    bool enableValidationLayers = true;
#endif
};

/**
 * @brief Handle the Vulkan context creation
 * 
 * @details Include Context, Instance, Physical/Logical Device, and Queue creation
 */
class Context {
public:
    Context() = default;

    void init(const ContextInitInfo& contextInitInfo);

    // vk::Instance        getInstance()       const noexcept {return *m_instance;}
    // vk::Device          getDevice()         const noexcept {return *m_device;}
    // vk::PhysicalDevice  getPhysicalDevice() const noexcept {return *m_physicalDevice;}
    const vk::raii::Instance&       getInstanceRAII()   const noexcept {return m_instance;}
    const vk::raii::Device&         getDeviceRAII()   const noexcept {return m_device;}
    const vk::raii::PhysicalDevice& getPhysicalDeviceRAII()   const noexcept {return m_physicalDevice;}
    const QueueInfo&                getQueueInfo(uint32_t index) const {return m_queueInfos[index];}
    const std::vector<QueueInfo>&   getQueueInfos() const noexcept {return m_queueInfos;}


    // nvvk's static function
    static std::string getVendorName(uint32_t vendorID);
    static std::string getDeviceType(uint32_t deviceType);
    static std::string getVersionString(uint32_t version);
    static void printPhysicalDeviceProperties(const vk::PhysicalDeviceProperties2& properties);

private:
    // Used internally to create the Vulkan context
    void createInstance();
    void selectPhysicalDevice();
    void createDevice();
    bool findQueueFamilies();
    ContextInitInfo contextInfo{};

private:
    vk::raii::Context        m_context;
    vk::raii::Instance       m_instance{nullptr};
    vk::raii::PhysicalDevice m_physicalDevice{nullptr};
    vk::raii::Device         m_device{nullptr};

    // For queue creation
    std::vector<vk::QueueFlags>            m_desiredQueues{};
    std::vector<vk::DeviceQueueCreateInfo> m_queueCreateInfos{};
    std::vector<poki::QueueInfo>           m_queueInfos{};
    std::vector<std::vector<float>>        m_queuePriorities{}; // [familyIndex][queueIndexWithinFamily]
};

}; // namespace poki
