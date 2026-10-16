struct InputVertex
{
	float p[3];
	float n[3];
	float uv[2];
};

struct OutputVertex
{
	float p[3];
	float n[3];
	float uv[2];
};

struct SkinVertex
{
	uint joints01;
	uint joints23;
	float w0;
	float w1;
	float w2;
	float w3;
};

[[vk::binding(0, 0)]]
StructuredBuffer<InputVertex> u_InputVertices;

[[vk::binding(1, 0)]]
RWStructuredBuffer<OutputVertex> u_OutputVertices;

[[vk::binding(2, 0)]]
StructuredBuffer<float4x4> u_JointMatrices;

[[vk::binding(3, 0)]]
StructuredBuffer<SkinVertex> u_SkinVertices;

[[vk::binding(4, 0)]]
ConstantBuffer<uint> u_JointMatricesOffset;

[shader("compute")]
[numthreads(64, 1, 1)]
void mainCS(uint3 tid : SV_DispatchThreadID)
{
    uint id = tid.x;

    uint count, stride;
    u_InputVertices.GetDimensions(count, stride);
    if (id >= count)
    {
		return;
	}

	InputVertex v = u_InputVertices[id];
	SkinVertex s = u_SkinVertices[id];

	// little-endian: Joints[0] is the low 16 bits
	uint4 j = uint4(s.joints01 & 0xFFFF, s.joints01 >> 16, s.joints23 & 0xFFFF, s.joints23 >> 16);
	float4 w = float4(s.w0, s.w1, s.w2, s.w3);

    // Build the blended skin matrix from the four joint influences.
    // This is the core of the Linear Blend Skinning (LBS) equation:
    // SkinMatrix = sum(weight_i * JointMatrix_i) for i in [0, 3]
    float4x4 m = w.x * u_JointMatrices[u_JointMatricesOffset + j.x]
			   + w.y * u_JointMatrices[u_JointMatricesOffset + j.y]
			   + w.z * u_JointMatrices[u_JointMatricesOffset + j.z]
			   + w.w * u_JointMatrices[u_JointMatricesOffset + j.w];

    // Transform the position using the full 4x4 skin matrix.
	float3 pos = mul(m, float4(v.p[0], v.p[1], v.p[2], 1.0)).xyz;

    // Normals use only the 3x3 rotational portion (no translation).
	float3x3 r = float3x3(m[0].xyz, m[1].xyz, m[2].xyz);
	float3 nrm = normalize(mul(r, float3(v.n[0], v.n[1], v.n[2])));

    // Write the animated data to the output buffer element-by-element.
	OutputVertex o;
	o.p[0] = pos.x;
	o.p[1] = pos.y;
	o.p[2] = pos.z;
	o.n[0] = nrm.x;
	o.n[1] = nrm.y;
	o.n[2] = nrm.z;

    // UV coordinates are not affected by skinning.
	o.uv[0] = v.uv[0];
	o.uv[1] = v.uv[1];

	u_OutputVertices[id] = o;
}