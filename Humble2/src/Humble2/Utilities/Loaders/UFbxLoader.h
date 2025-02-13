#pragma once

#include "Base.h"

#include "ufbx\ufbx.h"

#include <filesystem>

namespace HBL2
{
	struct MeshData;
	struct Vertex;

	class UFbxLoader
	{
	public:
		bool Load(const std::filesystem::path& path, MeshData& meshData);

	private:
		bool LoadVertexData(const ufbx_node* node, MeshData& meshData);

	private:
		ufbx_scene* m_Scene;
	};
}