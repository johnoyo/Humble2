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
    float2 ScreenDimensions;
    float LightCount;
    // float padding; // implicit padding to 16 bytes.
};

public struct Light
{
    public float4 Position;
    public float4 Direction;
    public float4 Color;
    public float4 Metadata;
    public float4 ShadowData;
    public float4x4 LightSpaceMatrix;
    public float4 TileUVRange;
};

struct Plane
{
    float3 N; // Plane normal.
    float d;  // Distance to origin.
};

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

struct Sphere
{
    float3 c;   // Center point.
    float  r;   // Radius.
};

struct Cone
{
    float3 T; // Cone tip.
    float h;  // Height of the cone.
    float3 d; // Direction of the cone.
    float r;  // bottom radius of the cone.
};

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

// Check to see if a sphere is fully behind (inside the negative halfspace of) a plane.
// Source: Real-time collision detection, Christer Ericson (2005)
bool SphereInsidePlane(Sphere sphere, Plane plane)
{
    return dot(plane.N, sphere.c) - plane.d < -sphere.r;
}

// Check to see of a light is partially contained within the frustum.
bool SphereInsideFrustum(Sphere sphere, Frustum frustum, float zNear, float zFar)
{
    bool result = true;

    // First check depth
    // Note: Here, the view vector points in the -Z axis so the
    // far depth value will be approaching -infinity.
    if (sphere.c.z - sphere.r > zNear || sphere.c.z + sphere.r < zFar)
    {
        result = false;
    }

    // Then check frustum planes
    for (int i = 0; i < 4 && result; i++)
    {
        if (SphereInsidePlane(sphere, frustum.planes[i]))
        {
            result = false;
        }
    }

    return result;
}

// Check to see if a point is fully behind (inside the negative halfspace of) a plane.
bool PointInsidePlane(float3 p, Plane plane)
{
    return dot(plane.N, p) - plane.d < 0;
}

// Check to see if a cone if fully behind (inside the negative halfspace of) a plane.
// Source: Real-time collision detection, Christer Ericson (2005)
bool ConeInsidePlane(Cone cone, Plane plane)
{
    // Compute the farthest point on the end of the cone to the positive space of the plane.
    float3 m = cross(cross(plane.N, cone.d), cone.d);
    float3 Q = cone.T + cone.d * cone.h - m * cone.r;

    // The cone is in the negative halfspace of the plane if both
    // the tip of the cone and the farthest point on the end of the cone to the
    // positive halfspace of the plane are both inside the negative halfspace
    // of the plane.
    return PointInsidePlane(cone.T, plane) && PointInsidePlane(Q, plane);
}

bool ConeInsideFrustum(Cone cone, Frustum frustum, float zNear, float zFar)
{
    bool result = true;

    Plane nearPlane = { float3(0, 0, -1), -zNear };
    Plane farPlane = { float3(0, 0, 1), zFar };

    // First check the near and far clipping planes.
    if (ConeInsidePlane(cone, nearPlane) || ConeInsidePlane(cone, farPlane))
    {
        result = false;
    }

    // Then check frustum planes
    for (int i = 0; i < 4 && result; i++)
    {
        if (ConeInsidePlane(cone, frustum.planes[i]))
        {
            result = false;
        }
    }

    return result;
}

[[vk::binding(0, 0)]]
ConstantBuffer<DispatchParams> u_DispatchParams;

[[vk::binding(1, 0)]]
ConstantBuffer<ScreenToViewParams> u_ScreenToViewParams;

[[vk::binding(2, 0)]]
Texture2D u_DepthTexture;

[[vk::binding(3, 0)]]
StructuredBuffer<Frustum> u_Frustums;

[[vk::binding(4, 0)]]
StructuredBuffer<Light> u_Lights;

// Global counter for current index into the light index list.
// "o_" prefix indicates light lists for opaque geometry while
// "t_" prefix indicates light lists for transparent geometry.
[[vk::binding(5, 0)]]
RWStructuredBuffer<uint> o_LightIndexCounter;
[[vk::binding(6, 0)]]
RWStructuredBuffer<uint> t_LightIndexCounter;

// Light index lists and light grids.
[[vk::binding(7, 0)]]
RWStructuredBuffer<uint> o_LightIndexList;
[[vk::binding(8, 0)]]
RWStructuredBuffer<uint> t_LightIndexList;
[[vk::binding(9, 0)]]
RWTexture2D<uint2> o_LightGrid;
[[vk::binding(10, 0)]]
RWTexture2D<uint2> t_LightGrid;

groupshared uint uMinDepth;
groupshared uint uMaxDepth;

groupshared Frustum GroupFrustum;

// Opaque geometry light lists.
groupshared uint o_LightCount;
groupshared uint o_LightIndexStartOffset;
groupshared uint o_LightList[1024];

// Transparent geometry light lists.
groupshared uint t_LightCount;
groupshared uint t_LightIndexStartOffset;
groupshared uint t_LightList[1024];

// Add the light to the visible light list for opaque geometry.
void o_AppendLight(uint lightIndex)
{
    uint index; // Index into the visible lights array.
    InterlockedAdd(o_LightCount, 1, index);
    if (index < 1024)
    {
        o_LightList[index] = lightIndex;
    }
}

// Add the light to the visible light list for transparent geometry.
void t_AppendLight(uint lightIndex)
{
    uint index; // Index into the visible lights array.
    InterlockedAdd(t_LightCount, 1, index);
    if (index < 1024)
    {
        t_LightList[index] = lightIndex;
    }
}

