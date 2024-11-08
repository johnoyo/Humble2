#shader vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TextureCoord;

layout(std140, set = 0, binding = 0) uniform Camera
{
    mat4 ViewProjection;
} u_Camera;

layout(std140, set = 1, binding = 2) uniform Object
{
    mat4 Model;
    vec4 Color;
} u_Object;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_TextureCoord;

void main()
{
    v_Color = u_Object.Color;
    v_TextureCoord = a_TextureCoord;
    gl_Position = u_Camera.ViewProjection * u_Object.Model * vec4(a_Position, 1.0);
}

#shader fragment
#version 450 core
layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_TextureCoord;

layout (set = 1, binding = 0) uniform sampler2D u_AlbedoMap;

void main()
{
    FragColor = texture(u_AlbedoMap, v_TextureCoord) * v_Color;
}
