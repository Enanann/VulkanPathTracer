#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <ios>
#include <span>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace poki {

// Read the SPIR-V bytecode
[[nodiscard]] inline std::vector<uint32_t> readSpirV(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file " + path.string() + "\nCurrent working directory: " + std::filesystem::current_path().string());
    }

    const auto size{file.tellg()};
    if (size % sizeof(uint32_t) != 0) {
        throw std::runtime_error("Invalid SPIR-V file size"); // SPIR-V must be a multiple of 4
    }

    std::vector<uint32_t> code(static_cast<size_t>(size) / sizeof(uint32_t));

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(size));
    file.close();

    return code;
}

// Create a shader module
[[nodiscard]] inline vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::span<const uint32_t>& code /*SPIR-V*/) {
    const vk::ShaderModuleCreateInfo createInfo {
        .codeSize = code.size_bytes(),
        .pCode    = code.data()
    };

    return vk::raii::ShaderModule(device, createInfo);
}

//--------------------------------------------------------------------------------------------------
// Overload for createShaderModule(const vk::raii::Device& device, const std::span<const uint32_t>& code)
[[nodiscard]] inline vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::vector<uint32_t>& code /*SPIR-V*/) {
    return createShaderModule(device, std::span<const uint32_t> {code});
}

[[nodiscard]] inline vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::filesystem::path& path) {
    return createShaderModule(device, readSpirV(path));
}

}; // end of namespace poki
