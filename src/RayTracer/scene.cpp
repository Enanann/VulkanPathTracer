#include "scene.hpp"
#include "material.hpp"
#include "random.hpp"

#include "glm/geometric.hpp"

namespace poki {

void Scene::build() {
    // static_assert(sizeof(Sphere) == 48);
    addSphere({{0.0f, -1000.0f, 0.0f, 1000.0f}, {{0.5f, 0.5f, 0.5f}, 0.0f, 0.0f, MaterialType::Lambertian}});    // Ground
    addSphere({{-4.0f, 1.0f, 0.0f, 1.0f}, {{0.4f, 0.2f, 0.1f}, 0.0f, 0.0f, MaterialType::Lambertian}});          // Left
    addSphere({{0.0f, 1.0f, -0.0f, 1.0f}, {{0.0f, 0.0f, 0.0f}, 0.0f, 1.5f, MaterialType::Dielectric}});          // Middle
    addSphere({{4.0f, 1.0f, 0.0f, 1.0f}, {{0.7f, 0.6f, 0.5f}, 0.0f, 0.0f, MaterialType::Metal}});                // Right

    const float minDistance = 1.2f;

    for (int a{-11}; a < 11; ++a) {
        for (int b{-11}; b < 11; ++b) {
            auto choose_mat = random_float();
            glm::vec3 center(a + 0.9f * random_float(), 0.2f, b + 0.9f * random_float());

            const bool awayFromLeft = glm::length(center - glm::vec3(-4.0f, 1.0f, 0.0f)) > minDistance;
            const bool awayFromMiddle = glm::length(center - glm::vec3(0.0f, 1.0f, 0.0f)) > minDistance;
            const bool awayFromRight = glm::length(center - glm::vec3(4.0f, 1.0f, 0.0f)) > minDistance;

            if (awayFromLeft && awayFromMiddle && awayFromRight) {
                if (choose_mat < 0.8f) {             /*Diffuse*/
                    auto albedo = random_vec3() * random_vec3();
                    addSphere({glm::vec4(center, 0.2f), {albedo, 0.0f, 0.0f, MaterialType::Lambertian}});          
                } else if (choose_mat < 0.95f) {     /*Metal*/
                    auto albedo = random_vec3() * random_vec3();
                    auto fuzz = random_float(0.0f, 0.5f);
                    addSphere({glm::vec4(center, 0.2f), {albedo, fuzz, 0.0f, MaterialType::Metal}});         
                } else {                            /*Glass*/
                    addSphere({glm::vec4(center, 0.2f), {{0.0f, 0.0f, 0.0f}, 0.0f, 1.5f, MaterialType::Dielectric}});         
                }
            }
        }
    }
}

}; // namespace poki
