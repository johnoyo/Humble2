#pragma once

#include "Base.h"
#include "Asset\Asset.h"
#include "Resources\Types.h"

#include "Utilities\Result.h"

#include "ufbx\ufbx.h"

#include <filesystem>

namespace HBL2
{
	class UFbxLoader
	{
	public:
		Handle<Mesh> Load(const std::filesystem::path& path);

	private:
		void LoadMaterials(const std::filesystem::path& path);
		Handle<Asset> LoadMaterial(const std::filesystem::path& path, const ufbx_material* fbxMaterial, ufbx_material_pbr_map materialProperty, void* internalData = nullptr);
		Handle<Asset> LoadTexture(const ufbx_texture* texture);

		Result<MeshPartDescriptor> LoadMeshData(const ufbx_node* node, uint32_t meshIndex);
		Result<SubMeshDescriptor> LoadSubMeshVertexData(const ufbx_node* node, uint32_t meshIndex, uint32_t subMeshIndex);

	private:
		ufbx_scene* m_Scene;
		std::vector<Vertex> m_Vertices;
		std::vector<uint32_t> m_Indeces;
		std::unordered_map<const char*, Handle<Material>> m_MaterialNameToHandle;
	};
}