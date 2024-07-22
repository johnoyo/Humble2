#version 450
layout(location = 0) in vec3 a_Pos;

layout(std140, binding = 0) uniform Camera
{
    mat4 ModelViewProjection;
} u_Camera;

layout(std140, binding = 1) uniform Object
{
    vec4 Color;
} u_Object;

layout(location = 0) out vec4 v_Color;

void main()
{
    v_Color = u_Object.Color;
    gl_Position = u_Camera.ModelViewProjection * vec4(a_Pos, 1.0);
}