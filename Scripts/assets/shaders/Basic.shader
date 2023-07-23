#shader vertex
#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TextureCoord;
layout(location = 3) in float a_TextureID;
layout(location = 4) in float a_Normal;

uniform mat4 u_MVP;

out vec4 v_Color;
out vec2 v_TextureCoord;
out float v_TextureID;

void main()
{
	v_Color = a_Color;
	v_TextureCoord = a_TextureCoord;
	v_TextureID = a_TextureID;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 a_Color;

in vec4 v_Color;
in vec2 v_TextureCoord;
in float v_TextureID;

uniform sampler2D u_Textures[32];

void main()
{
    int id = int(v_TextureID + 0.1);
	a_Color = texture(u_Textures[id], v_TextureCoord) * v_Color;
};