module lighting_custom;

import lighting_common;

export struct LightProvider : ILightProvider
{
    uint GetLightCount(LightQuery q)
    {
        return 0;
    }

    uint GetLightIndex(LightQuery q, uint index)
    {
        return index;
    }
}