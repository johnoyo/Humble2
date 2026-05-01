// Common definitions

struct Camera
{
    float4x4 ViewProjection;
};

struct Object
{
    float4x4 Model;
    float4 Color;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Camera> u_Camera;

[[vk::binding(2, 1)]]
ConstantBuffer<Object> u_Object;

[[vk::binding(0, 1)]]
Sampler2D u_AlbedoMap;

[[vk::binding(3, 1)]]
Sampler2D u_ShadowAltasMap;

// Vertex Shader

struct VSInput
{
    [[vk::location(0)]] float3 a_Position;
    [[vk::location(1)]] float2 a_TextureCoord;
};

struct VSOutput
{
    float4 position : SV_Position;
    [[vk::location(0)]] float2 v_TextureCoord;
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;

    output.v_TextureCoord = input.a_TextureCoord;

    // GLSL-style multiplication order preserved
    float4 worldPos = mul(u_Object.Model, float4(input.a_Position, 1.0));
    output.position = mul(u_Camera.ViewProjection, worldPos);

    return output;
}

// Fragment / Pixel Shader

struct PSInput
{
    [[vk::location(0)]] float2 v_TextureCoord;
};

[shader("fragment")]
float4 mainPS(PSInput input) : SV_Target0
{
    float4 color = u_AlbedoMap.Sample(input.v_TextureCoord);
    return color * float4(1.0, 0.0, 1.0, 1.0);
}