// Common definitions

import lighting_common;

extern struct LightProvider : ILightProvider;

struct FrameData
{
    float4x4 ViewProjection;
    float4 ViewPosition;
    float LightCount;
};

struct Object
{
    float4x4 Model;
    float4x4 InverseModel;
    float4 Color;
};

struct Material
{
    float4 Color;
    float Roughness;
    float Metalicness;
};

[[vk::binding(0, 0)]]
ConstantBuffer<FrameData> u_FrameData;

[[vk::binding(2, 0)]]
Sampler2D u_ShadowAltasMap;

[[vk::binding(0, 2)]]
Sampler2D u_AlbedoMap;

[[vk::binding(1, 2)]]
Sampler2D u_NormalMap;

[[vk::binding(2, 2)]]
Sampler2D u_MetallicMap;

[[vk::binding(3, 2)]]
Sampler2D u_RoughnessMap;

[[vk::binding(4, 2)]]
ConstantBuffer<Material> u_Material;

[[vk::binding(0, 3)]]
ConstantBuffer<Object> u_Object;

// Vertex Shader

struct VSInput
{
    [[vk::location(0)]] float3 a_Position;
    [[vk::location(1)]] float3 a_Normal;
    [[vk::location(2)]] float2 a_TextureCoord;
};

struct VSOutput
{
    float4 position : SV_Position;

    [[vk::location(0)]] float4 v_Color;
    [[vk::location(1)]] float3 v_Position;
    [[vk::location(2)]] float3 v_Normal;
    [[vk::location(3)]] float2 v_TextureCoord;
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    VSOutput output;

    output.v_Position = mul(u_Object.Model, float4(input.a_Position, 1.0)).xyz;
    output.v_Color = u_Material.Color * u_Object.Color;
    output.v_TextureCoord = input.a_TextureCoord;
    output.v_Normal = mul(u_Object.InverseModel, float4(input.a_Normal, 0.0)).xyz;
    output.position = mul(u_FrameData.ViewProjection, mul(u_Object.Model, float4(input.a_Position, 1.0)));

    return output;
}

// Fragment / Pixel Shader

struct PSInput
{
    float4 position : SV_Position;

    [[vk::location(0)]] float4 v_Color;
    [[vk::location(1)]] float3 v_Position;
    [[vk::location(2)]] float3 v_Normal;
    [[vk::location(3)]] float2 v_TextureCoord;
};

static const float2 poissonDisk[16] =
{
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790)
};

static const float PI = 3.14159265359;

static const float3 SKY_AMBIENT = float3(0.12, 0.14, 0.20);
static const float3 GROUND_AMBIENT = float3(0.05, 0.04, 0.03);
static const float AMBIENT_INTENSITY = 2.0;

float DistanceWindow(float dist, float range)
{
    float r = saturate(dist / max(range, 1e-4));
    float f = saturate(1.0 - r * r * r * r);
    return f * f;
}

float PointAttenuation(float dist, float range)
{
    return DistanceWindow(dist, range) / (1.0 + 0.09 * dist + 0.032 * dist * dist);
}

float CalculateAdaptiveNormalOffset(float3 normal, float3 lightDir, float baseOffset, float lightDistance)
{
    float NdotL = clamp(dot(normal, lightDir), 0.0, 1.0);

    float angleAttenuation = smoothstep(0.0, 0.3, NdotL);
    float distanceAttenuation = clamp(1.0 - lightDistance * 0.01, 0.1, 1.0);
    float adaptiveOffset = baseOffset * angleAttenuation * distanceAttenuation;

    return adaptiveOffset;
}

float CalculateHybridBias(float3 normal, float3 lightDir, float texelSize, float constantBias, float slopeBias)
{
    float cosTheta = clamp(dot(normal, lightDir), 0.0005, 1.0);
    float tanTheta = sqrt(1.0 - cosTheta * cosTheta) / cosTheta;
    float adaptiveBias = constantBias + slopeBias * tanTheta;
    adaptiveBias *= texelSize;

    return clamp(adaptiveBias, 0.0, 0.005);
}

float PoissonPCF(float2 atlasUV, float2 texelSize, float currentDepth, float bias, float radius)
{
    float shadow = 0.0;

    for (int i = 0; i < 16; ++i)
    {
        float2 offset = poissonDisk[i] * texelSize * radius;
        float pcfDepth = u_ShadowAltasMap.Sample(atlasUV + offset).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }

    return shadow / 16.0;
}

