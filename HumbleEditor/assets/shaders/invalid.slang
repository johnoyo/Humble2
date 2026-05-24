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

[[vk::binding(4, 3)]]
ConstantBuffer<Object> u_Object;

// Vertex Shader

struct VSInput
{
    [[vk::location(0)]] float3 a_Position;
};

struct VSOutput
{
    float4 position : SV_Position;
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;

    // GLSL-style multiplication order preserved
    float4 worldPos = mul(u_Object.Model, float4(input.a_Position, 1.0));
    output.position = mul(u_Camera.ViewProjection, worldPos);

    return output;
}

// Fragment / Pixel Shader

[shader("fragment")]
float4 mainPS() : SV_Target0
{
    return float4(1.0, 0.0, 1.0, 1.0);
}