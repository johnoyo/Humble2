// Common definitions

struct CaptureMatrices
{
    float4x4 view[6];
    float faceSize;
};

[[vk::binding(0, 0)]]
Sampler2D u_EquirectMap;

[[vk::binding(1, 0)]]
[[vk::image_format("rgba16f")]]
RWTexture2DArray<float4> u_CubeMap; // https://github.com/shader-slang/slang/issues/6742

[[vk::binding(2, 0)]]
ConstantBuffer<CaptureMatrices> u_CaptureMatrices;

// Compute Shader

static const float PI = 3.14159265359;

float2 dirToUV(float3 dir)
{
    float phi = atan2(dir.z, dir.x);
    float theta = asin(dir.y);
    return float2((phi + PI) / (2.0 * PI), (theta + PI / 2.0) / PI);
}

[shader("compute")]
[numthreads(16, 16, 1)]
void mainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    int2 pix = int2(dispatchThreadID.xy);
    int face = int(dispatchThreadID.z);
    int N = int(u_CaptureMatrices.faceSize);

    if (pix.x >= N || pix.y >= N)
    {
        return;
    }

    // Compute a [-1,1] screen coordinate.
    float2 uv = (float2(pix) / float(N - 1)) * 2.0 - 1.0;

    // Build a 90° FOV ray in view space, then rotate by the view matrix.
    float4 viewDir = float4(uv.x, uv.y, -1.0, 0.0);
    float3 dir = mul(u_CaptureMatrices.view[face], viewDir).xyz;
    dir = normalize(dir);

    // Sample the equirectangular map.
    float lod = 0.0;
    float3 color = u_EquirectMap.SampleLevel(dirToUV(dir), lod).rgb;

    // Write into the cubemap layer.
    u_CubeMap[uint3(pix, face)] = float4(color, 1.0);
}