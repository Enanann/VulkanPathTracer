#include "graphics_pipeline.hpp"

#include "log.hpp"
#include "shader.hpp"

namespace poki {

void GraphicsPipeline::init(const GraphicsPipelineInitInfo& initInfo) {
    // Shader stages
    vk::raii::ShaderModule shaderModule{poki::createShaderModule(initInfo.device, initInfo.shaderPath)};
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    for (const auto& stage : initInfo.shaderStages) {
        shaderStages.emplace_back(vk::PipelineShaderStageCreateInfo{
            .stage  = stage.stage,
            .module = shaderModule,
            .pName  = stage.pName.c_str()
        });
    }

    // Dynamic state
    std::vector<vk::DynamicState> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicCreateInfo {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates    = dynamicStates.data()
    };

    // Vertex Input
    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo {
        .vertexBindingDescriptionCount   = static_cast<uint32_t>(initInfo.vertexBindings.size()),
        .pVertexBindingDescriptions      = initInfo.vertexBindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(initInfo.vertexAttributes.size()),
        .pVertexAttributeDescriptions    = initInfo.vertexAttributes.data()
    };

    // Input Assembly
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
        .topology               = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False
    };

    // Viewport + Scissor
    vk::PipelineViewportStateCreateInfo viewportCreateInfo {
        .viewportCount = 1,
        .scissorCount  = 1
    };

    // Rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo {
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable         = vk::False,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f
    };

    // Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable  = vk::False
    };

    // Depth + Stencil
    vk::PipelineDepthStencilStateCreateInfo depthCreateInfo {
        .depthTestEnable  = initInfo.enableDepth,
        .depthWriteEnable = vk::True,
        .depthCompareOp   = vk::CompareOp::eLess
    };

    // Color blending
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable    = vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };
    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable   = vk::False, 
        .logicOp         = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments    = &colorBlendAttachment
    };

    // Pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .setLayoutCount         = 0,
        // .pSetLayouts            = nullptr,
        .pushConstantRangeCount = 0
    };
    m_pipelineLayout = vk::raii::PipelineLayout(initInfo.device, pipelineLayoutCreateInfo);

    // Graphics pipeline
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain {
        {
            .stageCount          = static_cast<uint32_t>(shaderStages.size()),
            .pStages             = shaderStages.data(),
            .pVertexInputState   = &vertexInputCreateInfo,
            .pInputAssemblyState = &inputAssemblyCreateInfo,
            .pViewportState      = &viewportCreateInfo,
            .pRasterizationState = &rasterizerCreateInfo,
            .pMultisampleState   = &multisampleCreateInfo,
            .pDepthStencilState  = &depthCreateInfo,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicCreateInfo,
            .layout              = m_pipelineLayout,
            .renderPass          = nullptr
        },
        {
            .colorAttachmentCount    = 1, 
            .pColorAttachmentFormats = &initInfo.colorFormat
        }
    };

    m_graphicsPipeline = vk::raii::Pipeline(initInfo.device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());

    LOGI("Graphics Pipeline created successfully");
}

}; // namespace poki
