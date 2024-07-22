#version 450
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec4 v_Color;

void main()
{
    FragColor = v_Color;
}