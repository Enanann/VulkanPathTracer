module ray;

public struct Ray {
    public __init(float3 _orig, float3 _dir) {
        m_orig = _orig;
        m_dir  = _dir;
    }

    public float3 at(float _t) {
        return m_orig + _t * m_dir;
    }

    public float3 test() {
        return float3(1.0, 1.0, 0.0);
    }

    public float3 orig() {return m_orig;}
    public float3 dir() {return m_dir;}

    float3 m_orig;
    float3 m_dir;
};
