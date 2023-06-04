#version 330 core

layout(location = 0) out vec4 a_Color;

uniform vec2 u_LightPosition;

in vec2 v_Position;
in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexID;

uniform sampler2D u_Textures[32];

void main()
{
	int id = int(v_TexID);
	float intensity = 1.0 / length(v_Position - u_LightPosition);
	a_Color = texture(u_Textures[id], v_TexCoord) * v_Color * intensity * 50.0;
};