float ShadowCalculation(float4 fragPosLightSpace, float4 tileUVRange, int index, PSInput input)
{
    float4 lightClip = fragPosLightSpace;
    float3 ndc = lightClip.xyz / lightClip.w;

    float u = ndc.x * 0.5 + 0.5;
    float v = 1.0 - (ndc.y * 0.5 + 0.5);
    float depth = ndc.z;

    float3 projCoords = float3(u, v, depth);

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    {
        return 0.0;
    }

    float3 normal = normalize(input.v_Normal);
    float3 lightDir;
    float lightDistance;

    if (u_Light[index].Position.w == 0.0)
    {
        // For directional lights, parallel rays, so the position is irrelevant.
        lightDir = normalize(-u_Light[index].Direction.xyz);
        lightDistance = 0.0;
    }
    else
    {
        float3 toLight = u_Light[index].Position.xyz - input.v_Position;
        lightDistance = length(toLight);
        lightDir = toLight / lightDistance;
    }

    float constantBias = u_Light[index].ShadowData.y;
    float slopeBias = u_Light[index].ShadowData.z;
    float baseNormalOffset = u_Light[index].ShadowData.w;

    float adaptiveNormalOffset = CalculateAdaptiveNormalOffset(normal, lightDir, baseNormalOffset, lightDistance);

    float NdotL = clamp(dot(normal, lightDir), 0.0, 1.0);
    float offsetWeight = smoothstep(0.2, 0.6, NdotL);

    float2 tileOffset = tileUVRange.xy;
    float2 tileScale = tileUVRange.zw;

    uint atlasSizeWidth;
    uint atlasSizeHeight;

    u_ShadowAltasMap.GetDimensions(atlasSizeWidth, atlasSizeHeight);

    float2 atlasSize = float2(atlasSizeWidth, atlasSizeHeight);
    float2 texelSize = abs(tileScale) / atlasSize;
    float  avgTexelSize = (texelSize.x + texelSize.y) * 0.5;

    float2 atlasUV;
    float  finalBias;

    if (offsetWeight > 0.3)
    {
        float3 offsetWorldPos = input.v_Position + normal * adaptiveNormalOffset;
        float4 offsetLightSpace = mul(u_Light[index].LightSpaceMatrix, float4(offsetWorldPos, 1.0));
        float3 offsetNDC = offsetLightSpace.xyz / offsetLightSpace.w;

        float offsetU = offsetNDC.x * 0.5 + 0.5;
        float offsetV = 1.0 - (offsetNDC.y * 0.5 + 0.5);

        atlasUV = tileOffset + float2(offsetU, offsetV) * tileScale;
        finalBias = CalculateHybridBias(normal, lightDir, avgTexelSize, constantBias * 0.1, slopeBias * 0.1);
    }
    else
    {
        atlasUV = tileOffset + projCoords.xy * tileScale;
        finalBias = CalculateHybridBias(normal, lightDir, avgTexelSize, constantBias, slopeBias);
    }

    float2 tileMin, tileMax;
    if (tileScale.x > 0.0)
    {
        tileMin.x = tileOffset.x + texelSize.x * 0.5;
        tileMax.x = tileOffset.x + tileScale.x - texelSize.x * 0.5;
    }
    else
    {
        tileMin.x = tileOffset.x + tileScale.x + texelSize.x * 0.5;
        tileMax.x = tileOffset.x - texelSize.x * 0.5;
    }

    if (tileScale.y > 0.0)
    {
        tileMin.y = tileOffset.y + texelSize.y * 0.5;
        tileMax.y = tileOffset.y + tileScale.y - texelSize.y * 0.5;
    }
    else
    {
        tileMin.y = tileOffset.y + tileScale.y + texelSize.y * 0.5;
        tileMax.y = tileOffset.y - texelSize.y * 0.5;
    }

    atlasUV = clamp(atlasUV, tileMin, tileMax);

    float currentDepth = projCoords.z;
    float pcfRadius = clamp(1.0 + lightDistance * 0.005, 0.8, 2.0);
    float shadow = PoissonPCF(atlasUV, texelSize, currentDepth, finalBias, pcfRadius);

    return shadow;
}

