#pragma once

#include "Base.h"
#include "Asset/AssetManager.h"
#include "Resources/Types.h"

#include "ModelLoaderCommon.h"
#include "Utilities/Result.h"

#include "ufbx/ufbx.h"

#include <filesystem>

namespace HBL2
{
	struct FbxMeshSkin   // per mesh node, built once
	{
		bool Valid = false;
		const ufbx_skin_deformer* Deformer = nullptr;
		std::vector<uint16_t> ClusterToJoint;   // cluster index -> joint index, 0xFFFF if the bone isn't a joint
		ufbx_matrix GeometryToWorldBind{};      // mesh geometry space -> world at bind pose; vertices are baked with it
		ufbx_matrix NormalMatrix{};
	};

	class UFbxLoader
	{
	public:
		Handle<Mesh> Load(const std::filesystem::path& path);
		void Reload(Asset* asset);

	private:
		void LoadMaterials(ufbx_scene* ufbxScene, const std::filesystem::path& path);
		void ReloadMaterials(ufbx_scene* ufbxScene, const std::filesystem::path& path, Asset* sourceAsset);

		Handle<Asset> LoadMaterial(const std::filesystem::path& path, const ufbx_material* fbxMaterial, ufbx_material_pbr_map materialProperty, JobContext& ctx, bool reload, void* internalData = nullptr);

		Handle<Asset> LoadTexture(const ufbx_texture* texture, JobContext& ctx);
		Handle<Asset> ReloadTexture(const ufbx_texture* texture, JobContext& ctx);

		SkeletonImport BuildSkeleton(const ufbx_scene* scene, const std::filesystem::path& path);
		void BuildAnimationClips(const ufbx_scene* scene, const SkeletonImport& skeleton, std::vector<AnimationClipDescriptor>& out);
		void CreateSkeletonAndClips(const ufbx_scene* scene, SkeletonImport& skeletonImport, Handle<Skeleton>& skeleton, std::vector<Handle<AnimationClip>>& clips);

		Result<MeshPartDescriptor> LoadMeshData(const ufbx_node* node, uint32_t meshIndex, const SkeletonImport& skeleton);
		Result<SubMeshDescriptor> LoadSubMeshVertexData(const ufbx_node* node, uint32_t meshIndex, uint32_t subMeshIndex, const FbxMeshSkin& skin);

	private:
		static thread_local std::vector<Vertex> s_Vertices;
		static thread_local std::vector<uint32_t> s_Indeces;
		static thread_local std::vector<SkinVertex> s_SkinVertices;
		static thread_local std::unordered_map<std::string, Handle<Asset>> s_MaterialNameToAssetHandle;
	};
}
