#pragma once

#include "Base.h"
#include "Resources\Handle.h"
#include "Resources\Types.h"
#include "Loaders\UFbxLoader.h"
#include "Loaders\FastGltfLoader.h"

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
		Span<const Handle<Asset>> GetBuiltInMeshAssets();

	private:
		void CreateMeshMetadataFile(Handle<Asset> handle);

	private:
		MeshUtilities();
		UFbxLoader* m_UFbxLoader = nullptr;
		FastGltfLoader* m_FastGltfLoader = nullptr;

		DArray<Handle<Asset>> m_BuiltInMeshAssets = MakeEmptyDArray<Handle<Asset>>();
		HMap<BuiltInMesh, Handle<Mesh>> m_LoadedBuiltInMeshes = MakeEmptyHMap<BuiltInMesh, Handle<Mesh>>();

		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		static MeshUtilities* s_Instance;
	};
}