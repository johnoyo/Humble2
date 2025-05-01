#shader vertex
#version 450 core
layout(location = 0) in vec3 a_Position;

layout(std140, set = 0, binding = 0) uniform Camera
{
    mat4 ViewProjection;
} u_Camera;

layout (location = 0) out vec3 v_WorldPosition;

void main()
{
    v_WorldPosition = a_Position;
	vec4 clipPos = u_Camera.ViewProjection * vec4(a_Position, 1.0);
	gl_Position = clipPos.xyww;
}

#shader fragment
#version 450 core
layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 v_WorldPosition;

layout (set = 1, binding = 1) uniform samplerCube u_EnvironmentMap;

void main()
{
    FragColor = texture(u_EnvironmentMap, v_WorldPosition);
}