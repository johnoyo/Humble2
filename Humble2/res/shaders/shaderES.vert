
attribute highp vec2 a_Position;
attribute highp vec4 a_Color;
attribute highp vec2 a_TextureCoord;
attribute highp float a_TextureID;

uniform highp mat4 u_MVP;

varying highp vec4 v_Position;
varying highp vec4 v_Color;
varying highp vec2 v_TextureCoord;
varying highp float v_TextureID;

void main()
{
	v_Position = a_Position;
	v_Color = a_Color;
	v_TextureCoord = a_TextureCoord;
	v_TextureID = a_TextureID;
	gl_Position = u_MVP * vec4(a_Position, 1.0, 1.0);
}
