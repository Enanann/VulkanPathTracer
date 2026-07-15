cmake_minimum_required(VERSION 4.0...4.4)

project(
    VulkanPathTracer
    VERSION 1.0
    LANGUAGES CXX
)

# Set CMAKE_ variables
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_package (Vulkan REQUIRED)

# glfw
add_subdirectory(third_party/GLFW)

# glm
add_subdirectory(third_party/glm)

# imgui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui.cpp

    # backend
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

# stb
add_subdirectory(third_party/stb)

# vma
add_subdirectory(third_party/vma)


add_subdirectory(src)
