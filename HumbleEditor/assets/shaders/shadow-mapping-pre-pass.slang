// Common definitions

struct LightData
{
    float4x4 LightSpaceMatrix;
};

struct Object
{
    float4x4 Model;
    float4x4 InverseModel;
    float4   Color;
    float    Glossiness;
};

[[vk::binding(0, 0)]]
ConstantBuffer<LightData> u_LightData;

[[vk::binding(2, 1)]]
ConstantBuffer<Object> u_Object;

// Vertex Shader

struct VSInput
{
    [[vk::location(0)]] float3 a_Position;
    [[vk::location(1)]] float3 a_Normal;
    [[vk::location(2)]] float2 a_TextureCoord;
};

[shader("vertex")]
float4 mainVS(VSInput input) : SV_Position
{
    return mul(u_LightData.LightSpaceMatrix, mul(u_Object.Model, float4(input.a_Position, 1.0)));
}

// Fragment / Pixel Shader

[shader("fragment")]
void mainPS()
{
}