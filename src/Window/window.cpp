#include "window.hpp"

#include "log.hpp"

#include <format>
#include <stdexcept>
#include <vulkan/vulkan_to_string.hpp>

namespace poki {
    
poki::Window::~Window() {
    glfwDestroyWindow(m_window);

    glfwTerminate();
}

void poki::Window::init(const WindowInitInfo& windowInitInfo) {
    windowInfo = windowInitInfo;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, (windowInitInfo.resizable) ? GLFW_TRUE : GLFW_FALSE);

    m_window = glfwCreateWindow(windowInitInfo.width, windowInitInfo.height, windowInitInfo.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        throw std::runtime_error("Failed to create glfw window");
    }

    glfwSetWindowUserPointer(m_window, this);
    
    LOGI("Create GLFW window successfully");
    LOGI(" - Window size : {}x{}", windowInitInfo.width, windowInitInfo.height);
    LOGI(" - Title       : {}", windowInitInfo.title);
}

vk::raii::SurfaceKHR poki::Window::createSurface(const vk::raii::Instance& instance) const {
    VkSurfaceKHR _surface{};
    auto result = glfwCreateWindowSurface(*instance, m_window, nullptr, &_surface);
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::format("Failed to create window surface ({})", vk::to_string(static_cast<vk::Result>(result))));
    }

    return vk::raii::SurfaceKHR(instance, _surface);
}

void poki::Window::PollEvents() {
    glfwPollEvents();
}

void poki::Window::WaitEvents() {
    glfwWaitEvents();
}

bool poki::Window::wantToClose() const {
    return glfwWindowShouldClose(m_window);
}

}; // namespace poki
