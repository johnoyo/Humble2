// Common definitions

struct CameraSettings
{
    float Exposure;
    float Gamma;
};

[[vk::binding(0, 0)]]
Sampler2D u_FullScreenTexture;

[[vk::binding(1, 0)]]
ConstantBuffer<CameraSettings> u_CameraSettings;

// ACES tone mapping (NOTE: row major matrices here)

static const float3x3 ACESInputMat = float3x3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

static const float3x3 ACESOutputMat = float3x3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

float3 ACESFitted(float3 color)
{
    color = mul(ACESInputMat, color);
    color = RRTAndODTFit(color);
    color = mul(ACESOutputMat, color);
    color = clamp(color, 0.0, 1.0);
    return color;
}

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
    float3 hdrColor = u_FullScreenTexture.Sample(input.v_TextureCoord).rgb;

    // Apply exposure
    float3 exposedColor = hdrColor * u_CameraSettings.Exposure;

    // Apply ACES tone mapping
    float3 acesColor = ACESFitted(exposedColor);

    // Gamma correction
    float3 mapped = pow(acesColor, float3(1.0 / u_CameraSettings.Gamma));

    return float4(mapped, 1.0);
}