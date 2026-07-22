set(SOURCES
    main.cpp

    application.cpp

    # ImGui
    ${IMGUI_SOURCES}

    # Stb
    Stb/StbUsage.cpp

    # Window
    Window/window.cpp
    
    # Vulkan
    Vulkan/VmaUsage.cpp
    Vulkan/context.cpp
    Vulkan/command_pool.cpp
    Vulkan/command_pool.cpp
    Vulkan/command_pool.cpp
    Vulkan/swapchain.cpp
    Vulkan/graphics_pipeline.cpp
    Vulkan/command_pool.cpp
    Vulkan/commands.cpp
    Vulkan/semaphore.cpp 
    Vulkan/barriers.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_definitions(${PROJECT_NAME}
    PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1 
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../third_party/GLFW/include
    ../third_party/glm
    ${IMGUI_DIR}
    ../third_party/stb/include
    ../third_party/vma/include
    Utils
    Vulkan
    Window
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glfw
    glm-header-only
    stb_image
    VulkanMemoryAllocator
)

add_slang_shader_target(
    ${PROJECT_NAME}_shaders 
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/shader_base.slang"
    ENTRY_POINTS 
        "vertMain"
        "fragMain"
)
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_shaders)
