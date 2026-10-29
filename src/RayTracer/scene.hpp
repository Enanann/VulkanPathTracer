#pragma once

#include "material.hpp"

#include "glm/ext/vector_float4.hpp"

#include <vector>

struct Sphere {
    glm::vec4             m_props; // (xyz: center, w: radius)
    alignas(16) Material  m_mat;
};

namespace poki {

/*

*/
class Scene {
public:
    void build();

    void addSphere(const Sphere& sphere) {m_spheres.push_back(sphere);}

    size_t size() const {return m_spheres.size();}

    std::vector<Sphere>& data() {return m_spheres;}

private:
    std::vector<Sphere> m_spheres;
};

}; // namespace poki
