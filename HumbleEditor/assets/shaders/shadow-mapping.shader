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

float ShadowCalculation(vec4 fragPosLightSpace, vec4 tileUVRange, int index)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Check if fragment is outside the light's projection
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    {
        return 0.0;
    }

    // Apply atlas tile offset and scale
    vec2 tileOffset = tileUVRange.xy;
    vec2 tileScale  = tileUVRange.zw;
    vec2 atlasUV = tileOffset + projCoords.xy * tileScale;

    // Get texel size within the tile
    vec2 texelSize = tileScale / vec2(textureSize(u_ShadowAltasMap, 0));

    // Calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(u_Light.Positions[index].xyz - v_Position);
    
    float constantBias = u_Light.ShadowData[index].y;
    float slopeBias    = u_Light.ShadowData[index].z;
    float bias = constantBias + slopeBias * (1.0 - (max(dot(normal, lightDir), 0.0)));

    // PCF sampling
    float shadow = 0.0;
    float currentDepth = projCoords.z;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 offset = vec2(x, y) * texelSize;
            float pcfDepth = texture(u_ShadowAltasMap, atlasUV + offset).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;

    return shadow;
}

void main()
{
    vec3 textureColor = texture(u_AlbedoMap, v_TextureCoord).rgb;
    vec3 normal       = normalize(v_Normal);
    vec3 camDir       = normalize(u_Light.ViewPosition.xyz - v_Position);

    // Global ambient (only once)
    vec3 ambient = 0.1 * textureColor;

    // Accumulators for diffuse & specular
    vec3 diffuseAcc  = vec3(0.0);
    vec3 specularAcc = vec3(0.0);
    float totalShadow = 0.0;

    // Loop over all lights
    for (int i = 0; i < int(u_Light.Count); ++i)
    {
        float intensity = u_Light.Metadata[i].x;
        vec3  lightCol  = u_Light.Colors[i].xyz;

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
            vec3  L      = normalize(u_Light.Positions[i].xyz - v_Position);
            float dist   = length(u_Light.Positions[i].xyz - v_Position);
            float att    = 1.0 / (u_Light.Metadata[i].y + u_Light.Metadata[i].z * dist + u_Light.Metadata[i].w * dist * dist);

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
            vec3 L         = normalize(u_Light.Positions[i].xyz - v_Position);
            float inner    = u_Light.Metadata[i].y;
            float outer    = u_Light.Metadata[i].z;
            float theta    = dot(L, normalize(-u_Light.Directions[i].xyz));
            float spotFrac = clamp((theta - outer) / (inner - outer), 0.0, 1.0);

            float dist = length(u_Light.Positions[i].xyz - v_Position);
            float att  = 1.0 / (1.0 + 0.09  * dist + 0.032 * dist * dist);

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
    vec3 result = ambient + (1.0 - totalShadow) * textureColor * diffuseAcc + specularAcc;
    FragColor = vec4(result * v_Color.rgb, 1.0);
}