float3 getNormalFromMap(PSInput input)
{
    float3 tangentNormal = u_NormalMap.Sample(input.v_TextureCoord).xyz * 2.0 - 1.0;

    float3 Q1 = ddx(input.v_Position);
    float3 Q2 = -ddy(input.v_Position);
    float2 st1 = ddx(input.v_TextureCoord);
    float2 st2 = -ddy(input.v_TextureCoord);

    float3 N = normalize(input.v_Normal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(TBN, tangentNormal));
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

[shader("fragment")]
float4 mainPS(PSInput input) : SV_Target0
{
    LightQuery q = { input.position.xyz, uint(u_FrameData.LightCount) };
    LightProvider provider;

    // Apply vertex and material colors to linear albedo before lighting calculations.
    // float3 albedo = pow(u_AlbedoMap.Sample(input.v_TextureCoord).rgb, float3(2.2, 2.2, 2.2)) * input.v_Color.rgb;
    float3 albedo = u_AlbedoMap.Sample(input.v_TextureCoord).rgb * input.v_Color.rgb;
    float metallic = saturate(u_MetallicMap.Sample(input.v_TextureCoord).r * u_Material.Metalicness);
    float roughness = clamp(u_RoughnessMap.Sample(input.v_TextureCoord).r * u_Material.Roughness, 0.045, 1.0);

    float3 N = getNormalFromMap(input);
    float3 V = normalize(u_FrameData.ViewPosition.xyz - input.v_Position);

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = float3(0.0, 0.0, 0.0);

    uint lightCount = provider.GetLightCount(q);

    for (uint i = 0; i < lightCount; ++i)
    {
        uint lightIndex = provider.GetLightIndex(q, i);

        uint lightType = uint(u_Light[lightIndex].Position.w);
        float lightIntensity = u_Light[lightIndex].Metadata.x * 10.f;
        float3 lightColor = u_Light[lightIndex].Color.xyz;
        float lightRange = u_Light[lightIndex].Color.a;

        float3 lightPos = u_Light[lightIndex].Position.xyz;
        float distance = length(lightPos - input.v_Position);

        // Skip point and spot lights that are out of range of the point being shaded.
        if (lightType != 0 && lightRange > 0.0 && distance > lightRange)
        {
            continue;
        }

        float3 L = float3(0.0, 0.0, 0.0);
        float attenuation = 1.0;
        bool lit = true;
        float shadow = 0.0;

        if (lightType == 0) // Directional Light
        {
            L = normalize(-u_Light[lightIndex].Direction.xyz);
            lit = (max(dot(N, L), 0.0) > 0.0);

            if (u_Light[lightIndex].ShadowData.x == 1.0 && lit)
            {
                float4 fragPosLightSpace = mul(u_Light[lightIndex].LightSpaceMatrix, float4(input.v_Position, 1.0));
                shadow = ShadowCalculation(fragPosLightSpace, u_Light[lightIndex].TileUVRange, lightIndex, input);
            }
        }
        else if (lightType == 1) // Point Light
        {
            float3 toLight = lightPos - input.v_Position;
            L = toLight / max(distance, 1e-6);

            float constant = u_Light[lightIndex].Metadata.y;
            float linear = u_Light[lightIndex].Metadata.z;
            float quadratic = u_Light[lightIndex].Metadata.w;

            attenuation = PointAttenuation(distance, lightRange);

            lit = (attenuation > 0.0) && (max(dot(N, L), 0.0) > 0.0);

            if (u_Light[lightIndex].ShadowData.x == 1.0 && lit)
            {
                float4 fragPosLightSpace = mul(u_Light[lightIndex].LightSpaceMatrix, float4(input.v_Position, 1.0));
                shadow = ShadowCalculation(fragPosLightSpace, u_Light[lightIndex].TileUVRange, lightIndex, input);
            }
        }
        else if (lightType == 2) // Spot Light
        {
            float3 toLight = lightPos - input.v_Position;
            L = toLight / max(distance, 1e-6);

            float innerCutoff = u_Light[lightIndex].Metadata.y;
            float outerCutoff = u_Light[lightIndex].Metadata.z;
            float theta = dot(L, normalize(-u_Light[lightIndex].Direction.xyz));
            float epsilon = max(innerCutoff - outerCutoff, 1e-6);

            float spotFactor = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

            attenuation = PointAttenuation(distance, lightRange) * spotFactor;

            lit = (spotFactor > 0.0) && (attenuation > 0.0) && (max(dot(N, L), 0.0) > 0.0);

            if (u_Light[lightIndex].ShadowData.x == 1.0 && lit)
            {
                float4 fragPosLightSpace = mul(u_Light[lightIndex].LightSpaceMatrix, float4(input.v_Position, 1.0));
                shadow = ShadowCalculation(fragPosLightSpace, u_Light[lightIndex].TileUVRange, lightIndex, input);
            }
        }

        if (!lit)
        {
            continue;
        }

        float visibility = 1.0 - clamp(shadow, 0.0, 1.0);
        float3 radiance = lightColor * attenuation * lightIntensity * visibility;

        float3 H = normalize(V + L);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        float3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denom;

        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    float up = N.y * 0.5 + 0.5; // world is Y-up.
    float3 ambientLight = lerp(GROUND_AMBIENT, SKY_AMBIENT, up) * AMBIENT_INTENSITY;
    float3 ambient = ambientLight * albedo * (1.0 - metallic);

    float3 color = ambient + Lo;

    return float4(color, 1.0);
}