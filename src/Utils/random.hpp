#pragma once

#include "glm/ext/vector_float3.hpp"
#include <random>

inline float random_float() {
    static std::uniform_real_distribution<float> distribution(0.0, 1.0);
    static std::mt19937 gen;
    return distribution(gen);
} 

inline float random_float(float min, float max) {
    return (max - min) * random_float() + min;
}

inline glm::vec3 random_vec3() {
    return glm::vec3(random_float(), random_float(), random_float());
}
