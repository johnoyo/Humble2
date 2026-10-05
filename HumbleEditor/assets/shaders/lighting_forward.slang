module lighting_forward;

import lighting_common;

export struct LightProvider : ILightProvider
{
    uint GetLightCount(LightQuery q)
    {
        return q.lightCount;
    }

    uint GetLightIndex(LightQuery q, uint index)
    {
        return index;
    }
}