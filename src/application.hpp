#pragma once

#include "context.hpp"
#include "window.hpp"
#include "swapchain.hpp"
#include "graphics_pipeline.hpp"
#include "command_pool.hpp"
#include "semaphore.hpp"
#include "resources.hpp"
#include "resource_allocator.hpp"
#include "descriptors.hpp"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class Application {
public:
    Application();

    void run();

    void drawFrame();

private:
    poki::Context                m_context;
    poki::Window                 m_window;
    poki::Swapchain              m_swapchain;
    poki::DescriptorSetContainer m_fifDescriptorSetContainer;
    poki::DescriptorSetContainer m_textureDescriptorSetContainer;
    poki::DescriptorSetContainer m_compDescriptorSetContainer;
    vk::raii::PipelineLayout     m_computePipelineLayout{nullptr};
    vk::raii::Pipeline           m_computePipeline{nullptr};
    vk::raii::PipelineLayout     m_pipelineLayout{nullptr};
    poki::GraphicsPipeline       m_graphicsPipeline;
    poki::ManagedCommandPools    m_ManagedCommandPools;

    poki::TimelineSemaphore m_timelineSemaphore;

    poki::ResourceAllocator   m_resourceAllocator;
    poki::Buffer              m_vertexBuffer;
    std::vector<poki::Buffer> m_uniformBuffers;
    vk::raii::Sampler m_texSampler{nullptr};
    poki::Image m_texture;
    poki::Image m_gradient;
};
