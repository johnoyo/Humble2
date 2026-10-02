module lighting_common;

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

[[vk::binding(1, 0)]]
public StructuredBuffer<Light> u_Light : register(t1);

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