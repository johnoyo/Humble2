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

	Handle<Mesh> MeshUtilities::Load(const std::filesystem::path& path)
    {
		if (m_LoadedMeshes.find(path.string()) != m_LoadedMeshes.end())
		{
			return m_LoadedMeshes[path.string()];
		}

		const std::string& extension = path.filename().extension().string();

		Handle<Mesh> handle;

		if (extension == ".obj" || extension == ".fbx" || extension == ".FBX")
		{
			handle = m_UFbxLoader->Load(path);
		}
		else if (extension == ".gltf" || extension == ".glb")
		{
			handle = m_FastGltfLoader->Load(path);
		}

		if (!handle.IsValid())
		{
			return Handle<Mesh>();
		}

		m_LoadedMeshes[path.string()] = handle;

		return m_LoadedMeshes[path.string()];
    }
}

