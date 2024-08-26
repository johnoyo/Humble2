#version 450
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TextureCoord;

layout(std140, binding = 0) uniform Camera
{
    mat4 ViewProjection;
} u_Camera;

layout(std140, binding = 1) uniform Object
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