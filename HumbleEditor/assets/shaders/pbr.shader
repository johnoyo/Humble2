#shader vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TextureCoord;

layout(std140, set = 0, binding = 0) uniform Camera
{
    mat4 ViewProjection;
} u_Camera;

layout(std140, set = 1, binding = 4) uniform Object
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

layout(std140, set = 0, binding = 1) uniform Light
{
    vec4 ViewPosition;
    vec4 Positions[16];
    vec4 Colors[16];
    float Intensities[16];
    float Count;
} u_Light;

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

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < int(u_Light.Count); ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(u_Light.Positions[i].xyz - v_Position);
        vec3 H = normalize(V + L);
        float distance = length(u_Light.Positions[i].xyz - v_Position);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = u_Light.Colors[i].xyz * attenuation * u_Light.Intensities[i];

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
    
    // ambient lighting (note that the next IBL tutorial will replace this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.05) * albedo;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0 / 2.2)); 

    FragColor = vec4(color * v_Color.rgb, 1.0);
}
