#shader vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TextureCoord;

layout(std140, set = 0, binding = 0) uniform Camera
{
    mat4 ViewProjection;
} u_Camera;

layout(std140, set = 1, binding = 2) uniform Object
{
    mat4 Model;
    mat4 InverseModel;
    vec4 Color;
    float Glossiness;
} u_Object;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec3 v_Position;
layout(location = 2) out vec3 v_Normal;
layout(location = 3) out vec2 v_TextureCoord;
layout(location = 4) out float v_Glossiness;
layout(location = 5) out mat4 v_InverseViewProj;

void main()
{
    v_Position = (u_Object.Model * vec4(a_Position, 1.0)).xyz;
    v_Color = u_Object.Color;
    v_Glossiness = u_Object.Glossiness;
    v_TextureCoord = a_TextureCoord;
    v_Normal = normalize((u_Object.InverseModel * vec4(a_Normal, 0.0)).xyz);
    v_InverseViewProj = inverse(u_Camera.ViewProjection);
    gl_Position = u_Camera.ViewProjection * u_Object.Model * vec4(a_Position, 1.0);
}

#shader fragment
#version 450 core
layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec3 v_Position;
layout(location = 2) in vec3 v_Normal;
layout(location = 3) in vec2 v_TextureCoord;
layout(location = 4) in float v_Glossiness;
layout(location = 5) in mat4 v_InverseViewProj;

layout (set = 1, binding = 0) uniform sampler2D u_AlbedoMap;
layout (set = 1, binding = 3) uniform sampler2D u_ShadowAltasMap;

layout(std140, set = 0, binding = 1) uniform Light
{
    vec4 ViewPosition;
    vec4 Positions[16];
    vec4 Directions[16];
    vec4 Colors[16];
    vec4 Metadata[16];
    vec4 ShadowData[16];
    mat4 LightSpaceMatrices[16];
    vec4 TileUVRange[16];
    float Count;
} u_Light;

// Poisson disk samples for better PCF distribution
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
);

float CalculateAdaptiveNormalOffset(vec3 normal, vec3 lightDir, float baseOffset, float lightDistance)
{
    float NdotL = clamp(dot(normal, lightDir), 0.0, 1.0);
    
    // Reduce offset at grazing angles to prevent outlines
    float angleAttenuation = smoothstep(0.0, 0.3, NdotL);
    
    // Reduce offset with distance to prevent over-offsetting
    float distanceAttenuation = clamp(1.0 - lightDistance * 0.01, 0.1, 1.0);
    
    // Scale offset based on how perpendicular the surface is to the light
    float adaptiveOffset = baseOffset * angleAttenuation * distanceAttenuation;
    
    return adaptiveOffset;
}

float CalculateHybridBias(vec3 normal, vec3 lightDir, float texelSize, float constantBias, float slopeBias)
{
    float cosTheta = clamp(dot(normal, lightDir), 0.0005, 1.0);
    float tanTheta = sqrt(1.0 - cosTheta * cosTheta) / cosTheta;
    
    // More conservative bias calculation
    float adaptiveBias = constantBias + slopeBias * tanTheta;
    
    // Scale by texel size but cap it
    adaptiveBias *= texelSize;
    
    return clamp(adaptiveBias, 0.0, 0.005);
}

