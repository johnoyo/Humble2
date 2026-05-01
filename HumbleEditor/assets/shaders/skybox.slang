// Common definitions

struct Camera
{
    float4x4 ViewProjection;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Camera> u_Camera;

[[vk::binding(1, 1)]]
SamplerCube u_EnvironmentMap;

// Vertex Shader

struct VSInput
{
    [[vk::location(0)]] float3 a_Position;
};

struct VSOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]] float3 v_WorldPosition;
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;

    output.v_WorldPosition = input.a_Position;

    float4 clipPos = mul(u_Camera.ViewProjection, float4(input.a_Position, 1.0));
    output.position = clipPos.xyww; // Skybox depth trick: force depth to 1.0

    return output;
}

// Fragment / Pixel Shader

struct PSInput
{
    [[vk::location(0)]] float3 v_WorldPosition;
};

[shader("fragment")]
float4 mainPS(PSInput input) : SV_Target0
{
    return u_EnvironmentMap.Sample(input.v_WorldPosition);
}