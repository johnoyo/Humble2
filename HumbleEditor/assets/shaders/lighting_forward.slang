module lighting_forward;

import lighting_common;

export struct LightProvider : ILightProvider
{
    uint GetLightCount(LightQuery q)
    {
        return uint(1);
    }

    Light GetLight(LightQuery q, uint index)
    {
        return u_Light[index];
    }
}