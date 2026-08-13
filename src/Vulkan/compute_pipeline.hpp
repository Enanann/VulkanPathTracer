#pragma once

#include <vulkan/vulkan.hpp>

// Only contain helper function, user will need to create vk::raii::PipelineLayout and vk::Pipeline explicitly
namespace poki {

// Return the number of workgroups needed for the given image size and local workgroup size
inline vk::Extent3D getGroupCounts(const vk::Extent2D& size, const vk::Extent2D& localWorkGroupSize) {
    return vk::Extent3D{
        .width  = (size.width  + localWorkGroupSize.width  - 1) / localWorkGroupSize.width,
        .height = (size.height + localWorkGroupSize.height - 1) / localWorkGroupSize.height,
        .depth  = 1,
    };
}

inline vk::Extent3D getGroupCounts(const vk::Extent3D& size, const vk::Extent3D& localWorkGroupSize) {
    return vk::Extent3D{
        .width  = (size.width  + localWorkGroupSize.width  - 1) / localWorkGroupSize.width,
        .height = (size.height + localWorkGroupSize.height - 1) / localWorkGroupSize.height,
        .depth  = (size.depth  + localWorkGroupSize.depth  - 1) / localWorkGroupSize.depth,
    };
}


}; // namespace poki
