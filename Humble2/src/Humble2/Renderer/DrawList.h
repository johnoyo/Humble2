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
		DrawList& PerShader(std::function<void(LocalDrawStream&)> func);
		DrawList& PerDraw(std::function<void(LocalDrawStream&)> func);
		void Iterate();
		std::unordered_map<uint32_t, std::vector<LocalDrawStream>> m_Draws;
	private:
		std::function<void(LocalDrawStream&)> m_PerDrawFunc;
		std::function<void(LocalDrawStream&)> m_PerShaderFunc;
	};
}