#pragma once

#include "context.hpp"
#include "window.hpp"
#include "swapchain.hpp"
#include "graphics_pipeline.hpp"
#include "command_pool.hpp"
#include "semaphore.hpp"
#include "resources.hpp"
#include "resource_allocator.hpp"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class Application {
public:
    Application();

    void run();

    void drawFrame();

private:
    poki::Context             m_context;
    poki::Window              m_window;
    poki::Swapchain           m_swapchain;
    poki::GraphicsPipeline    m_graphicsPipeline;
    poki::ManagedCommandPools m_ManagedCommandPools;

    poki::TimelineSemaphore m_timelineSemaphore;

    poki::ResourceAllocator m_resouceAllocator;
    poki::Buffer            m_vertexBuffer;
};
