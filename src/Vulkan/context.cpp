#include "context.hpp"

#include "log.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace poki {

void poki::Context::init(const ContextInitInfo& contextInitInfo) {
    contextInfo = contextInitInfo;

    createInstance();
    selectPhysicalDevice();
    createDevice();
}

void Context::createInstance() {
    vk::ApplicationInfo appInfo {
        .pApplicationName = contextInfo.applicationName,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion    = contextInfo.apiVersion
    };

    // Check for the required instance extensions from GLFW
    uint32_t glfwExtensionCount{0};
    auto glfwExtensions{glfwGetRequiredInstanceExtensions(&glfwExtensionCount)};

    std::vector<const char*> requiredInstanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    for (const auto& ex : contextInfo.instanceExtensions) {
        requiredInstanceExtensions.push_back(ex);
    }

    auto extensionProperties{m_context.enumerateInstanceExtensionProperties()};
    for (size_t i{0}; i < requiredInstanceExtensions.size(); ++i) {
        if (std::ranges::none_of(extensionProperties, 
                                [extension = requiredInstanceExtensions[i]](const auto& extensionProperty) {
                                    return std::strcmp(extensionProperty.extensionName, extension) == 0; 
                                }))
        {
            throw std::runtime_error("Unsupported instance extension: " + std::string(requiredInstanceExtensions[i]));
        }
    }

    // Check for the required layers
    std::vector<const char*> layers;
    if (contextInfo.enableValidationLayers) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    auto layerProperties{m_context.enumerateInstanceLayerProperties()};
    for (const auto& layer : layers) {
        if (std::ranges::none_of(layerProperties, [req = layer](const auto& layerProperty) {
            return std::strcmp(req, layerProperty.layerName) == 0;
        }))
        {
            throw std::runtime_error("Unsupported layers: " + std::string(layer));
        }
    }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames     = layers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(requiredInstanceExtensions.size()),
        .ppEnabledExtensionNames = requiredInstanceExtensions.data()
    };

    m_instance = vk::raii::Instance(m_context, createInfo);

    LOGI("Vulkan instance created successfully");
}

void Context::selectPhysicalDevice() {
    if (m_instance == VK_NULL_HANDLE) {
        throw std::runtime_error("m_instance was null, need to call createInstance() first");
    }

    auto gpus{m_instance.enumeratePhysicalDevices()};

    if (gpus.empty()) {
        throw std::runtime_error("Can not find any GPUs with Vulkan support");
    }

    auto gpuIt{std::ranges::find_if(gpus, [&](const vk::raii::PhysicalDevice& gpu) {
        // Check support for Vulkan version
        bool supportVulkan{gpu.getProperties2().properties.apiVersion >= contextInfo.apiVersion};
        
        // Check support for all the required device extensions
        auto availableExtensions{gpu.enumerateDeviceExtensionProperties()};
        bool supportAllRequiredExtensions{std::ranges::all_of(contextInfo.deviceExtensions, [availableExtensions](const auto& devEx) {
            return std::ranges::any_of(availableExtensions, [devEx](const auto& avEx) {
                return std::strcmp(devEx, avEx.extensionName) == 0;
            });
        })};

        // Check support for all the required features
        FeatureChain supportedFeatures = gpu.getFeatures2<vk::PhysicalDeviceFeatures2,
                                                        vk::PhysicalDeviceVulkan11Features,
                                                        // vk::PhysicalDeviceVulkan12Features,
                                                        vk::PhysicalDeviceVulkan13Features,
                                                        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                                                        vk::PhysicalDeviceTimelineSemaphoreFeatures>();

        bool supportAllRequiredFeatures = supportedFeatures.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy  &&
                                          supportedFeatures.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                          supportedFeatures.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
                                          supportedFeatures.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                          supportedFeatures.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        return supportVulkan && supportAllRequiredExtensions && supportAllRequiredFeatures;
    })};

    if (gpuIt == gpus.end()) {
        throw std::runtime_error("Failed to find suitable GPU");
    } else {
        m_physicalDevice = *gpuIt;

        // find the queue family first
        m_desiredQueues = contextInfo.queues;
        bool foundQueues{findQueueFamilies()};
        if (!foundQueues) {
            throw std::runtime_error("Can not find suitable queue family");
        }
        
        LOGI("Physical device selected successfully");
        auto properties{m_physicalDevice.getProperties2()};
        printPhysicalDeviceProperties(properties);
    }
}

void Context::createDevice() {
    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("m_physicalDevice was null, need to call selectPhysicalDevice() first");
    }

    vk::DeviceCreateInfo createInfo{
        .pNext                   = &contextInfo.requiredFeatures.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = static_cast<uint32_t>(m_queueCreateInfos.size()),
        .pQueueCreateInfos       = m_queueCreateInfos.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(contextInfo.deviceExtensions.size()),
        .ppEnabledExtensionNames = contextInfo.deviceExtensions.data()
    };

    m_device = vk::raii::Device(m_physicalDevice, createInfo);

    for (auto& queue : m_queueInfos) {
        queue.queue       = vk::raii::Queue(m_device, queue.familyIndex, queue.queueIndex);
    }

    LOGI("Logical device created successfully");
}

