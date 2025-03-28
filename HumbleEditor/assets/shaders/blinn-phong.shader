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

void main()
{
    v_Position = (u_Object.Model * vec4(a_Position, 1.0)).xyz;
    v_Color = u_Object.Color;
    v_Glossiness = u_Object.Glossiness;
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
layout(location = 4) in float v_Glossiness;

layout (set = 1, binding = 0) uniform sampler2D u_AlbedoMap;

layout(std140, set = 0, binding = 1) uniform Light
{
    vec4 ViewPosition;
    vec4 Positions[16];
    vec4 Colors[16];
    float Intensities[16];
    float Count;
} u_Light;

void main()
{
    vec3 textureColor = texture(u_AlbedoMap, v_TextureCoord).rgb;

    float ambientCoefficient = 0.1;
    vec3 normal = normalize(v_Normal);
    vec3 camDir = normalize(u_Light.ViewPosition.xyz - v_Position);
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);
    vec3 ambient = vec3(0.0);
    
    for (int i = 0; i < int(u_Light.Count); i++)
    {
        // diffuse
        vec3 lightDir = normalize(u_Light.Positions[i].xyz - v_Position);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 D = diff * u_Light.Colors[i].xyz * u_Light.Intensities[i];

        // specular
        vec3 halfwayDir = normalize(lightDir + camDir);  
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0) * v_Glossiness;
        vec3 S = u_Light.Colors[i].xyz * spec * u_Light.Intensities[i];

        diffuse += D;
        specular += S;
        ambient += u_Light.Colors[i].xyz * u_Light.Intensities[i];
    }
    
    ambient = ambientCoefficient * ambient;

    vec3 BlinnPhong = ambient + diffuse + specular; 
    vec3 final = textureColor.rgb * BlinnPhong;
    final = pow(final, vec3(1.0 / 1.2));

    FragColor = vec4(final * v_Color.xyz, 1.0);
}
