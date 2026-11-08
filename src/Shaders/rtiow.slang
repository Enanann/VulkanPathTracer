import ray;
import sphere;
import hittable;
import camera;
import random;
import material;

[vk_binding(0, 0)]
RWTexture2D<float4> Image;

struct SceneObject {
    float4 objectProperties;
    Material mat;
}
[vk_binding(1, 0)]
StructuredBuffer<SceneObject> sceneObjects;

struct PushConstant {
    uint samples;
}
[vk_push_constant]
PushConstant pc;

static const int RR_MIN_DEPTH = 5;      // Minimum bounces before applying Russian Roulette
static const float RR_FLOOR   = 0.001f; // Minimun survival propability
static const float RR_CAP     = 0.95f;  // Maximin survival propability

bool hitt(const Ray r, float t_min, float t_max, inout HitRecord hit_rec) {
    HitRecord temp_rec;
    bool hit_anything = false;
    float closest_so_far = t_max;

    uint sceneObjectsLength;
    uint sceneObjectsStride;
    sceneObjects.GetDimensions(sceneObjectsLength, sceneObjectsStride);

    [unroll]
    for (int i = 0; i < sceneObjectsLength; i++) {
        if (Sphere(sceneObjects[i].objectProperties.xyz, sceneObjects[i].objectProperties.w, sceneObjects[i].mat).hit(r, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.m_t;
            hit_rec = temp_rec;
        }
    }

    return hit_anything;
}

float3 ray_color(Ray r, inout Random rng) {
    HitRecord rec;
    float3 throughput = float3(1.0, 1.0, 1.0);
    const int MAX_DEPTH = 50;
    for (int bounce = 0; bounce < MAX_DEPTH; bounce++) {
        if (!hitt(r, 0.001, 1000000.0, rec)) {
            float3 unit_dir = normalize(r.dir());
            // map y from [-1.0, 1.0] to [0.0, 1.0]
            float y = (unit_dir.y + 1.0) * 0.5;

            float3 sky_color = lerp(float3(1.0, 1.0, 1.0), float3(0.3, 0.5, 0.7), y);
            return throughput * sky_color;
        }

        Ray scattered;
        float3 attenuation;
        if (!scatter(r, rec, attenuation, scattered, rng)) {   
            return float3(0.0);
        }

        r = scattered;
        throughput *= attenuation;

        // Russian roulette
        // Survival probability (p) scales with throughput; surviving paths are boosted by 1/P
        // so the estimator remains unbiased. Only applied after RR_MIN_DEPTH bounces.
        if (bounce >= RR_MIN_DEPTH) {
            float p = clamp(max3(throughput.r, throughput.g, throughput.b), RR_FLOOR, RR_CAP);
            if (rng.random_float() >= p) {
                break; // terminate ray
            }
            throughput /= p; // boost energy of surviving rays
        }
    }
    return float3(0.0);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
    uint2 size;
    Image.GetDimensions(size.x, size.y);
    if (threadId.x >= size.x || threadId.y >= size.y) return;

    int2 texelCoord = threadId.xy;

    // viewport and camera setup
    float aspect_ratio = float(size.x) / float(size.y);
    float3 camera_center = float3(13, 2, 3);
    float3 lookAt = float3(0, 0, 0);
    float3 up = float3(0.0, 1.0, 0.0);
    float defocus_angle = 0.6;
    float focus_dist = 10.0;
    float fov = 20;

    Camera camera = {camera_center, up, lookAt, defocus_angle, focus_dist, aspect_ratio, size.y, fov};

    int samples = pc.samples;

    uint seed = uint(texelCoord.x) * 2005u + uint(texelCoord.y) * 2008u + uint(pc.samples) * 1234u;
    Random rng = {seed};

    float3 previousAverage = Image[texelCoord].rgb;
    float3 sampleColor = ray_color(camera.getRay(texelCoord, rng), rng);

    float3 newAverage = previousAverage + (sampleColor - previousAverage) / float(samples);

    Image[texelCoord] = float4(newAverage, 1.0);
}
