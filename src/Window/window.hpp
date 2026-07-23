#pragma once

#include <string>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

namespace poki {

// Struct for creating GLFW window
// width:     Window's width
// height:    Window's height
// resizable: Whether the window can be resize or not
// title:     Window's title
struct WindowInitInfo {
    int         width{};
    int         height{};
    bool        resizable{};
    std::string title{};
};

/**
 * @brief GLFW window class
 * 
 */
class Window {
public:
    Window() = default;
    ~Window();

    void init(const WindowInitInfo& windowInitInfo);

    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance) const;
    
    GLFWwindow* getGLFWHandle() const {return m_window;}
    int         getWidth() const {return windowInfo.width;}
    int         getHeight() const {return windowInfo.height;}
    bool        getResizeStatus() const {return m_resized;}
    
    static void PollEvents();
    static void WaitEvents();
    
    [[nodiscard]] bool wantToClose() const;
    
    WindowInitInfo windowInfo{};

public:
    // static callback function
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto _window{reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))};
        _window->m_resized = true;
    }

private:
    GLFWwindow* m_window{nullptr};
    
    bool m_resized{false};
};

}; // namespace poki
