set(SOURCES
    main.cpp

    # imgui
    ${IMGUI_SOURCES}
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ../third_party/GLFW/include
    ../third_party/glm
    ${IMGUI_DIR}
    ../third_party/stb/include
    ../third_party/vma/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glfw
    glm-header-only
    stb_image
    VulkanMemoryAllocator
)
