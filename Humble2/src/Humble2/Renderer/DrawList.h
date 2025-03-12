#pragma once

#include "Resources\Types.h"
#include "Resources\Handle.h"

#include <functional>
#include <unordered_map>

namespace HBL2
{
	struct LocalDrawStream
	{
		Handle<Shader> Shader;
		Handle<BindGroup> BindGroup;
		Handle<Mesh> Mesh;
		uint32_t MeshIndex = 0;
		uint32_t SubMeshIndex = 0;
		Handle<Material> Material;
		uint32_t Offset = 0;
		uint32_t Size = 0;
	};

	struct GlobalDrawStream
	{
		Handle<BindGroup> BindGroup;
		uint32_t DynamicUniformBufferOffset;
		uint32_t DynamicUniformBufferSize;
	};

	class DrawList
	{
	public:
		void Insert(const LocalDrawStream&& draw);
		const uint32_t GetCount() const { return m_Count; }
		void Reset();

		const std::unordered_map<uint32_t, std::vector<LocalDrawStream>>& GetDraws() const { return m_Draws; };

	private:
		uint32_t m_Count = 0;
		std::unordered_map<uint32_t, std::vector<LocalDrawStream>> m_Draws;
	};
}