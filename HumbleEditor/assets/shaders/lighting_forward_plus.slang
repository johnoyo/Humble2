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
        uint2 tileIndex = uint2(floor(q.worldPosition.xy / TILE_SIZE));
        uint lightCount = u_LightGrid[tileIndex].y;
        return lightCount;
    }

    uint GetLightIndex(LightQuery q, uint index)
    {
        uint2 tileIndex = uint2(floor(q.worldPosition.xy / TILE_SIZE));
        uint startOffset = u_LightGrid[tileIndex].x;
        uint lightIndex = u_LightIndexList[startOffset + index];
        return lightIndex;
    }
}