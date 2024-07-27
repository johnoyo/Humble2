#version 450
layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_TextureCoord;

layout (binding = 0) uniform sampler2D u_AlbedoMap;

void main()
{
    FragColor = texture(u_AlbedoMap, v_TextureCoord) * v_Color;
}