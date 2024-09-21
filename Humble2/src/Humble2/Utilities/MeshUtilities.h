#pragma once

#include "Base.h"
#include "UFbxLoader.h"
#include "FastGltfLoader.h"

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

		MeshData Load(const std::filesystem::path& path);

	private:
		MeshUtilities() = default;
		UFbxLoader* m_UFbxLoader = nullptr;
		FastGltfLoader* m_FastGltfLoader = nullptr;

		inline static MeshUtilities* s_Instance = nullptr;
	};
}