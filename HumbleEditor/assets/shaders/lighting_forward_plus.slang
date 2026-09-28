module lighting_forward_plus;

import lighting_common;

[[vk::binding(3, 0)]]
public StructuredBuffer<uint> u_LightIndexList : register(t2);

[[vk::binding(4, 0)]]
public Texture2D<uint2> u_LightGrid : register(t1);

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