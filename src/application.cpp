#include "application.hpp"

#include "barriers.hpp"
#include "commands.hpp"
#include "descriptors.hpp"
#include "shader.hpp"
#include "vertex.hpp"
#include "stb_image.h"
#include "vulkan/vulkan.hpp"

#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <chrono>

static auto startTime = std::chrono::high_resolution_clock::now();

const std::vector<Vertex> vertices = {
    {{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{-1.0f, 3.0f},  {1.0f, 1.0f, 1.0f}, {0.0f, 2.0f}},
    {{3.0f, -1.0f},  {0.0f, 0.0f, 1.0f}, {2.0f, 0.0f}},

    // {{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    // {{1.0f, 1.0f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    // {{-1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

    // {{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    // {{1.0f, -1.0f},  {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    // {{1.0f, 1.0f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

Application::Application() {
    poki::WindowInitInfo windowInitInfo{ // create glfw window first, if not (glfwinit()) will make glfwGetRequiredInstanceExtensions() wrong
        .width     = 1280,
        .height    = 720,
        .resizable = true,
        .title     = "pokisuki"
    };
    m_window.init(windowInitInfo);

    poki::ContextInitInfo contextInitInfo{
        .instanceExtensions = {vk::KHRGetSurfaceCapabilities2ExtensionName},
        .deviceExtensions = {vk::KHRSwapchainExtensionName,
                            vk::KHRSpirv14ExtensionName,
                            vk::KHRSynchronization2ExtensionName},
        .requiredFeatures = {
            {.features = {.samplerAnisotropy = true}},            // vk::PhysicalDeviceFeatures2
            {.shaderDrawParameters = true},                       // vk::PhysicalDeviceVulkan11Features
            // {},                                                // vk::PhysicalDeviceVulkan12Features
            {.synchronization2 = true, .dynamicRendering = true}, // vk::PhysicalDeviceVulkan13Features
            {.extendedDynamicState = true},                       // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
            {.timelineSemaphore = true},                          // vk::PhysicalDeviceTimelineSemaphoreFeatures
        },
        // .queues = {vk::QueueFlagBits::eGraphics, vk::QueueFlagBits::eCompute}
    };
    m_context.init(contextInitInfo);

    poki::SwapchainInitInfo swapchainInitInfo{
        .physicalDevice = m_context.getPhysicalDeviceRAII(),
        .device         = m_context.getDeviceRAII(),
        .queue          = m_context.getQueueInfo(0) // Most of the time, the queue family of index 0 supports Present
    };
    m_swapchain.init(swapchainInitInfo, m_window.createSurface(m_context.getInstanceRAII()));
    m_swapchain.initResources(m_window.getGLFWHandle());

    // Descriptor Sets
    poki::DescriptorBindings uniformBufferBinding; // FiF (UBO)
    uniformBufferBinding.addBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
    m_fifDescriptorSetContainer.init(uniformBufferBinding, m_context.getDeviceRAII(), MAX_FRAMES_IN_FLIGHT, {}, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    poki::DescriptorBindings textureBinding; // The same across multiple FiF (Texture) 
    textureBinding.addBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    textureBinding.addBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    m_textureDescriptorSetContainer.init(textureBinding, m_context.getDeviceRAII(), 1, {}, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    std::vector<vk::DescriptorSetLayout> descriptorSetLayout = {*m_fifDescriptorSetContainer.getLayout(), *m_textureDescriptorSetContainer.getLayout()};
    // Graphics Pipeline
    std::vector<poki::ShaderStageInfo> shaderStages{{vk::ShaderStageFlagBits::eVertex, "vertMain"}, {vk::ShaderStageFlagBits::eFragment, "fragMain"}};
    auto vertexBindingDescription{Vertex::getBindingDescription()};
    auto vertexAttributeDescription{Vertex::getAttributeDescriptions()};
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .setLayoutCount         = static_cast<uint32_t>(descriptorSetLayout.size()),
        .pSetLayouts            = descriptorSetLayout.data(),
        .pushConstantRangeCount = 0,
        // .pPushConstantRanges    = pcSize
    };
    m_pipelineLayout = vk::raii::PipelineLayout(m_context.getDeviceRAII(), pipelineLayoutCreateInfo);
    poki::GraphicsPipelineInitInfo graphicsPipelineInitInfo{
        .device = m_context.getDeviceRAII(),
        .shaderPath = "build/src/Shaders/shader_base.spv",
        .shaderStages = shaderStages,
        .colorFormat = m_swapchain.getSwapchainImageFormat(),
        .enableDepth = vk::False,
        .layout = m_pipelineLayout,
        .vertexBindings = vertexBindingDescription,
        .vertexAttributes = vertexAttributeDescription
    };
    m_graphicsPipeline.init(graphicsPipelineInitInfo);

    // Compute descriptors
    poki::DescriptorBindings bindings_2;
    bindings_2.addBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute);
    m_compDescriptorSetContainer.init(bindings_2, m_context.getDeviceRAII(), 1, {}, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    std::vector<vk::DescriptorSetLayout> compDescriptorSetLayout = {*m_compDescriptorSetContainer.getLayout()};

    // Compute pipeline
    vk::PipelineLayoutCreateInfo computePipelineLayoutCreateInfo{
        .setLayoutCount = static_cast<uint32_t>(compDescriptorSetLayout.size()),
        .pSetLayouts    = compDescriptorSetLayout.data()
    };
    m_computePipelineLayout = vk::raii::PipelineLayout(m_context.getDeviceRAII(), computePipelineLayoutCreateInfo);

    auto compShader = poki::createShaderModule(m_context.getDeviceRAII() ,"build/src/Shaders/gradient.spv");
    vk::PipelineShaderStageCreateInfo ssCreateInfo{
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = compShader,
        .pName = "compMain"
    };

    vk::ComputePipelineCreateInfo compCreateInfo{
        .stage = ssCreateInfo,
        .layout = *m_computePipelineLayout
    };
    m_computePipeline = vk::raii::Pipeline(m_context.getDeviceRAII(), nullptr, compCreateInfo);

    // ManagedCommandPools
    poki::ManagedCommandPoolsInitInfo managedCommandPoolsInitInfo {
        .device           = m_context.getDeviceRAII(),
        .queueFamilyIndex = m_context.getQueueInfo(0).familyIndex,
        .maxPoolCount     = MAX_FRAMES_IN_FLIGHT
    };
    m_ManagedCommandPools.init(managedCommandPoolsInitInfo);

    m_timelineSemaphore = poki::TimelineSemaphore(m_context.getDeviceRAII(), 0);

    // ResourceAllocator
    poki::ResourceAllocatorInitInfo RAInitInfo {
        .instance = m_context.getInstanceRAII(),
        .device  = m_context.getDeviceRAII(),
        .physicalDevice = m_context.getPhysicalDeviceRAII(),
        .allocInfo = {{}, *m_context.getPhysicalDeviceRAII()} 
    };
    m_resourceAllocator.init(RAInitInfo);

    // Vertex Buffer
    auto vertexBufferSize = sizeof(vertices[0]) * vertices.size(); 
    poki::Buffer stagingBuffer = m_resourceAllocator.createBuffer(
        {
            .size = vertexBufferSize, 
            .usage = vk::BufferUsageFlagBits::eTransferSrc, 
            .sharingMode = vk::SharingMode::eExclusive
        }, 
        {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
            .usage = vma::MemoryUsage::eAuto
        }
    );
    m_vertexBuffer = m_resourceAllocator.createBuffer( 
        {
            .size = vertexBufferSize, 
            .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
            .sharingMode = vk::SharingMode::eExclusive
        }, 
        {
            .flags = {}, 
            .usage = vma::MemoryUsage::eAutoPreferDevice,
            .requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal
        }
    );
    // Map the vertices data
    std::memcpy(stagingBuffer.mapping, vertices.data(), vertexBufferSize);

    auto tempCmdPool{poki::createTransientCommandPool(m_context.getDeviceRAII(), 0)};
    {
        auto singleTimeCmd{poki::createSingleTimeCommands(m_context.getDeviceRAII(), tempCmdPool)};
        singleTimeCmd.copyBuffer(stagingBuffer.buffer, m_vertexBuffer.buffer, vk::BufferCopy{.srcOffset = 0, .dstOffset = 0, .size = vertexBufferSize});
        poki::engSingleTimeCommands(singleTimeCmd, m_context.getDeviceRAII(), m_context.getQueueInfo(0).queue);
    }

    // Uniform Buffer
    auto uniformBufferSize = sizeof(UniformBufferObject);
    for (auto i{0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        poki::Buffer ubo = m_resourceAllocator.createBuffer(
            {
                .size = uniformBufferSize,
                .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                .sharingMode = vk::SharingMode::eExclusive
            }, 
            {
                .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
                .usage = vma::MemoryUsage::eAuto,
                .requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            }
        );
        m_uniformBuffers.emplace_back(std::move(ubo));
    }

    // Texture (Create a temporary purple texture instead of loading an image with `stbi_load()`.
    // See the commented-out code below.)
    int textWidth{2};
    int texHeight{2};
    int texChannels{4};
    // stbi_uc* pixels{stbi_load("/path_to_image", &textWidth, &texHeight, &texChannels, STBI_rgb_alpha)};
    // if (!pixels) {
    //     throw std::runtime_error("Failed to load texture image");
    // }
    vk::DeviceSize texSize{static_cast<uint64_t>(textWidth * texHeight * 4)};
    std::array<stbi_uc, 2 * 2 * 4> pixels;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        pixels[i + 0] = 200; // R
        pixels[i + 1] = 160; // G
        pixels[i + 2] = 255; // B
        pixels[i + 3] = 255; // A
    }
    
    poki::Buffer texStagingBuffer = m_resourceAllocator.createBuffer(
        {
            .size = texSize,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive
        },
        {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
            .usage = vma::MemoryUsage::eAuto
        }
    );
    std::memcpy(texStagingBuffer.mapping, pixels.data(), texSize);
    // stbi_image_free(pixels);

    vk::ImageCreateInfo texCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .extent = {static_cast<uint32_t>(textWidth), static_cast<uint32_t>(texHeight), 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };
    vk::ImageViewCreateInfo texViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
    };
    m_texture = m_resourceAllocator.createImage(texCreateInfo, texViewCreateInfo);

    // Storage Image for Compute Shader
    vk::ImageCreateInfo storageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR8G8B8A8Unorm,
        .extent = {static_cast<uint32_t>(m_swapchain.getExtent().width), static_cast<uint32_t>(m_swapchain.getExtent().height), 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };
    vk::ImageViewCreateInfo storageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Unorm,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
    };
    m_gradient = m_resourceAllocator.createImage(storageCreateInfo, storageViewCreateInfo);

    // Sampler
    vk::PhysicalDeviceProperties properties = m_context.getPhysicalDeviceRAII().getProperties();
    vk::SamplerCreateInfo        samplerInfo{.magFilter        = vk::Filter::eLinear,
                                            .minFilter        = vk::Filter::eLinear,
                                            .mipmapMode       = vk::SamplerMipmapMode::eLinear,
                                            .addressModeU     = vk::SamplerAddressMode::eRepeat,
                                            .addressModeV     = vk::SamplerAddressMode::eRepeat,
                                            .addressModeW     = vk::SamplerAddressMode::eRepeat,
                                            .mipLodBias       = 0.0f,
                                            .anisotropyEnable = vk::True,
                                            .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
                                            .compareEnable    = vk::False,
                                            .compareOp        = vk::CompareOp::eAlways};
    m_texSampler = vk::raii::Sampler(m_context.getDeviceRAII(), samplerInfo);

    m_texture.sampler = *m_texSampler;
    m_gradient.sampler = *m_texSampler;

    // Since we only perform the compute operation once to calculate the gradient,
    // we need to call `Device::updateDescriptorSets()` so that the compute shader knows the descriptor layout.
    poki::WriteSetContainer compWriteSet;
    compWriteSet.append(m_compDescriptorSetContainer.makeWriteSet(0, 0), m_gradient, vk::ImageLayout::eGeneral);
    m_context.getDeviceRAII().updateDescriptorSets(compWriteSet.data(), {});
    
    // Perform two independent operations during preprocessing:
    //      1. Calculate the gradient and store it in `m_gradient`.
    //      2. Transfer the texture data from `texStagingBuffer` to `m_texture`.
    // These two do not depend on each other, so no synchronization is required between them
    { 
        auto singleTimeCmd{poki::createSingleTimeCommands(m_context.getDeviceRAII(), tempCmdPool)};

        // Calculate gradient
        poki::cmdImageMemoryBarrier(singleTimeCmd, m_gradient, {
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eGeneral
        });

        singleTimeCmd.bindPipeline(vk::PipelineBindPoint::eCompute, *m_computePipeline);
        singleTimeCmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *m_computePipelineLayout, 0, *m_compDescriptorSetContainer.getSet(0), nullptr);
        singleTimeCmd.dispatch((m_swapchain.getExtent().width + 15) / 16, (m_swapchain.getExtent().height + 15) / 16, 1);

        poki::cmdImageMemoryBarrier(singleTimeCmd, m_gradient, {
            .oldLayout = vk::ImageLayout::eGeneral,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        });

        // Copy the texture from staging buffer to image
        poki::cmdImageMemoryBarrier(singleTimeCmd, m_texture, {
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal
        });

        vk::BufferImageCopy2 pRegions{
            .bufferOffset = 0, 
            .bufferRowLength = 0, 
            .bufferImageHeight = 0,
            .imageSubresource = { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, 
            .imageOffset = {0, 0, 0}, 
            .imageExtent = {static_cast<uint32_t>(textWidth), static_cast<uint32_t>(texHeight), 1} 
        };
        vk::CopyBufferToImageInfo2 copyBufferToImageInfo{
            .srcBuffer = texStagingBuffer.buffer,
            .dstImage  = m_texture.image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &pRegions
        };
        singleTimeCmd.copyBufferToImage2(copyBufferToImageInfo);

        poki::cmdImageMemoryBarrier(singleTimeCmd, m_texture, {
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        });

        poki::engSingleTimeCommands(singleTimeCmd, m_context.getDeviceRAII(), m_context.getQueueInfo(0).queue);
    }
    
    poki::WriteSetContainer graphicsWriteSet;
    for (auto i{0}; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        graphicsWriteSet.append(m_fifDescriptorSetContainer.makeWriteSet(0, i), m_uniformBuffers[i]);
    }
    graphicsWriteSet.append(m_textureDescriptorSetContainer.makeWriteSet(0, 0), m_texture);
    graphicsWriteSet.append(m_textureDescriptorSetContainer.makeWriteSet(1, 0), m_gradient);
    m_context.getDeviceRAII().updateDescriptorSets(graphicsWriteSet.data(), {});
}

void Application::run() {
    while (!m_window.wantToClose()) {
        m_window.PollEvents();
        drawFrame();
    }
    m_context.getDeviceRAII().waitIdle();
}

void Application::drawFrame() {
    // Recreate swapchain
    if (m_swapchain.needRebuild()) {
        m_swapchain.reinitResources(m_window.getGLFWHandle());
        return;
    }

    // wait for last frame (timelinesemaphore)
    uint64_t waitValue = m_timelineSemaphore.getWaitValue(MAX_FRAMES_IN_FLIGHT);
    auto semaphoreResult = m_context.getDeviceRAII().waitSemaphores(
        vk::SemaphoreWaitInfo{
            .semaphoreCount = 1,
            .pSemaphores    = &*m_timelineSemaphore.get(),
            .pValues        = &waitValue
        }, 
        std::numeric_limits<uint64_t>::max()
    );
    //

    // Get the current frame in flight and reset its command buffer
    uint64_t frameIndex = m_timelineSemaphore.getCurrentValue() % MAX_FRAMES_IN_FLIGHT;
    m_ManagedCommandPools.resetPool(frameIndex);

    // Acquire swapchain image
    auto [result, imageIndex] = m_swapchain.acquireNextImage(frameIndex);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        m_swapchain.requestRebuild();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    // ubo
    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time        = std::chrono::duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    // ubo.model = glm::mat4(1.0f);
    // ubo.view = glm::mat4(1.0f);
    // ubo.proj = glm::mat4(1.0f);
    ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view  = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj  = glm::perspective(glm::radians(45.0f), static_cast<float>(m_swapchain.getExtent().width) / static_cast<float>(m_swapchain.getExtent().height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    std::memcpy(m_uniformBuffers[frameIndex].mapping, &ubo, sizeof(ubo));

    // Record command buffer
    auto& cmd = m_ManagedCommandPools.acquireCommandBuffer(frameIndex);
    { // Rendering
        cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    
        poki::cmdImageMemoryBarrier(cmd, {
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .image = m_swapchain.getImage(imageIndex)
        });
    
        vk::ClearValue clearColor{vk::ClearColorValue(0.346704f, 0.337164f, 0.558340f, 1.0f)};
        vk::RenderingAttachmentInfo attachmentInfo{
            .imageView = m_swapchain.getImageView(imageIndex),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };
    
        vk::RenderingInfo renderingInfo{
            .renderArea = {.offset{0, 0}, .extent = m_swapchain.getExtent()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo
        };
    
        cmd.beginRendering(renderingInfo);
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.getPipelineRAII());
    
        cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapchain.getExtent().width), static_cast<float>(m_swapchain.getExtent().height)));
        cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapchain.getExtent()));
        cmd.bindVertexBuffers(0, *m_vertexBuffer.buffer, {0});
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, *m_fifDescriptorSetContainer.getSet(frameIndex), nullptr);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 1, *m_textureDescriptorSetContainer.getSet(0), nullptr);
        cmd.draw(3, 1, 0, 0);
        cmd.endRendering();

        poki::cmdImageMemoryBarrier(cmd, {
            .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .newLayout = vk::ImageLayout::ePresentSrcKHR,
            .image = m_swapchain.getImage(imageIndex)
        });    
        cmd.end();
    }

    auto& queue = m_context.getQueueInfo(0).queue;
    uint64_t signalValue = m_timelineSemaphore.nextSignalValue();

    vk::SemaphoreSubmitInfo waitImageAvail{
        .semaphore = *m_swapchain.getImageAvailableSemaphore(frameIndex),
        .value = 0,
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .deviceIndex = 0
    };
    vk::SemaphoreSubmitInfo signalGraphics{
        .semaphore = *m_timelineSemaphore.get(),
        .value     = signalValue,
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .deviceIndex = 0
    };
    vk::SemaphoreSubmitInfo signalRenderFinished{
        .semaphore = *m_swapchain.getRenderFinishedSemaphore(imageIndex),
        .value     = 0,
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .deviceIndex = 0
    };
    std::array<vk::SemaphoreSubmitInfo, 2> signalSemaphores = {signalGraphics, signalRenderFinished};

    vk::CommandBufferSubmitInfo cmdSubmitInfo{
        .commandBuffer = *cmd,
        .deviceMask = 0
    };

    const vk::SubmitInfo2 graphicsSubmitInfo{
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos    = &waitImageAvail,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos    = &cmdSubmitInfo,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphores.size()),
        .pSignalSemaphoreInfos    = signalSemaphores.data()
    };

    queue.submit2(graphicsSubmitInfo);

    auto presentResult = m_swapchain.presentFrame(imageIndex);
    if ((presentResult == vk::Result::eSuboptimalKHR) || (presentResult == vk::Result::eErrorOutOfDateKHR)) {
        m_swapchain.requestRebuild();
        return;
    } else {
        assert(presentResult == vk::Result::eSuccess);
    }
}
