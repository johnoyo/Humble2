#pragma once

#include "Base.h"
#include "Resources\Handle.h"
#include "Resources\Types.h"
#include "Loaders\UFbxLoader.h"
#include "Loaders\FastGltfLoader.h"

#include <filesystem>

namespace HBL2
{
	class HBL2_API MeshUtilities
	{
	public:
		MeshUtilities(const MeshUtilities&) = delete;

		static MeshUtilities& Get();

		static void Initialize();
		static void Shutdown();

		Handle<Mesh> Load(const std::filesystem::path& path);
		void ClearCachedHandles();

		Handle<Mesh> GetLoadedMeshHandle(const std::string& path)
		{
			if (m_LoadedMeshes.find(path) == m_LoadedMeshes.end())
			{
				return {};
			}

			return m_LoadedMeshes[path];
		}

		void LoadBuiltInMeshes();
		void DeleteBuiltInMeshes();

	private:
		void CreateMeshMetadataFile(Handle<Asset> handle);

	private:
		MeshUtilities() = default;
		UFbxLoader* m_UFbxLoader = nullptr;
		FastGltfLoader* m_FastGltfLoader = nullptr;

		std::unordered_map<std::string, Handle<Mesh>> m_LoadedMeshes;

		static MeshUtilities* s_Instance;
	};
}