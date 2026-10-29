#include "application.hpp"

#include "barriers.hpp"
#include "commands.hpp"
#include "descriptors.hpp"
#include "shader.hpp"
#include "vertex.hpp"
#include "stb_image.h"
#include "compute_pipeline.hpp"

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
        // .instanceExtensions = {vk::KHRGetSurfaceCapabilities2ExtensionName}, 
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
    poki::DescriptorBindings textureBinding; // The same across multiple FiF (Texture) 
    textureBinding.addBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    m_textureDescriptorSetContainer.init(textureBinding, m_context.getDeviceRAII(), 1, {}, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    std::vector<vk::DescriptorSetLayout> descriptorSetLayout = {*m_textureDescriptorSetContainer.getLayout()};
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
    bindings_2.addBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute); /*SSBO with scene data do not need to be duplicated per frame*/
    m_compDescriptorSetContainer.init(bindings_2, m_context.getDeviceRAII(), 1, {}, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    std::vector<vk::DescriptorSetLayout> compDescriptorSetLayout = {*m_compDescriptorSetContainer.getLayout()};

    // Compute pipeline
    vk::PipelineLayoutCreateInfo computePipelineLayoutCreateInfo{
        .setLayoutCount = static_cast<uint32_t>(compDescriptorSetLayout.size()),
        .pSetLayouts    = compDescriptorSetLayout.data(),
        .pushConstantRangeCount = 1,
        // .pPushConstantRanges =
    };
    vk::PushConstantRange range = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset     = 0,
        .size       = 4
    };
    computePipelineLayoutCreateInfo.setPPushConstantRanges(&range);
    m_computePipelineLayout = vk::raii::PipelineLayout(m_context.getDeviceRAII(), computePipelineLayoutCreateInfo);

    auto compShader = poki::createShaderModule(m_context.getDeviceRAII() ,"build/src/Shaders/rtiow.spv");
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

    // Storage Image for Compute Shader
    vk::ImageCreateInfo storageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR32G32B32A32Sfloat,
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
        .format = vk::Format::eR32G32B32A32Sfloat,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
    };
    m_rt = m_resourceAllocator.createImage(storageCreateInfo, storageViewCreateInfo);

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

    m_rt.sampler = *m_texSampler;

    // Scene
    m_scene.build();
    vk::DeviceSize storageBufferSize = m_scene.size() * sizeof(Sphere);
    poki::Buffer storageStagingBuffer = m_resourceAllocator.createBuffer(
        {
            .size = storageBufferSize,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive
        },
        {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
            .usage = vma::MemoryUsage::eAuto
        }
    );
    std::memcpy(storageStagingBuffer.mapping, m_scene.data().data(), storageBufferSize);

    m_sceneBuffer = m_resourceAllocator.createBuffer(
        {
            .size = storageBufferSize,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            .sharingMode = vk::SharingMode::eExclusive
        },
        {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
            .usage = vma::MemoryUsage::eAuto
        }
    );

    // Since we only perform the compute operation once to calculate the gradient,
    // we need to call `Device::updateDescriptorSets()` so that the compute shader knows the descriptor layout.
    poki::WriteSetContainer compWriteSet;
    compWriteSet.append(m_compDescriptorSetContainer.makeWriteSet(0, 0), m_rt, vk::ImageLayout::eGeneral);
    compWriteSet.append(m_compDescriptorSetContainer.makeWriteSet(1, 0), m_sceneBuffer);
    m_context.getDeviceRAII().updateDescriptorSets(compWriteSet.data(), {});

    poki::WriteSetContainer graphicsWriteSet;
    graphicsWriteSet.append(m_textureDescriptorSetContainer.makeWriteSet(0, 0), m_rt, vk::ImageLayout::eShaderReadOnlyOptimal);
    m_context.getDeviceRAII().updateDescriptorSets(graphicsWriteSet.data(), {});
    
    // Perform two independent operations during preprocessing:
    //      1. Transfer the scene data from `storageStageBuffer` to `m_storageBuffer`
    //      2. Calculate the -gradient- scene and store it in `m_gradient`.
    // 1 must be done before 2, so we will need to make a buffer memory barrier
    { 
        auto compGroupCounts{poki::getGroupCounts(m_swapchain.getExtent(), {16, 16})};
        auto singleTimeCmd{poki::createSingleTimeCommands(m_context.getDeviceRAII(), tempCmdPool)};

        // Transfer scene data
        vk::BufferCopy copyRegion{
            .srcOffset = 0,
            .dstOffset = 0,
            .size      = storageBufferSize
        };
        singleTimeCmd.copyBuffer(storageStagingBuffer.buffer, m_sceneBuffer.buffer, copyRegion);
        poki::cmdBufferMemoryBarrier(singleTimeCmd, {
            .srcStageMask  = vk::PipelineStageFlagBits2::eTransfer, 
            .dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
            .buffer        = m_sceneBuffer.buffer
        });

        // // Calculate -gradient- scene
        // poki::cmdImageMemoryBarrier(singleTimeCmd, m_rt, {
        //     .oldLayout = vk::ImageLayout::eUndefined,
        //     .newLayout = vk::ImageLayout::eGeneral
        // });

        // vk::PushConstantsInfo pcInfo{
        //     .layout = m_computePipelineLayout,
        //     .stageFlags = vk::ShaderStageFlagBits::eCompute,
        //     .offset = 0,
        //     .size = sizeof(uint32_t),
        //     .pValues = &m_samples
        // };
        // singleTimeCmd.bindPipeline(vk::PipelineBindPoint::eCompute, *m_computePipeline);
        // singleTimeCmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *m_computePipelineLayout, 0, *m_compDescriptorSetContainer.getSet(0), nullptr);
        // singleTimeCmd.pushConstants2(pcInfo);
        // singleTimeCmd.dispatch(compGroupCounts.width, compGroupCounts.height, compGroupCounts.depth);


        // poki::cmdImageMemoryBarrier(singleTimeCmd, m_rt, {
        //     .oldLayout = vk::ImageLayout::eGeneral,
        //     .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        // });
        
        poki::engSingleTimeCommands(singleTimeCmd, m_context.getDeviceRAII(), m_context.getQueueInfo(0).queue);
    } 
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

    // Get the current FiF's index
    uint64_t frameIndex = m_currentFrameIndex % MAX_FRAMES_IN_FLIGHT;
    m_currentFrameIndex++;

    // wait for last frame (timelinesemaphore)
    uint64_t lastFrameWaitValue = m_timelineSemaphore.getWaitValue(frameIndex);
    auto waitLastFrameResult = m_context.getDeviceRAII().waitSemaphores(
        vk::SemaphoreWaitInfo{
            .semaphoreCount = 1,
            .pSemaphores    = &*m_timelineSemaphore.get(),
            .pValues        = &lastFrameWaitValue
        }, 
        std::numeric_limits<uint64_t>::max()
    );
    //

    // Get the current frame in flight and reset its command buffer
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

    // Timeline semaphore values
    uint64_t lastSignaledValue = m_timelineSemaphore.getCurrentValue();
    uint64_t computeSignalValue = m_timelineSemaphore.nextSignalValue();
    uint64_t graphicsSignalValue = m_timelineSemaphore.nextSignalValue();

    // Record Compute command buffer
    if (m_sampleCounter <= m_samples) {
        { // Compute
            auto& computeCmd = m_ManagedCommandPools.acquireCommandBuffer(frameIndex);
            computeCmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    
            auto compGroupCounts{poki::getGroupCounts(m_swapchain.getExtent(), {16, 16})};
    
            vk::PushConstantsInfo pcInfo{
                .layout = m_computePipelineLayout,
                .stageFlags = vk::ShaderStageFlagBits::eCompute,
                .offset = 0,
                .size = sizeof(m_sampleCounter),
                .pValues = &m_sampleCounter
            };
    
            // Change layout of m_rt from the previous frame
            poki::cmdImageMemoryBarrier(computeCmd, m_rt,  {
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eGeneral
            });
    
            computeCmd.bindPipeline(vk::PipelineBindPoint::eCompute, *m_computePipeline);
            computeCmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *m_computePipelineLayout, 0, *m_compDescriptorSetContainer.getSet(0), nullptr);
            computeCmd.pushConstants2(pcInfo);
            computeCmd.dispatch(compGroupCounts.width, compGroupCounts.height, compGroupCounts.depth);
    
            // Change layout of m_rt for graphics recording
            poki::cmdImageMemoryBarrier(computeCmd, m_rt,  {
                .oldLayout = vk::ImageLayout::eGeneral,
                .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            });
    
            computeCmd.end();
    
            auto& queue = m_context.getQueueInfo(0).queue;
    
            vk::SemaphoreSubmitInfo waitCompute{
                .semaphore   = *m_timelineSemaphore.get(),
                .value       = lastSignaledValue, 
                .stageMask   = vk::PipelineStageFlagBits2::eComputeShader,
                .deviceIndex = 0
            };
    
            vk::SemaphoreSubmitInfo signalCompute{
                .semaphore   = *m_timelineSemaphore.get(),
                .value       = computeSignalValue,
                .stageMask   = vk::PipelineStageFlagBits2::eComputeShader,
                .deviceIndex = 0
            };
    
            vk::CommandBufferSubmitInfo cmdSubmitInfo{
                .commandBuffer = *computeCmd,
                .deviceMask = 0
            };
    
            const vk::SubmitInfo2 computeSubmitInfo{
                .waitSemaphoreInfoCount = 1,
                .pWaitSemaphoreInfos    = &waitCompute,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos    = &cmdSubmitInfo,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos    = &signalCompute
            };
        
            queue.submit2(computeSubmitInfo);
        }
        m_sampleCounter++;
    } else {
        computeSignalValue = 0; // prevent the graphics from waiting indefinitely (stuck at frame = `m_samples`)
    }

    // Record Graphics command buffer
    { // Rendering
        auto& graphicsCmd = m_ManagedCommandPools.acquireCommandBuffer(frameIndex);
        graphicsCmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    
        poki::cmdImageMemoryBarrier(graphicsCmd, {
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
    
        graphicsCmd.beginRendering(renderingInfo);
        graphicsCmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline.getPipelineRAII());
    
        graphicsCmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapchain.getExtent().width), static_cast<float>(m_swapchain.getExtent().height)));
        graphicsCmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapchain.getExtent()));
        graphicsCmd.bindVertexBuffers(0, *m_vertexBuffer.buffer, {0});
        graphicsCmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, *m_textureDescriptorSetContainer.getSet(0), nullptr);
        graphicsCmd.draw(3, 1, 0, 0);
        graphicsCmd.endRendering();

        poki::cmdImageMemoryBarrier(graphicsCmd, {
            .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .newLayout = vk::ImageLayout::ePresentSrcKHR,
            .image = m_swapchain.getImage(imageIndex)
        });    
        graphicsCmd.end();
    
        auto& queue = m_context.getQueueInfo(0).queue;
    
        vk::SemaphoreSubmitInfo waitImageAvail{
            .semaphore = *m_swapchain.getImageAvailableSemaphore(frameIndex),
            .value = 0,
            .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .deviceIndex = 0
        };
        vk::SemaphoreSubmitInfo waitComputeFinished{
            .semaphore = *m_timelineSemaphore.get(),
            .value     = computeSignalValue,
            .stageMask = vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eFragmentShader,
            .deviceIndex = 0
        };
        std::array<vk::SemaphoreSubmitInfo, 2> waitSemaphores = {waitImageAvail, waitComputeFinished};

        vk::SemaphoreSubmitInfo signalGraphics{
            .semaphore = *m_timelineSemaphore.get(),
            .value     = graphicsSignalValue,
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
            .commandBuffer = *graphicsCmd,
            .deviceMask = 0
        };
    
        const vk::SubmitInfo2 graphicsSubmitInfo{
            .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphoreInfos    = waitSemaphores.data(),
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos    = &cmdSubmitInfo,
            .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphoreInfos    = signalSemaphores.data()
        };
    
        queue.submit2(graphicsSubmitInfo);
        
        m_timelineSemaphore.getFrameCompletionArray()[frameIndex] = graphicsSignalValue;
    }

    auto presentResult = m_swapchain.presentFrame(imageIndex);
    if ((presentResult == vk::Result::eSuboptimalKHR) || (presentResult == vk::Result::eErrorOutOfDateKHR)) {
        m_swapchain.requestRebuild();
        return;
    } else {
        assert(presentResult == vk::Result::eSuccess);
    }
}
