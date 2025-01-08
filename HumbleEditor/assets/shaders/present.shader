#shader vertex
#version 450 core
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TextureCoord;

layout (location = 0) out vec2 v_TextureCoord;

void main()
{
    v_TextureCoord = a_TextureCoord;
    gl_Position = vec4(a_Position.x, a_Position.y, 0.0, 1.0);
}

#shader fragment
#version 450 core

layout (location = 0 ) in vec2 v_TextureCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform sampler2D u_FullScreenTexture;

void main()
{
    vec3 color = texture(u_FullScreenTexture, v_TextureCoord).xyz;
    FragColor = vec4(color, 1.0f);
}