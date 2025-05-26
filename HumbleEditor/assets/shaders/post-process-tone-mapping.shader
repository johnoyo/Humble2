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
layout (std140, set = 0, binding = 1) uniform CameraSettings
{
    float Exposure;
    float Gamma;
} u_CameraSettings;

mat3 ACESInputMat = mat3(
    0.59719, 0.35458, 0.04823,
    0.07600, 0.90834, 0.01566,
    0.02840, 0.13383, 0.83777
);

mat3 ACESOutputMat = mat3(
     1.60475, -0.53108, -0.07367,
    -0.10208,  1.10813, -0.00605,
    -0.00327, -0.07276,  1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = transpose(ACESInputMat) * color;
    color = RRTAndODTFit(color);
    color = transpose(ACESOutputMat) * color;
    color = clamp(color, 0, 1);
    return color;
}

void main()
{
    vec3 hdrColor = texture(u_FullScreenTexture, v_TextureCoord).rgb;
  
    // Apply exposure
    vec3 exposedColor = hdrColor * u_CameraSettings.Exposure;

    // Apply ACES tone mapping
    vec3 acesColor = ACESFitted(exposedColor);

    // Gamma correction
    vec3 mapped = pow(acesColor, vec3(1.0 / u_CameraSettings.Gamma));
  
    FragColor = vec4(mapped, 1.0);
}