static const uint BLOCK_SIZE = 16;

struct ComputeShaderInput
{
    uint3 groupID : SV_GroupID;                   // 3D index of the thread group in the dispatch.
    uint3 groupThreadID : SV_GroupThreadID;       // 3D index of local thread ID in a thread group.
    uint3 dispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
    uint groupIndex : SV_GroupIndex;              // Flattened local index of the thread within a thread group.
};

// Implementation of light culling compute shader is based on the presentation
// "DirectX 11 Rendering in Battlefield 3" (2011) by Johan Andersson, DICE.
// Retrieved from: http://www.slideshare.net/DICEStudio/directx-11-rendering-in-battlefield-3
// Retrieved: July 13, 2015
// And "Forward+: A Step Toward Film-Style Shading in Real Time", Takahiro Harada (2012)
// published in "GPU Pro 4", Chapter 5 (2013) Taylor & Francis Group, LLC.
[shader("compute")]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void mainCS(ComputeShaderInput input)
{
    // Calculate min & max depth in threadgroup / tile.
    int2 texCoord = input.dispatchThreadID.xy;
    float fDepth = u_DepthTexture.Load(int3(texCoord, 0)).r;

    uint uDepth = asuint(fDepth);

    if (input.groupIndex == 0) // Avoid contention by other threads in the group.
    {
        uMinDepth = 0xffffffff;
        uMaxDepth = 0;
        o_LightCount = 0;
        t_LightCount = 0;
        GroupFrustum = u_Frustums[input.groupID.x + (input.groupID.y * u_DispatchParams.numThreadGroups.x)];
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin(uMinDepth, uDepth);
    InterlockedMax(uMaxDepth, uDepth);

    GroupMemoryBarrierWithGroupSync();

    float fMinDepth = asfloat(uMinDepth);
    float fMaxDepth = asfloat(uMaxDepth);

    // Convert depth values to view space.
    float minDepthVS = ClipToView(float4(0, 0, fMinDepth, 1)).z;
    float maxDepthVS = ClipToView(float4(0, 0, fMaxDepth, 1)).z;
    float nearClipVS = ClipToView(float4(0, 0, 0, 1)).z;

    // Clipping plane for minimum depth value
    // (used for testing lights within the bounds of opaque geometry).
    Plane minPlane = { float3(0, 0, -1), -minDepthVS };

    // Cull lights
    // Each thread in a group will cull 1 light until all lights have been culled.
    for (uint i = input.groupIndex; i < u_ScreenToViewParams.LightCount; i += BLOCK_SIZE * BLOCK_SIZE)
    {
        // if (u_Lights[i].Enabled) // Not need since only if the light is enabled it gets added to the u_Lights buffer.
        {
            Light light = u_Lights[i];

            uint lightType = uint(light.Position.w);
            float lightRange = light.Color.w;

            switch (lightType)
            {
            case 0: // Directional light
                {
                    // Directional lights always get added to our light list.
                    // (Hopefully there are not too many directional lights!)
                    t_AppendLight(i);
                    o_AppendLight(i);
                    break;
                }
            case 1: // Point light
                {
                    Sphere sphere = { light.Position.xyz, lightRange };
                    if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
                    {
                        // Add light to light list for transparent geometry.
                        t_AppendLight(i);

                        if (!SphereInsidePlane(sphere, minPlane))
                        {
                            // Add light to light list for opaque geometry.
                            o_AppendLight(i);
                        }
                    }
                    break;
                }
                case 2: // Spot light
                {
                    float spotlightAngle = light.Metadata.w;
                    float coneRadius = tan(radians(spotlightAngle)) * lightRange;
                    Cone cone = { light.Position.xyz, lightRange, light.Direction.xyz, coneRadius };
                    if (ConeInsideFrustum(cone, GroupFrustum, nearClipVS, maxDepthVS))
                    {
                        // Add light to light list for transparent geometry.
                        t_AppendLight(i);

                        if (!ConeInsidePlane(cone, minPlane))
                        {
                            // Add light to light list for opaque geometry.
                            o_AppendLight(i);
                        }
                    }
                    break;
                }
            }
        }
    }

    // Wait till all threads in group have caught up.
    GroupMemoryBarrierWithGroupSync();

    // Update global memory with visible light buffer.
    // First update the light grid (only thread 0 in group needs to do this)
    if (input.groupIndex == 0)
    {
        // Update light grid for opaque geometry.
        InterlockedAdd(o_LightIndexCounter[0], o_LightCount, o_LightIndexStartOffset);
        o_LightGrid[input.groupID.xy] = uint2(o_LightIndexStartOffset, o_LightCount);

        // Update light grid for transparent geometry.
        InterlockedAdd(t_LightIndexCounter[0], t_LightCount, t_LightIndexStartOffset);
        t_LightGrid[input.groupID.xy] = uint2(t_LightIndexStartOffset, t_LightCount);
    }

    GroupMemoryBarrierWithGroupSync();

    // Now update the light index list (all threads).
    // For opaque geometry.
    for (uint i = input.groupIndex; i < o_LightCount; i += BLOCK_SIZE * BLOCK_SIZE)
    {
        o_LightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
    }
    // For transparent geometry.
    for (uint i = input.groupIndex; i < t_LightCount; i += BLOCK_SIZE * BLOCK_SIZE)
    {
        t_LightIndexList[t_LightIndexStartOffset + i] = t_LightList[i];
    }
}