#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

#include <array>
#include <vector>

struct Vertex {
    glm::vec2 position;
    glm::vec3 color;
    glm::vec2 texCoord;

    static std::vector<vk::VertexInputBindingDescription> getBindingDescription() {
        return {{
            .binding   = 0,
            .stride    = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        }};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription{
                .location = 0,
                .binding  = 0,
                .format   = vk::Format::eR32G32Sfloat,
                .offset   = offsetof(Vertex, position)
            },
            vk::VertexInputAttributeDescription{
                .location = 1,
                .binding  = 0,
                .format   = vk::Format::eR32G32B32Sfloat,
                .offset   = offsetof(Vertex, color)
            },
            vk::VertexInputAttributeDescription{
                .location = 2,
                .binding  = 0,
                .format   = vk::Format::eR32G32Sfloat,
                .offset   = offsetof(Vertex, texCoord)
            }
        };
    }
};
