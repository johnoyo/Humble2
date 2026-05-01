// Common definitions

struct Camera
{
    float4x4 ViewProjection;
};

struct Object
{
    float4x4 Model;
    float4x4 InverseModel;
    float4 Color;
    float Glossiness;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Camera> u_Camera;

[[vk::binding(2, 1)]]
ConstantBuffer<Object> u_Object;

// Vertex Shader

struct VSInput
{
    [[vk::location(0)]] float3 a_Position;
    [[vk::location(1)]] float3 a_Normal;
};

[shader("vertex")]
float4 mainVS(VSInput input) : SV_Position
{
    float4 worldPos = mul(u_Object.Model, float4(input.a_Position, 1.0));
    return mul(u_Camera.ViewProjection, worldPos);
}

// Fragment / Pixel Shader

[shader("fragment")]
void mainPS()
{
}