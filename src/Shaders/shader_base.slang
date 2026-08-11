struct VertexInput {
    float2 inPosition;
    float3 inColor;
    float2 inTexCoord;
};

struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};
[vk_binding(0, 0)]
ConstantBuffer<UniformBufferObject> ubo;

struct VertexOutput {
    float3 color;
    float4 sv_position : SV_Position;
    float2 texCoord;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    output.sv_position = float4(input.inPosition, 0.0, 1.0);
    // output.sv_position = mul(ubo.proj,
    //                      mul(ubo.view,
    //                      mul(ubo.model, float4(input.inPosition, 0.0, 1.0))));
    output.color = input.inColor;
    output.texCoord = input.inTexCoord;
    return output;
}

[vk_binding(0, 1)]
Sampler2D Texture;

[vk_binding(1, 1)]
Sampler2D Gradient;

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    // float3 color = inVert.color;
    // return float4(color, 1.0);
    return Gradient.Sample(inVert.texCoord);
}
