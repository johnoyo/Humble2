#include "MeshUtilities.h"

namespace HBL2
{
	MeshUtilities* MeshUtilities::s_Instance = nullptr;
	
	MeshUtilities& MeshUtilities::Get()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "MeshUtilities::s_Instance is null! Call MeshUtilities::Initialize before use.");
		return *s_Instance;
	}

	void MeshUtilities::Initialize()
	{
		HBL2_CORE_ASSERT(s_Instance == nullptr, "MeshUtilities::s_Instance is not null! MeshUtilities::Initialize has been called twice.");
		s_Instance = new MeshUtilities;
		s_Instance->m_UFbxLoader = new UFbxLoader;
		s_Instance->m_FastGltfLoader = new FastGltfLoader;
	}

	void MeshUtilities::Shutdown()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "MeshUtilities::s_Instance is null!");

		delete s_Instance->m_UFbxLoader;
		s_Instance->m_UFbxLoader = nullptr;

		delete s_Instance->m_FastGltfLoader;
		s_Instance->m_FastGltfLoader = nullptr;

		delete s_Instance;
		s_Instance = nullptr;
	}

	const MeshData* MeshUtilities::Load(const std::filesystem::path& path)
    {
		if (m_LoadedMeshes.find(path.string()) != m_LoadedMeshes.end())
		{
			return &m_LoadedMeshes[path.string()];
		}

		MeshData meshData;

		const std::string& extension = path.filename().extension().string();

		bool result = false;

		if (extension == ".obj" || extension == ".fbx" || extension == ".FBX")
		{
			result = m_UFbxLoader->Load(path, meshData);
		}
		else if (extension == ".gltf" || extension == ".glb")
		{
			result = m_FastGltfLoader->Load(path, meshData);
		}

		if (!result)
		{
			return nullptr;
		}

		m_LoadedMeshes[path.string()] = meshData;

		return &m_LoadedMeshes[path.string()];
    }
}

