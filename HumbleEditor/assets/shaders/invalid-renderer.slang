// Vertex Shader

struct VSInput
{
    [[vk::location(0)]] float2 a_Position;
};

struct VSOutput
{
    float4 position : SV_Position;
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;

    output.position = float4(input.a_Position.x, input.a_Position.y, 0.0, 1.0);

    return output;
}

// Fragment / Pixel Shader

struct PSInput
{
};

[shader("fragment")]
float4 mainPS(PSInput input) : SV_Target0
{
    return float4(1.0, 0.0, 1.0, 1.0);
}