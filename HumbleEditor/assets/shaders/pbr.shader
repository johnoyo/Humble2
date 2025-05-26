#shader vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TextureCoord;

layout(std140, set = 0, binding = 0) uniform Camera
{
    mat4 ViewProjection;
} u_Camera;

layout(std140, set = 1, binding = 5) uniform Object
{
    mat4 Model;
    mat4 InverseModel;
    vec4 Color;
    float Metalicness;
    float Roughness;
} u_Object;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec3 v_Position;
layout(location = 2) out vec3 v_Normal;
layout(location = 3) out vec2 v_TextureCoord;

void main()
{
    v_Position = (u_Object.Model * vec4(a_Position, 1.0)).xyz;
    v_Color = u_Object.Color;
    v_TextureCoord = a_TextureCoord;
    v_Normal = (u_Object.InverseModel * vec4(a_Normal, 0.0)).xyz;
    gl_Position = u_Camera.ViewProjection * u_Object.Model * vec4(a_Position, 1.0);
}

#shader fragment
#version 450 core
layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec3 v_Position;
layout(location = 2) in vec3 v_Normal;
layout(location = 3) in vec2 v_TextureCoord;

layout (set = 1, binding = 0) uniform sampler2D u_AlbedoMap;
layout (set = 1, binding = 1) uniform sampler2D u_NormalMap;
layout (set = 1, binding = 2) uniform sampler2D u_MetallicMap;
layout (set = 1, binding = 3) uniform sampler2D u_RoughnessMap;
layout (set = 1, binding = 4) uniform sampler2D u_ShadowAltasMap;

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

const float PI = 3.14159265359;

// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anyways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(u_NormalMap, v_TextureCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(v_Position);
    vec3 Q2  = dFdy(v_Position);
    vec2 st1 = dFdx(v_TextureCoord);
    vec2 st2 = dFdy(v_TextureCoord);

    vec3 N   = normalize(v_Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 albedo     = pow(texture(u_AlbedoMap, v_TextureCoord).rgb, vec3(2.2));
    float metallic  = texture(u_MetallicMap, v_TextureCoord).r;
    float roughness = texture(u_RoughnessMap, v_TextureCoord).r;

    vec3 N = getNormalFromMap();
    vec3 V = normalize(u_Light.ViewPosition.xyz - v_Position);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    float totalShadow = 0.0;

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < int(u_Light.Count); ++i) 
    {
        float intensity = u_Light.Metadata[i].x;

        vec3 L;
        float attenuation = 1.0;

        if (u_Light.Positions[i].w == 0.0)
        {
            // Directional Light
            L = normalize(-u_Light.Directions[i].xyz);

            // If it is a shadow casting light.
            if (u_Light.ShadowData[i].x == 1.0)
            {
                mat4 lightSpaceMatrix = u_Light.LightSpaceMatrices[i];
                vec4 fragPosLightSpace = lightSpaceMatrix * vec4(v_Position, 1.0);

                totalShadow += ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i);
            }
        }
        else if (u_Light.Positions[i].w == 1.0)
        {
            // Point light
            vec3 lightPos = u_Light.Positions[i].xyz - v_Position;
            L = normalize(lightPos);
            float distance = length(lightPos);

            float constant = u_Light.Metadata[i].y;
            float linear = u_Light.Metadata[i].z;
            float quadratic = u_Light.Metadata[i].w;
            attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

            if (u_Light.ShadowData[i].x == 1.0 && attenuation > 0.0)
            {
                mat4 lightSpaceMatrix = u_Light.LightSpaceMatrices[i];
                vec4 fragPosLightSpace = lightSpaceMatrix * vec4(v_Position, 1.0);

                totalShadow += ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i);
            }
        }
        else if (u_Light.Positions[i].w == 2.0)
        {
            // Spot Light
            vec3 lightPos = u_Light.Positions[i].xyz - v_Position;
            L = normalize(lightPos);
            float distance = length(lightPos);

            float constant = 1.0;
            float linear = 0.09;
            float quadratic = 0.032;
            attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

            float innerCutoff = u_Light.Metadata[i].y;
            float outerCutoff = u_Light.Metadata[i].z;
            float theta = dot(L, normalize(-u_Light.Directions[i].xyz));
            float epsilon = innerCutoff - outerCutoff;
            float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);
            attenuation *= intensity;

            bool lit = intensity > 0.0;

            if (u_Light.ShadowData[i].x == 1.0 && lit)
            {
                mat4 lightSpaceMatrix = u_Light.LightSpaceMatrices[i];
                vec4 fragPosLightSpace = lightSpaceMatrix * vec4(v_Position, 1.0);

                totalShadow += ShadowCalculation(fragPosLightSpace, u_Light.TileUVRange[i], i);
            }
        }

        // Calculate per-light radiance
        vec3 H = normalize(V + L);
        vec3 radiance = u_Light.Colors[i].xyz * attenuation * intensity;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    totalShadow = clamp(totalShadow / float(u_Light.Count), 0.0, 1.0);
    
    // ambient lighting (note that the next IBL tutorial will replace this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.015) * albedo;
    
    vec3 color = ambient + (1.0 - totalShadow) * Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));

    // gamma correct
    color = pow(color, vec3(1.0 / 2.2)); 

    FragColor = vec4(color * v_Color.rgb, 1.0);
}
