module lighting_custom;

import lighting_common;

export struct LightProvider : ILightProvider
{
    uint GetLightCount(LightQuery q)
    {
        return 0;
    }

    Light GetLight(LightQuery q, uint index)
    {
        return u_Light[index];
    }
}