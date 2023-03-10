#shader vertex
attribute highp vec4 a_Position;
attribute highp vec4 a_Color;
attribute highp vec2 a_TexCoord;
attribute highp float a_TexID;

uniform highp mat4 u_MVP;

varying highp vec4 v_Color;
varying highp vec2 v_TexCoord;
varying highp float v_TexID;

void main()
{
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
	v_TexID = a_TexID;
	gl_Position = u_MVP * a_Position;
}

#shader fragment
precision highp float;

uniform sampler2D u_Textures0;
uniform sampler2D u_Textures1;
uniform sampler2D u_Textures2;
uniform sampler2D u_Textures3;
uniform sampler2D u_Textures4;
uniform sampler2D u_Textures5;
uniform sampler2D u_Textures6;
uniform sampler2D u_Textures7;

varying highp vec4 v_Color;
varying highp vec2 v_TexCoord;
varying highp float v_TexID;

void main()
{
	int id = int(v_TexID);

	vec4 texColor = v_Color;

	if (id == 0)
		texColor = v_Color * texture2D(u_Textures0, v_TexCoord);
	else if (id == 1)
		texColor = v_Color * texture2D(u_Textures1, v_TexCoord);
	else if (id == 2)
		texColor = v_Color * texture2D(u_Textures2, v_TexCoord);
	else if (id == 3)
		texColor = v_Color * texture2D(u_Textures3, v_TexCoord);
	else if (id == 4)
		texColor = v_Color * texture2D(u_Textures4, v_TexCoord);
	else if (id == 5)
		texColor = v_Color * texture2D(u_Textures5, v_TexCoord);
	else if (id == 6)
		texColor = v_Color * texture2D(u_Textures6, v_TexCoord);
	else if (id == 7)
		texColor = v_Color * texture2D(u_Textures7, v_TexCoord);

	gl_FragColor = texColor;
}