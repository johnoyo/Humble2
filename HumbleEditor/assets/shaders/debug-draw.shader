#shader vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in uint a_Color;

layout(std140, set = 0, binding = 0) uniform Camera
{
    mat4 ViewProjection;
} u_Camera;


layout(location = 0) out vec4 v_Color;

vec4 UnpackColor(uint packedColor)
{
    float r = float(packedColor & 0xFFu) / 255.0;
    float g = float((packedColor >> 8u) & 0xFFu) / 255.0;
    float b = float((packedColor >> 16u) & 0xFFu) / 255.0;
    float a = float((packedColor >> 24u) & 0xFFu) / 255.0;

    return vec4(r, g, b, a);
}

void main()
{
    v_Color = UnpackColor(a_Color);
    gl_Position = u_Camera.ViewProjection * vec4(a_Position, 1.0);
}

#shader fragment
#version 450 core
layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_Color;

void main()
{
    FragColor = v_Color;
}
