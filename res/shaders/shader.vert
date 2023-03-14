#version 330 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexID;

uniform mat4 u_MVP;

out vec2 v_Position;
out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexID;

void main()
{
	v_Position = a_Position;
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
	v_TexID = a_TexID;
	gl_Position = u_MVP * vec4(a_Position, 1.0, 1.0);
};