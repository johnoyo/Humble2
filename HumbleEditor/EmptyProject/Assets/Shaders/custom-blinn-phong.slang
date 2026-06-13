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
};

struct Light
{
    float4 ViewPosition;
    float4 Positions[16];
    float4 Directions[16];
    float4 Colors[16];
    float4 Metadata[16];
    float4 ShadowData[16];
    float4x4 LightSpaceMatrices[16];
    float4 TileUVRange[16];
    float Count;
};

struct Material
{
    float4 Color;
    float Glossiness;
    float _Unused;
};

struct Fog
{
    float Intensity;
    float Falloff;
    float2 Bias;
    float4 Color;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Camera> u_Camera;

[[vk::binding(1, 0)]]
ConstantBuffer<Light> u_Light;

[[vk::binding(2, 0)]]
Sampler2D u_ShadowAltasMap;

[[vk::binding(5, 1)]]
ConstantBuffer<Fog> u_Fog;

[[vk::binding(0, 2)]]
Sampler2D u_AlbedoMap;

[[vk::binding(2, 2)]]
ConstantBuffer<Material> u_Material;

[[vk::binding(4, 3)]]
ConstantBuffer<Object> u_Object;

[vk::constant_id(0)]
const bool s_MyConstV = false;

[vk::constant_id(1)]
const bool s_MyConst = false;

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
    [[vk::location(4)]] float v_Glossiness;
};

[shader("vertex")]
VSOutput mainVS(VSInput input)
{
    if (s_MyConstV)
    {
        VSOutput output;

        output.v_Position = mul(u_Object.Model, float4(input.a_Position, 1.0)).xyz;
        output.v_Color = float4(1.0, 0.0, 0.0, 1.0);
        output.v_Glossiness = u_Material.Glossiness;
        output.v_TextureCoord = input.a_TextureCoord;
        output.v_Normal = normalize(mul(u_Object.InverseModel, float4(input.a_Normal, 0.0)).xyz);
        output.position = mul(u_Camera.ViewProjection, mul(u_Object.Model, float4(input.a_Position, 1.0)));

        return output;
    }

    VSOutput output;

    output.v_Position = mul(u_Object.Model, float4(input.a_Position, 1.0)).xyz;
    output.v_Color = u_Material.Color * u_Object.Color;
    output.v_Glossiness = u_Material.Glossiness;
    output.v_TextureCoord = input.a_TextureCoord;
    output.v_Normal = normalize(mul(u_Object.InverseModel, float4(input.a_Normal, 0.0)).xyz);
    output.position = mul(u_Camera.ViewProjection, mul(u_Object.Model, float4(input.a_Position, 1.0)));

    return output;
}

// Fragment / Pixel Shader

struct PSInput
{
    [[vk::location(0)]] float4 v_Color;
    [[vk::location(1)]] float3 v_Position;
    [[vk::location(2)]] float3 v_Normal;
    [[vk::location(3)]] float2 v_TextureCoord;
    [[vk::location(4)]] float v_Glossiness;
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
        float2 offset  = poissonDisk[i] * texelSize * radius;
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
    float3 lightPos = u_Light.Positions[index].xyz;
    float3 lightDir = normalize(lightPos - input.v_Position);
    float lightDistance = length(lightPos - input.v_Position);

    float constantBias = u_Light.ShadowData[index].y;
    float slopeBias = u_Light.ShadowData[index].z;
    float baseNormalOffset = u_Light.ShadowData[index].w;

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
    float avgTexelSize = (texelSize.x + texelSize.y) * 0.5;

    float2 atlasUV;
    float finalBias;

