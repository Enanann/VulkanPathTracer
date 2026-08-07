#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <span>
#include <string>
#include <filesystem>

namespace poki {

// Contains all the stages for a shader module and its respective entry point name
// e.g. {vk::ShaderStageFlagBits::eVertex, "vertMain"}, {vk::ShaderStageFlagBits::eFragment, "fragMain"}
struct ShaderStageInfo {
    vk::ShaderStageFlagBits stage;
    std::string             pName;
};

// TODO: Add support for descriptor sets and push constants
// Graphics pipeline creation info
// device          : The logical device
// shaderPath      : The compiled shader (SPIR-V) path (Right now only support Slang - can have multiple entry point in one file)
// shaderStages    : An std::span to a std::vector<ShaderStageInfo> (the user should be creating the vector)
// colorFormat     : Color attachment format for this pipeline (should be the swapchain image format)
// enableDepth     : Enable depth testing
// layout          : The pipelineLayout object (must outlive the vk::raii::Pipeline)
// vertexBindings  : The vertex bindings description (Describes the rate to load data from memory throughout the vertices)
// vertexAttributes: The vertex attributes (Describes how to extract a vertex attribute from a chunk of vertex data originating from a binding description)
struct GraphicsPipelineInitInfo {
    const vk::raii::Device&         device;
    std::filesystem::path           shaderPath;
    std::span<ShaderStageInfo>      shaderStages;
    vk::Format                      colorFormat;
    vk::Bool32                      enableDepth{vk::False};
    const vk::raii::PipelineLayout& layout;
    std::span<const vk::VertexInputBindingDescription>   vertexBindings;
    std::span<const vk::VertexInputAttributeDescription> vertexAttributes;
};

class GraphicsPipeline {
public:
    GraphicsPipeline() = default;

    void init(const GraphicsPipelineInitInfo& initInfo);

    const vk::raii::Pipeline& getPipelineRAII() const noexcept {return m_graphicsPipeline;}

private:
    vk::raii::Pipeline       m_graphicsPipeline{nullptr};
};


}; // namespace poki
