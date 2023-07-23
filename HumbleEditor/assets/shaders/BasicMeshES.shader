#shader vertex
#version 300 es

precision highp float;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TextureCoord;
layout(location = 3) in float a_TextureID;
layout(location = 4) in float a_Normal;

uniform mat4 u_VP;
uniform mat4 u_M;

out vec4 v_Color;
out vec2 v_TextureCoord;
out float v_TextureID;

void main()
{
	v_Color = a_Color;
	v_TextureCoord = a_TextureCoord;
	v_TextureID = a_TextureID;
    gl_Position = u_VP * u_M * vec4(a_Position, 1.0);
}

#shader fragment
#version 300 es

precision highp float;

layout(location = 0) out vec4 a_Color;

uniform vec4 u_Color;
in vec4 v_Color;
in vec2 v_TextureCoord;
in float v_TextureID;

uniform sampler2D u_Textures0;
uniform sampler2D u_Textures1;
uniform sampler2D u_Textures2;
uniform sampler2D u_Textures3;
uniform sampler2D u_Textures4;
uniform sampler2D u_Textures5;
uniform sampler2D u_Textures6;
uniform sampler2D u_Textures7;
uniform sampler2D u_Textures8;
uniform sampler2D u_Textures9;
uniform sampler2D u_Textures10;
uniform sampler2D u_Textures11;
uniform sampler2D u_Textures12;
uniform sampler2D u_Textures13;
uniform sampler2D u_Textures14;
uniform sampler2D u_Textures15;

void main()
{
    int id = int(v_TextureID + 0.1	);

	vec4 texColor = v_Color;

	switch (id)
	{
	case  0: texColor = v_Color * texture(u_Textures0,  v_TextureCoord); break;
	case  1: texColor = v_Color * texture(u_Textures1,  v_TextureCoord); break;
	case  2: texColor = v_Color * texture(u_Textures2,  v_TextureCoord); break;
	case  3: texColor = v_Color * texture(u_Textures3,  v_TextureCoord); break;
	case  4: texColor = v_Color * texture(u_Textures4,  v_TextureCoord); break;
	case  5: texColor = v_Color * texture(u_Textures5,  v_TextureCoord); break;
	case  6: texColor = v_Color * texture(u_Textures6,  v_TextureCoord); break;
	case  7: texColor = v_Color * texture(u_Textures7,  v_TextureCoord); break;
	case  8: texColor = v_Color * texture(u_Textures8,  v_TextureCoord); break;
	case  9: texColor = v_Color * texture(u_Textures9,  v_TextureCoord); break;
	case 10: texColor = v_Color * texture(u_Textures10, v_TextureCoord); break;
	case 11: texColor = v_Color * texture(u_Textures11, v_TextureCoord); break;
	case 12: texColor = v_Color * texture(u_Textures12, v_TextureCoord); break;
	case 13: texColor = v_Color * texture(u_Textures13, v_TextureCoord); break;
	case 14: texColor = v_Color * texture(u_Textures14, v_TextureCoord); break;
	case 15: texColor = v_Color * texture(u_Textures15, v_TextureCoord); break;
	}

	a_Color = texColor;
}