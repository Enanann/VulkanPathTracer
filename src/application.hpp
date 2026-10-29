#pragma once

#include "config.hpp"
#include "context.hpp"
#include "window.hpp"
#include "swapchain.hpp"
#include "graphics_pipeline.hpp"
#include "command_pool.hpp"
#include "semaphore.hpp"
#include "resources.hpp"
#include "resource_allocator.hpp"
#include "descriptors.hpp"
#include "scene.hpp"

class Application {
public:
    Application();

    void run();

    void drawFrame();

private:
    poki::Context                m_context;
    poki::Window                 m_window;
    poki::Swapchain              m_swapchain;
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
    vk::raii::Sampler m_texSampler{nullptr};
    // poki::Image m_texture;
    poki::Image m_rt;

    poki::Scene  m_scene;
    poki::Buffer m_sceneBuffer;

    uint32_t m_samples{500};
    uint32_t m_sampleCounter{1};

    uint32_t m_currentFrameIndex{0};
};
