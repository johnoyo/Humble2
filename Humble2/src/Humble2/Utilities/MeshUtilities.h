#pragma once

#include "Base.h"
#include "Loaders\UFbxLoader.h"
#include "Loaders\FastGltfLoader.h"

#include <filesystem>

namespace HBL2
{
	struct MeshData
	{
		std::vector<Vertex> VertexBuffer;
		std::vector<uint32_t> IndexBuffer;
	};

	class MeshUtilities
	{
	public:
		MeshUtilities(const MeshUtilities&) = delete;

		static MeshUtilities& Get()
		{
			HBL2_CORE_ASSERT(s_Instance != nullptr, "MeshUtilities::s_Instance is null! Call MeshUtilities::Initialize before use.");
			return *s_Instance;
		}

		static void Initialize();
		static void Shutdown();

		MeshData* Load(const std::filesystem::path& path);

	private:
		MeshUtilities() = default;
		UFbxLoader* m_UFbxLoader = nullptr;
		FastGltfLoader* m_FastGltfLoader = nullptr;

		std::unordered_map<std::string, MeshData> m_LoadedMeshes;

		inline static MeshUtilities* s_Instance = nullptr;
	};
}