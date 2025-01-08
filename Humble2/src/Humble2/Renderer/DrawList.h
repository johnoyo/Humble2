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
		Handle<Material> Material;
		uint32_t Offset;
		uint32_t Size;
	};

	struct GlobalDrawStream
	{
		Handle<BindGroup> BindGroup;
	};

	class DrawList
	{
	public:
		void Insert(const LocalDrawStream&& draw);
		void Reset();

		const std::unordered_map<uint32_t, std::vector<LocalDrawStream>>& GetDraws() const { return m_Draws; };

	private:
		std::unordered_map<uint32_t, std::vector<LocalDrawStream>> m_Draws;
	};
}