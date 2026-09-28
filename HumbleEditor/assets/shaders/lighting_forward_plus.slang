module lighting_forward_plus;

import lighting_common;

[[vk::binding(3, 0)]]
StructuredBuffer<uint> u_LightIndexList;

[[vk::binding(4, 0)]]
Texture2D<uint2> u_LightGrid;

static const uint TILE_SIZE = 16;

export struct LightProvider : ILightProvider
{
    uint GetLightCount(LightQuery q)
    {
        return 0;
    }

    Light GetLight(LightQuery q, uint index)
    {
        return u_Light;
    }
}