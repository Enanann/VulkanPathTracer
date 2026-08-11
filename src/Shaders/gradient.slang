[vk_binding(0, 0)]
RWTexture2D<float4> Image;

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
    uint2 size;
    Image.GetDimensions(size.x, size.y);

    if (threadId.x >= size.x || threadId.y >= size.y) return;

    int2 texelCoord = threadId.xy;
    float t = float(texelCoord.x) / float(size.x - 1);

    Image[texelCoord] = float4(lerp(float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), t), 1.0);
}
