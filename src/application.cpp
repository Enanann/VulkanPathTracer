#include "application.hpp"

#include "barriers.hpp"
#include "vulkan/vulkan.hpp"

#include <array>
#include <limits>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

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

    std::vector<poki::ShaderStageInfo> shaderStages{{vk::ShaderStageFlagBits::eVertex, "vertMain"}, {vk::ShaderStageFlagBits::eFragment, "fragMain"}};
    poki::GraphicsPipelineInitInfo graphicsPipelineInitInfo{
        .device = m_context.getDeviceRAII(),
        .shaderPath = "build/src/Shaders/shader_base.spv",
        .shaderStages = shaderStages,
        .colorFormat = m_swapchain.getSwapchainImageFormat(),
        .enableDepth = vk::False
    };
    m_graphicsPipeline.init(graphicsPipelineInitInfo);

    poki::ManagedCommandPoolsInitInfo managedCommandPoolsInitInfo {
        .device           = m_context.getDeviceRAII(),
        .queueFamilyIndex = m_context.getQueueInfo(0).familyIndex,
        .maxPoolCount     = MAX_FRAMES_IN_FLIGHT
    };
    m_ManagedCommandPools.init(managedCommandPoolsInitInfo);

    m_timelineSemaphore = poki::TimelineSemaphore(m_context.getDeviceRAII(), 0);
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

    // Record command buffer
    auto& cmd = m_ManagedCommandPools.acquireCommandBuffer(frameIndex);
    { // Rendering
        cmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    
        poki::cmdImageMemoryBarrier(cmd, {
            .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .srcAccessMask = {},
            .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = m_swapchain.getImage(imageIndex),
            // .subresourceRange = {}
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
        cmd.draw(3, 1, 0, 0);
        cmd.endRendering();

        poki::cmdImageMemoryBarrier(cmd, {
            .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
            .dstAccessMask = {},
            .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .newLayout = vk::ImageLayout::ePresentSrcKHR,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = m_swapchain.getImage(imageIndex),
            // .subresourceRange = {}
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
    } else {
        assert(presentResult == vk::Result::eSuccess);
    }
}
