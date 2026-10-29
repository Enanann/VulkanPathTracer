module random;

// TODO: Consider Owen-scrambled Sobol sequences for better quality
public struct Random {
    public __init(uint state) {
        m_state = state;
    }

    [mutating]
    public float random_float() {
        uint oldState = m_state;
        m_state = oldState * 747796405u + 2891336453u;

        uint word = ((oldState >> ((oldState >> 28u) + 4u)) ^ oldState) * 277803737u;
        uint result = (word >> 22u) ^ word;
        return float(result) / 4294967295.0; // 2^32 - 1
    }

    [mutating]
    public float random_float(float min, float max) {
        return random_float() * (max - min) + min;
    }

    [mutating]
    public float3 sample_square() {
        // return vector to a random point in the [-0.5, -0.5]-[+0.5, +0.5] unit square
        float x = random_float();
        float y = random_float();

        return float3(x - 0.5, y - 0.5, 0);
    }

    [mutating]
    public float3 random_vec() {
        float x = random_float();
        float y = random_float();
        float z = random_float();

        return float3(x, y, z);
    }

    [mutating]
    public float3 random_vec(float min, float max) {
        float x = random_float(min, max);
        float y = random_float(min, max);
        float z = random_float(min, max);

        return float3(x, y, z);
    }

    [mutating]
    public float3 random_unit_vec() {
        while (true) {
            float3 v = random_vec(-1.0, 1.0);
            float lsq = dot(v, v);
            if (1e-8 <= lsq && lsq <= 1) {
                return normalize(v);
            }
        }
    }

    [mutating]
    public float3 random_on_hemisphere(float3 normal) {
        float3 v = random_unit_vec();
        if (dot(v, normal) > 0.0) {
            return v;
        } else {
            return -v;
        }
    }

    [mutating]
    public float3 random_in_unit_disk() {
        while (true) {
            float3 p = float3(random_float(-1, 1), random_float(-1, 1), 0);
            if (dot(p, p) < 1) {
                return p;
            }
        }
    }

    [mutating]
    public float3 defocus_disk_sample(float3 center, float3 defocus_disk_u, float3 defocus_disk_v) {
        float3 p = random_in_unit_disk();
        return center + p.x * defocus_disk_u + p.y * defocus_disk_v;
    }

    uint m_state;
}