// TODO: Check for present support and handle separate graphics and present queue?
bool Context::findQueueFamilies() {
    auto queueFamilies = m_physicalDevice.getQueueFamilyProperties2();
    uint32_t queueFamilyCount{static_cast<uint32_t>(queueFamilies.size())};

    std::unordered_map<uint32_t, uint32_t> queueFamilyUsage;
    for (uint32_t i{0}; i < queueFamilyCount; ++i) {
        queueFamilyUsage[i] = 0;
    }

    for (size_t i{0}; i < m_desiredQueues.size(); ++i) {
        bool found{false};
        for (uint32_t j{0}; j < queueFamilyCount; ++j) {
            // Check for queue family that contains all required queue flags and is unused
            // Avoid queue family with vk::QueueFlagBits::eGraphics if not needed
            if ((queueFamilies[j].queueFamilyProperties.queueFlags & m_desiredQueues[i]) == m_desiredQueues[i] 
                && queueFamilyUsage[j] == 0
                && ((m_desiredQueues[i] & vk::QueueFlagBits::eGraphics) || !(queueFamilies[j].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)))
            {
                m_queueInfos.push_back({j, queueFamilyUsage[j]});
                queueFamilyUsage[j]++;
                found = true;
                break;
            }
        }

        if (!found) {
            for(uint32_t j{0}; j < queueFamilyCount; ++j) {
                // Check for queue family that contains all required queue flags and allow reuse if queue count not exceeded
                // Avoid queue family with VK_QUEUE_GRAPHICS_BIT if not needed
                if((queueFamilies[j].queueFamilyProperties.queueFlags & m_desiredQueues[i]) == m_desiredQueues[i]
                    && queueFamilyUsage[j] < queueFamilies[j].queueFamilyProperties.queueCount
                    && ((m_desiredQueues[i] & vk::QueueFlagBits::eGraphics) || !(queueFamilies[j].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)))
                {
                    m_queueInfos.push_back({j, queueFamilyUsage[j]});
                    queueFamilyUsage[j]++;
                    found = true;
                    break;
                }
            }
        }

        if(!found) {
            for(uint32_t j{0}; j < queueFamilyCount; ++j) {
                // Check for a partial match and allow reuse if queue count not exceeded
                // Avoid queue family with VK_QUEUE_GRAPHICS_BIT if not needed
                if((queueFamilies[j].queueFamilyProperties.queueFlags & m_desiredQueues[i]) 
                    && queueFamilyUsage[j] < queueFamilies[j].queueFamilyProperties.queueCount
                    && ((m_desiredQueues[i] & vk::QueueFlagBits::eGraphics) || !(queueFamilies[j].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)))
                {
                    m_queueInfos.push_back({j, queueFamilyUsage[j]});
                    queueFamilyUsage[j]++;
                    found = true;
                    break;
                }
            }
        }

        if(!found) {
            for(uint32_t j = 0; j < queueFamilyCount; ++j) {
                // Check for a partial match and allow reuse if queue count not exceeded
                if((queueFamilies[j].queueFamilyProperties.queueFlags & m_desiredQueues[i]) 
                    && queueFamilyUsage[j] < queueFamilies[j].queueFamilyProperties.queueCount)
                {
                    m_queueInfos.push_back({j, queueFamilyUsage[j]});
                    queueFamilyUsage[j]++;
                    found = true;
                    break;
                }
            }
        }

        if(!found) {
        // If no suitable queue family is found
            return false;
        }
    }

    for (const auto& usage : queueFamilyUsage) {
        if (usage.second > 0) {
            m_queuePriorities.emplace_back(usage.second, 1.0f); // Same priority for all queues in a family
            m_queueCreateInfos.push_back({
                .queueFamilyIndex = usage.first,
                .queueCount       = usage.second,
                .pQueuePriorities = m_queuePriorities.back().data(),
            });
        }
    }

    return true;
} 

//--------------------------------------------------------------------------------------------------
// Static functions to print Vulkan information


std::string Context::getVendorName(uint32_t vendorID) {
  static const std::unordered_map<uint32_t, std::string> vendorMap = {{0x1002, "AMD"},      {0x1010, "ImgTec"},
                                                                      {0x10DE, "NVIDIA"},   {0x13B5, "ARM"},
                                                                      {0x5143, "Qualcomm"}, {0x8086, "INTEL"}};

  auto it = vendorMap.find(vendorID);
  return it != vendorMap.end() ? it->second : "Unknown Vendor";
}

std::string Context::getDeviceType(uint32_t deviceType) {
  static const std::unordered_map<uint32_t, std::string> deviceTypeMap = {{VK_PHYSICAL_DEVICE_TYPE_OTHER, "Other"},
                                                                          {VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, "Integrated GPU"},
                                                                          {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, "Discrete GPU"},
                                                                          {VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU, "Virtual GPU"},
                                                                          {VK_PHYSICAL_DEVICE_TYPE_CPU, "CPU"}};

  auto it = deviceTypeMap.find(deviceType);
  return it != deviceTypeMap.end() ? it->second : "Unknown";
}


std::string Context::getVersionString(uint32_t version) {
  return std::to_string(VK_VERSION_MAJOR(version)) + "."    //
         + std::to_string(VK_VERSION_MINOR(version)) + "."  //
         + std::to_string(VK_VERSION_PATCH(version));
}

void Context::printPhysicalDeviceProperties(const vk::PhysicalDeviceProperties2& properties) {
    LOGI(" - Device Name    : {}", std::string(properties.properties.deviceName));
    LOGI(" - Vendor         : {}", getVendorName(properties.properties.vendorID));
    LOGI(" - Driver Version : {}", getVersionString(properties.properties.driverVersion));
    LOGI(" - API Version    : {}", getVersionString(properties.properties.apiVersion));
    LOGI(" - Device Type    : {}", getDeviceType(static_cast<uint32_t>(properties.properties.deviceType)));
}

}; // namespace poki
