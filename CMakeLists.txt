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

# Custom shaders compilation
add_executable(glslang::validator IMPORTED)
find_program (GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
set_property (TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

function (add_slang_shader_target TARGET)
    # Parse the SOURCES argument    
    cmake_parse_arguments ("SHADER" "" "" "SOURCES;ENTRY_POINTS" ${ARGN})
  
    # Set output directory
    set (SHADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/Shaders)
    
    # Entry points:
    set(SLANG_ENTRY_ARGS)
    foreach(entry ${SHADER_ENTRY_POINTS})
        list(APPEND SLANG_ENTRY_ARGS -entry ${entry})
    endforeach()
    

    # Create directory
    add_custom_command (
            OUTPUT ${SHADERS_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )

    foreach(SRC ${SHADER_SOURCES})

        # Extract filename without extension
        get_filename_component(SHADER_NAME ${SRC} NAME_WE)

        # Output file
        set(OUT_FILE ${SHADERS_DIR}/${SHADER_NAME}.spv)

        list(APPEND OUTPUT_FILES ${OUT_FILE})

        add_custom_command(
            OUTPUT ${OUT_FILE}
            COMMAND ${SLANGC_EXECUTABLE} ${SRC}
                    -target spirv
                    -profile spirv_1_4+spvRayQueryKHR
                    -emit-spirv-directly
                    -fvk-use-entrypoint-name ${SLANG_ENTRY_ARGS}
                    -o ${OUT_FILE}
            WORKING_DIRECTORY ${SHADERS_DIR}
            DEPENDS ${SHADERS_DIR} ${SRC}
            COMMENT "Compiling Slang shader ${SRC}"
            VERBATIM
        )
    endforeach()

    # Create the build target
    add_custom_target (${TARGET} DEPENDS ${OUTPUT_FILES})
endfunction()

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
