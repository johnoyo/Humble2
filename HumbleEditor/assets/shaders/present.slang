// Common definitions

[[vk::binding(0, 0)]]
Sampler2D u_FullScreenTexture;

// Vertex Shader

struct VSInput
{
    [[vk::location(0)]] float2 a_Position;
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
    output.position = float4(input.a_Position.x, input.a_Position.y, 0.0, 1.0);

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
    float3 color = u_FullScreenTexture.Sample(input.v_TextureCoord).xyz;
    return float4(color, 1.0);
}