// Common definitions

struct Camera
{
    float4x4 ViewProjection;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Camera> u_Camera;

// Vertex Shader

float4 UnpackColor(uint packedColor)
{
    float r = float(packedColor & 0xFFu) / 255.0;
    float g = float((packedColor >> 8u) & 0xFFu) / 255.0;
    float b = float((packedColor >> 16u) & 0xFFu) / 255.0;
    float a = float((packedColor >> 24u) & 0xFFu) / 255.0;

    return float4(r, g, b, a);
}

struct VSInput
{
    [[vk::location(0)]] float3 a_Position;
    [[vk::location(1)]] uint a_Color;
};

struct VSOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]] float4 v_Color;
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;

    output.v_Color  = UnpackColor(input.a_Color);
    output.position = mul(u_Camera.ViewProjection, float4(input.a_Position, 1.0));

    return output;
}

// Fragment / Pixel Shader

struct PSInput
{
    [[vk::location(0)]] float4 v_Color;
};

[shader("fragment")]
float4 mainPS(PSInput input) : SV_Target0
{
    return input.v_Color;
}