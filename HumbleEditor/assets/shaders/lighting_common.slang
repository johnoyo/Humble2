module lighting_common;

public struct Light
{
    public float4 ViewPosition;
    public float4 Positions[16];
    public float4 Directions[16];
    public float4 Colors[16];
    public float4 Metadata[16];
    public float4 ShadowData[16];
    public float4x4 LightSpaceMatrices[16];
    public float4 TileUVRange[16];
    public float Count;
};

public struct LightQuery
{
    public float3 worldPosition;
    public uint2 pixelCoord;
}

public interface ILightProvider
{
    uint GetLightCount(LightQuery q);
    Light GetLight(LightQuery q, uint index);
}

[[vk::binding(1, 0)]]
public ConstantBuffer<Light> u_Light;