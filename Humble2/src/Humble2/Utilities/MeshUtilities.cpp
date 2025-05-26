#include "MeshUtilities.h"

#include <yaml-cpp\yaml.h>
#include <Project\Project.h>

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

		s_Instance->m_LoadedMeshes.clear();

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

	void MeshUtilities::ClearCachedHandles()
	{
		m_LoadedMeshes.clear();
	}

	void MeshUtilities::LoadBuiltInMeshes()
	{
		// Plane
		auto planeAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "plane-mesh-asset",
			.filePath = "assets/meshes/plane.obj",
			.type = AssetType::Mesh,
		});

		CreateMeshMetadataFile(planeAssetHandle);

		AssetManager::Instance->GetAsset<Mesh>(planeAssetHandle);

		// Tessellated Plane
		auto tessellatedPlaneAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "plane-mesh-asset",
			.filePath = "assets/meshes/tessellated_plane.obj",
			.type = AssetType::Mesh,
		});

		CreateMeshMetadataFile(tessellatedPlaneAssetHandle);

		AssetManager::Instance->GetAsset<Mesh>(tessellatedPlaneAssetHandle);

		// Cube
		auto cubeAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "cube-mesh-asset",
			.filePath = "assets/meshes/cube.obj",
			.type = AssetType::Mesh,
		});

		CreateMeshMetadataFile(cubeAssetHandle);

		AssetManager::Instance->GetAsset<Mesh>(cubeAssetHandle);

		// Sphere
		auto sphereAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "sphere-mesh-asset",
			.filePath = "assets/meshes/sphere.obj",
			.type = AssetType::Mesh,
		});

		CreateMeshMetadataFile(sphereAssetHandle);

		AssetManager::Instance->GetAsset<Mesh>(sphereAssetHandle);

		// Cylinder
		auto cylinderAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "cylinder-mesh-asset",
			.filePath = "assets/meshes/cylinder.obj",
			.type = AssetType::Mesh,
		});

		CreateMeshMetadataFile(cylinderAssetHandle);

		AssetManager::Instance->GetAsset<Mesh>(cylinderAssetHandle);

		// Capsule
		auto capsuleAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "capsule-mesh-asset",
			.filePath = "assets/meshes/capsule.obj",
			.type = AssetType::Mesh,
		});

		CreateMeshMetadataFile(capsuleAssetHandle);

		AssetManager::Instance->GetAsset<Mesh>(capsuleAssetHandle);

		// Torus
		auto torusAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "torus-mesh-asset",
			.filePath = "assets/meshes/torus.obj",
			.type = AssetType::Mesh,
		});

		CreateMeshMetadataFile(torusAssetHandle);

		AssetManager::Instance->GetAsset<Mesh>(torusAssetHandle);
	}

	void MeshUtilities::DeleteBuiltInMeshes()
	{
	}

	void MeshUtilities::CreateMeshMetadataFile(Handle<Asset> handle)
	{
		Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

		if (asset == nullptr)
		{
			HBL2_CORE_ERROR("Error creating asset metadata file for mesh, invalid asset handle passed.");
			return;
		}

		const auto& assetFilePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& path = std::filesystem::exists(assetFilePath) ? assetFilePath : asset->FilePath;

		if (std::filesystem::exists(path.string() + ".hblmesh"))
		{
			return;
		}

		std::ofstream fout(path.string() + ".hblmesh", 0);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}
}