float PoissonPCF(vec2 atlasUV, vec2 texelSize, float currentDepth, float bias, float radius)
{
    float shadow = 0.0;
    
    for (int i = 0; i < 16; ++i)
    {
        vec2 offset = poissonDisk[i] * texelSize * radius;
        float pcfDepth = texture(u_ShadowAltasMap, atlasUV + offset).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
    
    return shadow / 16.0;
}

float ShadowCalculation(vec4 fragPosLightSpace, vec4 tileUVRange, int index)
{
    vec4 lightClip = fragPosLightSpace;
    vec3 ndc = lightClip.xyz / lightClip.w;

    // Remap X from [-1, -1] to [0, 1]
    float u = ndc.x * 0.5 + 0.5;
    // Flip and remap Y
    float v = 1.0 - (ndc.y * 0.5 + 0.5);
    // Z already in [0, 1]
    float depth = ndc.z;

    vec3 projCoords = vec3(u, v, depth);

    // Check if fragment is outside the light's projection
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    {
        return 0.0;
    }

    // Get surface normal and light direction
    vec3 normal = normalize(v_Normal);
    vec3 lightPos = u_Light.Positions[index].xyz;
    vec3 lightDir = normalize(lightPos - v_Position);
    float lightDistance = length(lightPos - v_Position);
    
    // Shadow parameters
    float constantBias = u_Light.ShadowData[index].y;
    float slopeBias = u_Light.ShadowData[index].z;
    float baseNormalOffset = u_Light.ShadowData[index].w;
    
    // Calculate adaptive normal offset to prevent outlines
    float adaptiveNormalOffset = CalculateAdaptiveNormalOffset(normal, lightDir, baseNormalOffset, lightDistance);
    
    // Calculate how much to blend between normal offset and depth bias
    float NdotL = clamp(dot(normal, lightDir), 0.0, 1.0);
    float offsetWeight = smoothstep(0.2, 0.6, NdotL);
    
    // Apply atlas tile offset and scale using UV range system
    vec2 tileOffset = tileUVRange.xy;
    vec2 tileScale = tileUVRange.zw;
    
    // Calculate texel size - handle negative scale for Vulkan
    vec2 atlasSize = vec2(textureSize(u_ShadowAltasMap, 0));
    vec2 texelSize = abs(tileScale) / atlasSize;
    float avgTexelSize = (texelSize.x + texelSize.y) * 0.5;
    
    vec2 atlasUV;
    float finalBias;
    
    // Hybrid approach: blend between normal offset and traditional depth bias
    if (offsetWeight > 0.3)
    {
        // Use normal offset for surfaces facing the light
        vec3 offsetWorldPos = v_Position + normal * adaptiveNormalOffset;
        
        // Reproject the offset position to light space
        vec4 offsetLightSpace = u_Light.LightSpaceMatrices[index] * vec4(offsetWorldPos, 1.0);
        vec3 offsetNDC = offsetLightSpace.xyz / offsetLightSpace.w;
        
        // Convert to [0, 1] range
        float offsetU = offsetNDC.x * 0.5 + 0.5;
        float offsetV = 1.0 - (offsetNDC.y * 0.5 + 0.5);
        
        // Apply UV range transformation
        atlasUV = tileOffset + vec2(offsetU, offsetV) * tileScale;
        
        // Use minimal depth bias with normal offset
        finalBias = CalculateHybridBias(normal, lightDir, avgTexelSize, constantBias * 0.1, slopeBias * 0.1);
    }
    else
    {
        // Use traditional depth bias for grazing angles
        // Apply UV range transformation
        atlasUV = tileOffset + projCoords.xy * tileScale;
        
        // Use larger depth bias for grazing angles
        finalBias = CalculateHybridBias(normal, lightDir, avgTexelSize, constantBias, slopeBias);
    }
    
    // Clamp UV coordinates to stay within the tile bounds
    // Handle both positive and negative scales
    vec2 tileMin, tileMax;
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
    
    // Use original depth for comparison
    float currentDepth = projCoords.z;
    
    // Adaptive PCF radius
    float pcfRadius = clamp(1.0 + lightDistance * 0.005, 0.8, 2.0);
    
    // Perform shadow sampling
    float shadow = PoissonPCF(atlasUV, texelSize, currentDepth, finalBias, pcfRadius);
    
    return shadow;
}

void main()
{
    vec3 textureColor = texture(u_AlbedoMap, v_TextureCoord).rgb;
    vec3 normal = normalize(v_Normal);
    vec3 camDir = normalize(u_Light.ViewPosition.xyz - v_Position);

    // Global ambient (only once)
    vec3 ambient = 0.1 * textureColor;

    // Accumulators for diffuse & specular
    vec3 diffuseAcc = vec3(0.0);
    vec3 specularAcc = vec3(0.0);
    float totalShadow = 0.0;

    // Loop over all lights
    for (int i = 0; i < int(u_Light.Count); ++i)
    {
        float intensity = u_Light.Metadata[i].x;
        vec3 lightCol = u_Light.Colors[i].xyz;

        if (u_Light.Positions[i].w == 0.0)
        {
            // Directional Light
            vec3 L = normalize(-u_Light.Directions[i].xyz);

            // Diffuse
            float diff = max(dot(normal, L), 0.0);
            vec3 D = diff * lightCol * intensity;

            // Specular
            vec3 H = normalize(L + camDir);
            float spec = pow(max(dot(normal, H), 0.0), 32.0) * v_Glossiness;
            vec3 S = lightCol * spec * intensity;

            // Accumulate
            diffuseAcc  += D;
            specularAcc += S;

            if (u_Light.ShadowData[i].x == 1.0)
            {
                mat4 lightSpaceMatrix = u_Light.LightSpaceMatrices[i];
                vec4 fragPosLightSpace = lightSpaceMatrix * vec4(v_Position, 1.0);

                totalShadow += ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i);
            }
        }
        else if (u_Light.Positions[i].w == 1.0)
        {
            // Point Light
            vec3 L = normalize(u_Light.Positions[i].xyz - v_Position);
            float dist = length(u_Light.Positions[i].xyz - v_Position);
            float att = 1.0 / (u_Light.Metadata[i].y + u_Light.Metadata[i].z * dist + u_Light.Metadata[i].w * dist * dist);

            // Diffuse
            float diff = max(dot(normal, L), 0.0);
            vec3 D = diff * lightCol * intensity * att;

            // Specular
            vec3 H = normalize(L + camDir);
            float spec = pow(max(dot(normal, H), 0.0), 32.0) * v_Glossiness;
            vec3 S = lightCol * spec * intensity * att;

            // Accumulate
            diffuseAcc  += D;
            specularAcc += S;

            if (u_Light.ShadowData[i].x == 1.0 && att >= 0.0)
            {
                mat4 lightSpaceMatrix = u_Light.LightSpaceMatrices[i];
                vec4 fragPosLightSpace = lightSpaceMatrix * vec4(v_Position, 1.0);

                totalShadow += ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i);
            }
        }
        else if (u_Light.Positions[i].w == 2.0)
        {
            // Spot Light
            vec3 L = normalize(u_Light.Positions[i].xyz - v_Position);
            float inner = u_Light.Metadata[i].y;
            float outer = u_Light.Metadata[i].z;
            float theta = dot(L, normalize(-u_Light.Directions[i].xyz));
            float spotFrac = clamp((theta - outer) / (inner - outer), 0.0, 1.0);

            float dist = length(u_Light.Positions[i].xyz - v_Position);
            float att = 1.0 / (1.0 + 0.09  * dist + 0.032 * dist * dist);

            // Diffuse
            float diff = max(dot(normal, L), 0.0);
            vec3 D = diff * lightCol * intensity * att * spotFrac;

            // Specular
            vec3 H = normalize(L + camDir);
            float spec = pow(max(dot(normal, H), 0.0), 32.0) * v_Glossiness;
            vec3 S = lightCol * spec * intensity * att * spotFrac;

            // Accumulate
            diffuseAcc  += D;
            specularAcc += S;

            bool lit = (spotFrac > 0.0) && (diff > 0.0) && (att > 0.0);

            if (u_Light.ShadowData[i].x == 1.0 && lit)
            {
                mat4 lightSpaceMatrix = u_Light.LightSpaceMatrices[i];
                vec4 fragPosLightSpace = lightSpaceMatrix * vec4(v_Position, 1.0);

                totalShadow += ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i);
            }
        }        
    }

    totalShadow = clamp(totalShadow / float(u_Light.Count), 0.0, 1.0);

    // Final color
    vec3 result = ambient + (1.0 - totalShadow) * (textureColor * diffuseAcc + specularAcc);
    FragColor = vec4(result * v_Color.rgb, 1.0);
}
