#pragma once

#include "Base.h"

#include "ufbx\ufbx.h"

#include <filesystem>

namespace HBL2
{
	class UFbxLoader
	{
	public:
		bool Load(const std::filesystem::path& path, std::vector<Vertex>& meshData);

	private:
		bool LoadVertexData(const ufbx_node* node, std::vector<Vertex>& meshData);

	private:
		ufbx_scene* m_Scene;
	};
}