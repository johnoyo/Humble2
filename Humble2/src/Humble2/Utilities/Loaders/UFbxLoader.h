#pragma once

#include "Base.h"
#include "Asset\AssetManager.h"
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
		void Reload(Asset* asset);

	private:
		void LoadMaterials(ufbx_scene* ufbxScene, const std::filesystem::path& path);
		void ReloadMaterials(ufbx_scene* ufbxScene, const std::filesystem::path& path);
		Handle<Asset> LoadMaterial(const std::filesystem::path& path, const ufbx_material* fbxMaterial, ufbx_material_pbr_map materialProperty, JobContext& ctx, ResourceTask<Texture>*& textureTask, bool reload, void* internalData = nullptr);
		Handle<Asset> LoadTexture(const ufbx_texture* texture, JobContext& ctx, ResourceTask<Texture>*& resourceTask);
		Handle<Asset> ReloadTexture(const ufbx_texture* texture, JobContext& ctx, ResourceTask<Texture>*& resourceTask);
		void CleanUpResourceTasks(ResourceTask<Texture>* albedoMapTask, ResourceTask<Texture>* normalMapTask, ResourceTask<Texture>* roughnessMapTask, ResourceTask<Texture>* metallicMapTask);

		Result<MeshPartDescriptor> LoadMeshData(const ufbx_node* node, uint32_t meshIndex);
		Result<SubMeshDescriptor> LoadSubMeshVertexData(const ufbx_node* node, uint32_t meshIndex, uint32_t subMeshIndex);

	private:
		static thread_local std::vector<Vertex> s_Vertices;
		static thread_local std::vector<uint32_t> s_Indeces;
		static thread_local std::unordered_map<std::string, Handle<Material>> s_MaterialNameToHandle;
	};
}