    if (offsetWeight > 0.3)
    {
        float3 offsetWorldPos  = input.v_Position + normal * adaptiveNormalOffset;
        float4 offsetLightSpace = mul(u_Light.LightSpaceMatrices[index], float4(offsetWorldPos, 1.0));
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

[shader("fragment")]
float4 mainPS(PSInput input) : SV_Target0
{
    if (s_MyConst)
    {
        return float4(input.v_Color.rgb, 1.0);
    }

    float3 textureColor = u_AlbedoMap.Sample(input.v_TextureCoord).rgb;
    float3 normal = normalize(input.v_Normal);
    float3 camDir = normalize(u_Light.ViewPosition.xyz - input.v_Position);

    float3 ambient = 0.1 * textureColor;
    float3 diffuseAcc = float3(0.0, 0.0, 0.0);
    float3 specularAcc = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < int(u_Light.Count); ++i)
    {
        float intensity = u_Light.Metadata[i].x;
        float3 lightCol = u_Light.Colors[i].xyz;

        float shadow = 0.0;

        if (u_Light.Positions[i].w == 0.0)
        {
            float3 L = normalize(-u_Light.Directions[i].xyz);

            float diff = max(dot(normal, L), 0.0);
            float3 D = diff * lightCol * intensity;

            float3 H = normalize(L + camDir);
            float spec = pow(max(dot(normal, H), 0.0), 32.0) * input.v_Glossiness;
            float3 S = lightCol * spec * intensity;

            if (u_Light.ShadowData[i].x == 1.0 && diff > 0.0)
            {
                float4 fragPosLightSpace = mul(u_Light.LightSpaceMatrices[i], float4(input.v_Position, 1.0));
                shadow = ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i, input);
            }

            diffuseAcc += (1.0 - shadow) * D;
            specularAcc += (1.0 - shadow) * S;
        }
        else if (u_Light.Positions[i].w == 1.0)
        {
            float3 toLight = u_Light.Positions[i].xyz - input.v_Position;
            float dist = length(toLight);
            float3 L = toLight / dist;

            float att = 1.0 / (u_Light.Metadata[i].y + u_Light.Metadata[i].z * dist + u_Light.Metadata[i].w * dist * dist);

            float diff = max(dot(normal, L), 0.0);
            float3 D = diff * lightCol * intensity * att;

            float3 H = normalize(L + camDir);
            float spec = pow(max(dot(normal, H), 0.0), 32.0) * input.v_Glossiness;
            float3 S = lightCol * spec * intensity * att;

            if (u_Light.ShadowData[i].x == 1.0 && diff > 0.0)
            {
                float4 fragPosLightSpace = mul(u_Light.LightSpaceMatrices[i], float4(input.v_Position, 1.0));
                shadow = ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i, input);
            }

            diffuseAcc += (1.0 - shadow) * D;
            specularAcc += (1.0 - shadow) * S;
        }
        else if (u_Light.Positions[i].w == 2.0)
        {
            float3 toLight = u_Light.Positions[i].xyz - input.v_Position;
            float dist = length(toLight);
            float3 L = toLight / dist;

            float inner = u_Light.Metadata[i].y;
            float outer = u_Light.Metadata[i].z;
            float theta = dot(L, normalize(-u_Light.Directions[i].xyz));
            float spotFrac = clamp((theta - outer) / (inner - outer), 0.0, 1.0);

            float att = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

            float diff = max(dot(normal, L), 0.0);
            float3 D = diff * lightCol * intensity * att * spotFrac;

            float3 H = normalize(L + camDir);
            float spec = pow(max(dot(normal, H), 0.0), 32.0) * input.v_Glossiness;
            float3 S = lightCol * spec * intensity * att * spotFrac;

            bool lit = (spotFrac > 0.0) && (diff > 0.0) && (att > 0.0);

            if (u_Light.ShadowData[i].x == 1.0 && lit)
            {
                float4 fragPosLightSpace = mul(u_Light.LightSpaceMatrices[i], float4(input.v_Position, 1.0));
                shadow = ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i, input);
            }

            diffuseAcc += (1.0 - shadow) * D;
            specularAcc += (1.0 - shadow) * S;
        }
    }

    float3 result = ambient + (textureColor * diffuseAcc + specularAcc);
    return float4(result * input.v_Color.rgb, 1.0);
}