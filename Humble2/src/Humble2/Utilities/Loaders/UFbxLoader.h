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
		bool LoadMeshData(const ufbx_node* node, uint32_t meshIndex, MeshData& meshData);
		bool LoadVertexData(const ufbx_node* node, uint32_t meshIndex, uint32_t subMeshIndex, MeshData& meshData);

	private:
		ufbx_scene* m_Scene;
	};
}