#shader vertex
attribute highp vec2 a_Position;
attribute highp vec4 a_Color;
attribute highp vec2 a_TextureCoord;
attribute highp float a_TextureID;

uniform highp mat4 u_MVP;

varying highp vec4 v_Color;
varying highp vec2 v_TextureCoord;
varying highp float v_TextureID;

void main()
{
	v_Color = a_Color;
	v_TextureCoord = a_TextureCoord;
	v_TextureID = a_TextureID;
	gl_Position = u_MVP * vec4(a_Position, 1.0, 1.0);
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
varying highp vec2 v_TextureCoord;
varying highp float v_TextureID;

void main()
{
	int id = int(v_TextureID);

	vec4 texColor = v_Color;

	if (id == 0)
		texColor = v_Color * texture2D(u_Textures0, v_TextureCoord);
	else if (id == 1)
		texColor = v_Color * texture2D(u_Textures1, v_TextureCoord);
	else if (id == 2)
		texColor = v_Color * texture2D(u_Textures2, v_TextureCoord);
	else if (id == 3)
		texColor = v_Color * texture2D(u_Textures3, v_TextureCoord);
	else if (id == 4)
		texColor = v_Color * texture2D(u_Textures4, v_TextureCoord);
	else if (id == 5)
		texColor = v_Color * texture2D(u_Textures5, v_TextureCoord);
	else if (id == 6)
		texColor = v_Color * texture2D(u_Textures6, v_TextureCoord);
	else if (id == 7)
		texColor = v_Color * texture2D(u_Textures7, v_TextureCoord);

	gl_FragColor = texColor;
}