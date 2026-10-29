#pragma once

#include "glm/ext/vector_float3.hpp"

#include <cstdint>

enum class MaterialType : uint32_t {
    Lambertian,
    Metal,
    Dielectric
};

struct Material {
    glm::vec3     m_albedo;
    float         m_roughness;
    float         m_refractionIndex;
    MaterialType  m_materialIndex;
};
