// Common definitions
struct ComputeShaderInput
{
    uint3 groupID : SV_GroupID;                   // 3D index of the thread group in the dispatch.
    uint3 groupThreadID : SV_GroupThreadID;       // 3D index of local thread ID in a thread group.
    uint3 dispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
    uint groupIndex : SV_GroupIndex;              // Flattened local index of the thread within a thread group.
};

// Global variables
struct DispatchParams
{
    // Number of groups dispatched. (This parameter is not available as an Slang system value!)
    uint3 numThreadGroups;
    // uint padding // implicit padding to 16 bytes.

    // Total number of threads dispatched. (Also not available as an Slang system value!)
    // Note: This value may be less than the actual number of threads executed
    // if the screen size is not evenly divisible by the block size.
    uint3 numThreads;
    // uint padding // implicit padding to 16 bytes.
};

// Parameters required to convert screen space coordinates to view space.
struct ScreenToViewParams
{
    float4x4 InverseProjection;
    float4x4 View;
    float2 ScreenDimensions;
    float LightCount;
    // float padding; // implicit padding to 16 bytes.
};

struct Plane
{
    float3 N; // Plane normal.
    float d;  // Distance to origin.
};

// Compute a plane from 3 noncollinear points that form a triangle.
// This equation assumes a right-handed (counter-clockwise winding order)
// coordinate system to determine the direction of the plane normal.
Plane ComputePlane(float3 p0, float3 p1, float3 p2)
{
    Plane plane;

    float3 v0 = p1 - p0;
    float3 v2 = p2 - p0;

    plane.N = normalize(cross(v0, v2));

    // Compute the distance to the origin using p0.
    plane.d = dot(plane.N, p0);

    return plane;
}

// Four planes of a view frustum (in view space).
// The planes are:
//  * Left,
//  * Right,
//  * Top,
//  * Bottom.
// The back and/or front planes can be computed from depth values in the
// light culling compute shader.
struct Frustum
{
    Plane planes[4]; // left, right, top, bottom frustum planes.
};

[[vk::binding(0, 0)]]
ConstantBuffer<DispatchParams> u_DispatchParams;

// View space frustums for the grid cells.
[[vk::binding(1, 0)]]
ConstantBuffer<ScreenToViewParams> u_ScreenToViewParams;

[[vk::binding(2, 0)]]
RWStructuredBuffer<Frustum> u_OutFrustums;

// Compute Shader
static const uint BLOCK_SIZE = 16;

// Convert clip space coordinates to view space
float4 ClipToView(float4 clip)
{
    // View space position.
    float4 view = mul(u_ScreenToViewParams.InverseProjection, clip);
    // Perspective projection.
    view = view / view.w;

    return view;
}

// Convert screen space coordinates to view space.
float4 ScreenToView(float4 screen)
{
    // Convert to normalized texture coordinates
    float2 texCoord = screen.xy / u_ScreenToViewParams.ScreenDimensions;

    // Convert to clip space
    float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

    return ClipToView(clip);
}

// A kernel to compute frustums for the grid
// This kernel is executed once per grid cell. Each thread
// computes a frustum for a grid cell.
[shader("compute")]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void mainCS(ComputeShaderInput input)
{
    // View space eye position is always at the origin.
    const float3 eyePos = float3(0, 0, 0);

    // Compute the 4 corner points on the far clipping plane to use as the frustum vertices.
    float4 screenSpace[4];

    // Top left point
    screenSpace[0] = float4(input.dispatchThreadID.xy * BLOCK_SIZE, -1.0f, 1.0f);
    // Top right point
    screenSpace[1] = float4(float2(input.dispatchThreadID.x + 1, input.dispatchThreadID.y) * BLOCK_SIZE, -1.0f, 1.0f);
    // Bottom left point
    screenSpace[2] = float4(float2(input.dispatchThreadID.x, input.dispatchThreadID.y + 1) * BLOCK_SIZE, -1.0f, 1.0f);
    // Bottom right point
    screenSpace[3] = float4(float2(input.dispatchThreadID.x + 1, input.dispatchThreadID.y + 1) * BLOCK_SIZE, -1.0f, 1.0f);

    float3 viewSpace[4];
    // Now convert the screen space points to view space
    for (int i = 0; i < 4; i++)
    {
        viewSpace[i] = ScreenToView(screenSpace[i]).xyz;
    }

    // Now build the frustum planes from the view space points
    Frustum frustum;

    // Left plane
    frustum.planes[0] = ComputePlane(eyePos, viewSpace[2], viewSpace[0]);
    // Right plane
    frustum.planes[1] = ComputePlane(eyePos, viewSpace[1], viewSpace[3]);
    // Top plane
    frustum.planes[2] = ComputePlane(eyePos, viewSpace[0], viewSpace[1]);
    // Bottom plane
    frustum.planes[3] = ComputePlane(eyePos, viewSpace[3], viewSpace[2]);

    // Store the computed frustum in global memory (if our thread ID is in bounds of the grid).
    if (input.dispatchThreadID.x < u_DispatchParams.numThreads.x && input.dispatchThreadID.y < u_DispatchParams.numThreads.y)
    {
        uint index = input.dispatchThreadID.x + (input.dispatchThreadID.y * u_DispatchParams.numThreads.x);
        u_OutFrustums[index] = frustum;
    }
}