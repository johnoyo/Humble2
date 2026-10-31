#pragma once

#include "Base.h"
#include "Resources/Handle.h"
#include "Resources/Types.h"
#include "Loaders/UFbxLoader.h"
#include "Loaders/FastGltfLoader.h"

#include <filesystem>

namespace HBL2
{
	enum class BuiltInMesh
	{
		PLANE,
		TESSELATED_PLANE,
		CUBE,
		SPHERE,
		CYLINDER,
		CAPSULE,
		TORUS,
	};

	struct MeshSettings
	{
		bool PreserveHierachy = true;
		bool GenerateColliders = false;
		bool ReimportTexturesOnReload = false;
	};

	struct ShaderReflectionData;

	class HBL2_API MeshUtilities
	{
	public:
		MeshUtilities(const MeshUtilities&) = delete;

		static MeshUtilities& Get();

		static void Initialize();
		static void Shutdown();

		Handle<Mesh> Load(const std::filesystem::path& path);
		void Reload(Asset* asset);

		void LoadBuiltInMeshes();
		void DeleteBuiltInMeshes();
		Handle<Mesh> GetBuiltInLoadedMeshHandle(BuiltInMesh builtInMesh);
		Handle<Asset> GetBuiltInLoadedMeshAssetHandle(BuiltInMesh builtInMesh);
		Span<const Handle<Asset>> GetBuiltInMeshAssets();

		void CreateMeshMetadataFile(Handle<Asset> handle);
		void SerializeAssetMetadataFile(Handle<Asset> handle, const MeshSettings& settings);
		MeshSettings DeserializeAssetMetadataFile(Handle<Asset> handle);
		MeshSettings DeserializeAssetMetadataFile(Asset* asset);

		void GetShaderAssetHandleAndReflectionData(Handle<Asset>& shaderAssetHandle, ShaderReflectionData* shaderReflectionData, bool isPBR);

	private:
		MeshUtilities();

		UFbxLoader* m_UFbxLoader = nullptr;
		FastGltfLoader* m_FastGltfLoader = nullptr;

		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		DArray<Handle<Asset>> m_BuiltInMeshAssets = MakeEmptyDArray<Handle<Asset>>();
		HMap<BuiltInMesh, Handle<Asset>> m_LoadedBuiltInMeshAssets = MakeEmptyHMap<BuiltInMesh, Handle<Asset>>();
		HMap<BuiltInMesh, Handle<Mesh>> m_LoadedBuiltInMeshes = MakeEmptyHMap<BuiltInMesh, Handle<Mesh>>();

		static MeshUtilities* s_Instance;
	};
}
