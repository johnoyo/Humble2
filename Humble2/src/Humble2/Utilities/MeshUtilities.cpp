#include "MeshUtilities.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Utilities/tiny_obj_loader.h"

namespace HBL2
{
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

	std::vector<Vertex> MeshUtilities::Load(const std::filesystem::path& path)
    {
		std::vector<Vertex> meshData;

		const std::string& extension = path.filename().extension().string();

		bool result = false;

		if (extension == ".obj" || extension == ".fbx")
		{
			result = m_UFbxLoader->Load(path, meshData);
		}
		else if (extension == ".gltf" || extension == ".glb")
		{
			result = m_FastGltfLoader->Load(path, meshData);
		}

		if (!result)
		{
			return {};
		}

		return meshData;
    }
}

