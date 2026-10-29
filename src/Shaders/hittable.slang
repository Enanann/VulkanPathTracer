module hittable;

import ray;

public enum MaterialType : uint {
    Lambertian,
    Metal,
    Dielectric,
}

public struct Material {
    public float3 albedo;
    public float  roughness;
    public float  refractionIndex;
    public MaterialType type;
}

public struct HitRecord {
    [mutating]
    public void set_face_normal(const Ray _r, const float3 _outward_normal) {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        m_front_face = (dot(_r.dir(), _outward_normal) < 0);
        m_normal = m_front_face ? _outward_normal : -_outward_normal;
    }
    
    public float3   m_p;
    public float3   m_normal;
    public float    m_t;
    public bool     m_front_face;
    public Material m_mat;
}

public interface IHittable {
    bool hit(const Ray _r, float t_min, float t_max, inout HitRecord hit_rec);
}
