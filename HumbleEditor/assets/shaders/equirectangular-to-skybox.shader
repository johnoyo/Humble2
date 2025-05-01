#shader compute
#version 450 core

layout (local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0) uniform sampler2D u_EquirectMap;
layout(set = 0, binding = 1, rgba16f) uniform writeonly imageCube u_CubeMap;
layout(std140, set = 0, binding = 2) uniform CaptureMatrices
{
    mat4 view[6];
    float  faceSize; // 512
} u_CaptureMatrices;

const mat4 captureProjection = mat4(
    vec4(1, 0, 0, 0),
    vec4(0, 1, 0, 0),
    vec4(0, 0, 1, 1),
    vec4(0, 0,-1, 0)
);

const float PI = 3.14159265359;
vec2 dirToUV(vec3 dir)
{
    float phi   = atan(dir.z, dir.x);
    float theta = asin(dir.y);
    return vec2((phi + PI) / (2.0 * PI), (theta + PI/2.0) / PI);
}

void main()
{
    // ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    // int face = int(gl_GlobalInvocationID.z);
    // int faceSize = int(u_CaptureMatrices.faceSize);

    // if (pix.x >= faceSize || pix.y >= faceSize) return;

    // // convert pixel to NDC [-1,1]
    // vec2 uv = (vec2(pix) / float(faceSize - 1)) * 2.0 - 1.0;

    // // reconstruct direction in world space
    // vec4 target = captureProjection * u_CaptureMatrices.view[face] * vec4(uv, 1.0, 1.0);
    // vec3 dir = normalize(target.xyz / target.w);

    // // sample the equirectangular map
    // vec3 color = texture(u_EquirectMap, dirToUV(dir)).rgb;

    // // write into the appropriate layer
    // imageStore(u_CubeMap, ivec3(pix, face), vec4(color, 1.0));


    // 1) Determine this work-item's pixel and face
    ivec2 pix  = ivec2(gl_GlobalInvocationID.xy);
    int   face = int(gl_GlobalInvocationID.z);
    int   N    = int(u_CaptureMatrices.faceSize);

    if (pix.x >= N || pix.y >= N) return;

    // 2) Compute a [-1,1] screen coordinate
    vec2 uv = (vec2(pix) / float(N - 1)) * 2.0 - 1.0;

    // 3) Build a 90° FOV ray in *view space*, then rotate by the view matrix
    vec4 viewDir = vec4(uv.x, uv.y, -1.0, 0.0);
    vec3 dir     = (u_CaptureMatrices.view[face] * viewDir).xyz;
    dir          = normalize(dir);

    // 4) Sample the equirectangular map
    vec3 color = texture(u_EquirectMap, dirToUV(dir)).rgb;

    // 5) Write into the cubemap layer
    imageStore(u_CubeMap, ivec3(pix, face), vec4(color, 1.0));
}
