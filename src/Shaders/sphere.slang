module sphere;

import ray;
import hittable;

public struct Sphere : IHittable {
    public __init(float3 center, float radius, Material mat) {
        m_center = center;
        m_radius = radius;
        m_mat = mat;
    }

    public float3 center() { 
        return m_center;
    }

    bool hit(const Ray r, float t_min, float t_max, inout HitRecord hit_rec) {
        float3 qc = m_center - r.orig();
        float a = dot(r.dir(), r.dir());
        float h = -dot(r.dir(), qc);
        float c = dot(qc, qc) - m_radius * m_radius;

        float delta = h * h - a * c;
        if (delta < 0) {
            return false;
        }

        // find the nearest hit that lies in the acceptable range (t_min, t_max)
        float sqrtd = sqrt(delta);
        float root = (-h - sqrtd) / a;
        if (root <= t_min || root >= t_max) {
            root = (-h + sqrtd) / a;
            if (root <= t_min || root >= t_max) {
                return false;
            }
        }

        hit_rec.m_t = root;
        hit_rec.m_p = r.at(root);
        float3 outward_normal = (hit_rec.m_p - m_center) / m_radius;
        hit_rec.set_face_normal(r, outward_normal);
        hit_rec.m_mat = m_mat;
        

        return true;
    }

    float3   m_center;
    float    m_radius;
    Material m_mat;
}
