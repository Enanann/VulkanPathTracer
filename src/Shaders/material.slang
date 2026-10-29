module material;

import random;
import ray;
import hittable;

namespace Lambertian {
    // Always scatter
    bool scatter(const Ray r_in, inout HitRecord rec, inout float3 attenuation, inout Ray scattered, inout Random rng) {
        float3 scatterDirection = rec.m_normal + rng.random_unit_vec();
        // Check for near zero in all dimensions
        if ((fabs(scatterDirection.x) < 1e-8) && (fabs(scatterDirection.y) < 1e-8) && (fabs(scatterDirection.z) < 1e-8)) {
            scatterDirection = rec.m_normal;
        }
        
        scattered = Ray(rec.m_p, scatterDirection);
        attenuation = rec.m_mat.albedo;
        
        return true;
    }
}

namespace Metal {
    bool scatter(const Ray r_in, inout HitRecord rec, inout float3 attenuation, inout Ray scattered, inout Random rng) {
        float3 reflected = reflect(r_in.dir(), rec.m_normal);
        reflected = normalize(reflected) + (rec.m_mat.roughness * rng.random_unit_vec());
        scattered = Ray(rec.m_p, reflected);
        attenuation = rec.m_mat.albedo;
        
        return dot(scattered.dir(), rec.m_normal) > 0; // Check if the scattered ray is still on the correct hemisphere
    }
}

namespace Dielectric {
    bool scatter(const Ray r_in, inout HitRecord rec, inout float3 attenuation, inout Ray scattered, inout Random rng) {
        attenuation = float3(1.0);
        
        float ri = rec.m_front_face ? (1.0 / rec.m_mat.refractionIndex) : rec.m_mat.refractionIndex;
        float3 unit_dir = normalize(r_in.dir());
        float cosTheta = fmin(dot(-unit_dir, rec.m_normal), 1.0);
        float sinTheta = sqrt(1 - cosTheta * cosTheta);

        float3 dir;
        if (ri * sinTheta > 1.0 || reflectance(cosTheta, ri) > rng.random_float()) { /*Cannot refract*/
            dir = reflect(unit_dir, rec.m_normal);
        } else { /*Can refract*/
            dir = refract(unit_dir, rec.m_normal, ri);
        }

        scattered = Ray(rec.m_p, dir);

        return true;
    }

    // Schlick's approximation for reflectance
    float reflectance(float cosine, float ri) {
        float r0 = (1 - ri) / (1 + ri);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cosine), 5);
    }
}

public bool scatter(const Ray r_in, inout HitRecord rec, inout float3 attenuation, inout Ray scattered, inout Random rng) {
    if (rec.m_mat.type == MaterialType::Lambertian) {
        return Lambertian::scatter(r_in, rec, attenuation, scattered, rng);
    }
    if (rec.m_mat.type == MaterialType::Metal) {
        return Metal::scatter(r_in, rec, attenuation, scattered, rng);
    }
    if (rec.m_mat.type == MaterialType::Dielectric) {
        return Dielectric::scatter(r_in, rec, attenuation, scattered, rng);
    }
    return false;
}

