#include "MeshUtilities.h"

#include "Project/Project.h"
#include "Asset/EditorAssetManager.h"

#include <yaml-cpp/yaml.h>

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

		s_Instance->m_BuiltInMeshAssets.clear();
		s_Instance->m_LoadedBuiltInMeshes.clear();

		delete s_Instance->m_UFbxLoader;
		s_Instance->m_UFbxLoader = nullptr;

		delete s_Instance->m_FastGltfLoader;
		s_Instance->m_FastGltfLoader = nullptr;

		delete s_Instance;
		s_Instance = nullptr;
	}

	MeshUtilities::MeshUtilities()
	{
		m_Reservation = Allocator::Arena.Reserve("MeshUtilitiesPool", 128_KB);
		m_Arena.Initialize(&Allocator::Arena, 128_KB, m_Reservation);

		m_BuiltInMeshAssets = MakeDArray<Handle<Asset>>(m_Arena, 1024);
		m_LoadedBuiltInMeshAssets = MakeHMap<BuiltInMesh, Handle<Asset>>(m_Arena, 1024);
		m_LoadedBuiltInMeshes = MakeHMap<BuiltInMesh, Handle<Mesh>>(m_Arena, 1024);
	}

	Handle<Mesh> MeshUtilities::Load(const std::filesystem::path& path)
    {
		const std::string& extension = path.filename().extension().string();

		if (extension == ".obj" || extension == ".fbx" || extension == ".FBX")
		{
			return m_UFbxLoader->Load(path);
		}
		else if (extension == ".gltf" || extension == ".glb")
		{
			return m_FastGltfLoader->Load(path);
		}

		return Handle<Mesh>();
    }

	void MeshUtilities::Reload(Asset* asset)
	{
		const std::string& extension = asset->FilePath.extension().string();

		if (extension == ".obj" || extension == ".fbx" || extension == ".FBX")
		{
			m_UFbxLoader->Reload(asset);
		}
		else if (extension == ".gltf" || extension == ".glb")
		{
			m_FastGltfLoader->Reload(asset);
		}
	}

	void MeshUtilities::LoadBuiltInMeshes()
	{
		JobContext ctx;
		auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

		// Plane
		auto planeAssetHandle = editorAssetManager->CreateAsset({
			.debugName = "plane-mesh-asset",
			.filePath = "assets/meshes/plane.obj",
			.type = AssetType::Mesh,
		});
		CreateMeshMetadataFile(planeAssetHandle);
		m_BuiltInMeshAssets.push_back(planeAssetHandle);
		m_LoadedBuiltInMeshAssets[BuiltInMesh::PLANE] = planeAssetHandle;
		auto* planeTask = AssetManager::Instance->GetAssetAsync<Mesh>(planeAssetHandle, &ctx);

		// Tessellated Plane
		auto tessellatedPlaneAssetHandle = editorAssetManager->CreateAsset({
			.debugName = "plane-mesh-asset",
			.filePath = "assets/meshes/tessellated_plane.obj",
			.type = AssetType::Mesh,
		});
		CreateMeshMetadataFile(tessellatedPlaneAssetHandle);
		m_BuiltInMeshAssets.push_back(tessellatedPlaneAssetHandle);
		m_LoadedBuiltInMeshAssets[BuiltInMesh::TESSELATED_PLANE] = tessellatedPlaneAssetHandle;
		auto* tesselatedPlaneTask = AssetManager::Instance->GetAssetAsync<Mesh>(tessellatedPlaneAssetHandle, &ctx);

		// Cube
		auto cubeAssetHandle = editorAssetManager->CreateAsset({
			.debugName = "cube-mesh-asset",
			.filePath = "assets/meshes/cube.obj",
			.type = AssetType::Mesh,
		});
		CreateMeshMetadataFile(cubeAssetHandle);
		m_BuiltInMeshAssets.push_back(cubeAssetHandle);
		m_LoadedBuiltInMeshAssets[BuiltInMesh::CUBE] = cubeAssetHandle;
		auto* cubeTask = AssetManager::Instance->GetAssetAsync<Mesh>(cubeAssetHandle, &ctx);

		// Sphere
		auto sphereAssetHandle = editorAssetManager->CreateAsset({
			.debugName = "sphere-mesh-asset",
			.filePath = "assets/meshes/sphere.obj",
			.type = AssetType::Mesh,
		});
		CreateMeshMetadataFile(sphereAssetHandle);
		m_BuiltInMeshAssets.push_back(sphereAssetHandle);
		m_LoadedBuiltInMeshAssets[BuiltInMesh::SPHERE] = sphereAssetHandle;
		auto* sphereTask = AssetManager::Instance->GetAssetAsync<Mesh>(sphereAssetHandle, &ctx);

		// Cylinder
		auto cylinderAssetHandle = editorAssetManager->CreateAsset({
			.debugName = "cylinder-mesh-asset",
			.filePath = "assets/meshes/cylinder.obj",
			.type = AssetType::Mesh,
		});
		CreateMeshMetadataFile(cylinderAssetHandle);
		m_BuiltInMeshAssets.push_back(cylinderAssetHandle);
		m_LoadedBuiltInMeshAssets[BuiltInMesh::CYLINDER] = cylinderAssetHandle;
		auto* cylinderTask = AssetManager::Instance->GetAssetAsync<Mesh>(cylinderAssetHandle, &ctx);

		// Capsule
		auto capsuleAssetHandle = editorAssetManager->CreateAsset({
			.debugName = "capsule-mesh-asset",
			.filePath = "assets/meshes/capsule.obj",
			.type = AssetType::Mesh,
		});
		CreateMeshMetadataFile(capsuleAssetHandle);
		m_BuiltInMeshAssets.push_back(capsuleAssetHandle);
		m_LoadedBuiltInMeshAssets[BuiltInMesh::CAPSULE] = capsuleAssetHandle;
		auto* capsuleTask = AssetManager::Instance->GetAssetAsync<Mesh>(capsuleAssetHandle, &ctx);

		// Torus
		auto torusAssetHandle = editorAssetManager->CreateAsset({
			.debugName = "torus-mesh-asset",
			.filePath = "assets/meshes/torus.obj",
			.type = AssetType::Mesh,
		});
		CreateMeshMetadataFile(torusAssetHandle);
		m_BuiltInMeshAssets.push_back(torusAssetHandle);
		m_LoadedBuiltInMeshAssets[BuiltInMesh::TORUS] = torusAssetHandle;
		auto* torusTask = AssetManager::Instance->GetAssetAsync<Mesh>(torusAssetHandle, &ctx);

		AssetManager::Instance->WaitForAsyncJobs(&ctx);

		m_LoadedBuiltInMeshes[BuiltInMesh::PLANE] = planeTask ? planeTask->ResourceHandle : Handle<Mesh>();
		m_LoadedBuiltInMeshes[BuiltInMesh::TESSELATED_PLANE] = tesselatedPlaneTask ? tesselatedPlaneTask->ResourceHandle : Handle<Mesh>();
		m_LoadedBuiltInMeshes[BuiltInMesh::CUBE] = cubeTask ? cubeTask->ResourceHandle : Handle<Mesh>();
		m_LoadedBuiltInMeshes[BuiltInMesh::SPHERE] = sphereTask ? sphereTask->ResourceHandle : Handle<Mesh>();
		m_LoadedBuiltInMeshes[BuiltInMesh::CYLINDER] = cylinderTask ? cylinderTask->ResourceHandle : Handle<Mesh>();
		m_LoadedBuiltInMeshes[BuiltInMesh::CAPSULE] = capsuleTask ? capsuleTask->ResourceHandle : Handle<Mesh>();
		m_LoadedBuiltInMeshes[BuiltInMesh::TORUS] = torusTask ? torusTask->ResourceHandle : Handle<Mesh>();

		AssetManager::Instance->ReleaseResourceTask(planeTask);
		AssetManager::Instance->ReleaseResourceTask(tesselatedPlaneTask);
		AssetManager::Instance->ReleaseResourceTask(sphereTask);
		AssetManager::Instance->ReleaseResourceTask(cylinderTask);
		AssetManager::Instance->ReleaseResourceTask(capsuleTask);
		AssetManager::Instance->ReleaseResourceTask(torusTask);
	}

	void MeshUtilities::DeleteBuiltInMeshes()
	{
		for (auto& [meshType, meshHandle] : m_LoadedBuiltInMeshes)
		{
			Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);

			if (mesh != nullptr)
			{
				for (auto& meshPart : mesh->Meshes)
				{
					ResourceManager::Instance->DeleteBuffer(meshPart.IndexBuffer);

					for (const auto vertexBuffer : meshPart.VertexBuffers)
					{
						ResourceManager::Instance->DeleteBuffer(vertexBuffer);
					}
				}
			}

			ResourceManager::Instance->DeleteMesh(meshHandle);
		}

		m_BuiltInMeshAssets.clear();
		m_LoadedBuiltInMeshAssets.clear();
		m_LoadedBuiltInMeshes.clear();
	}

	Handle<Mesh> MeshUtilities::GetBuiltInLoadedMeshHandle(BuiltInMesh builtInMesh)
	{
		auto it = m_LoadedBuiltInMeshes.find(builtInMesh);
		if (it == m_LoadedBuiltInMeshes.end())
		{
			return {};
		}

		return it->second;
	}

	Handle<Asset> MeshUtilities::GetBuiltInLoadedMeshAssetHandle(BuiltInMesh builtInMesh)
	{
		auto it = m_LoadedBuiltInMeshAssets.find(builtInMesh);
		if (it == m_LoadedBuiltInMeshAssets.end())
		{
			return {};
		}

		return it->second;
	}

	Span<const Handle<Asset>> MeshUtilities::GetBuiltInMeshAssets()
	{
		return { m_BuiltInMeshAssets.data(), m_BuiltInMeshAssets.size() };
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
        const auto& workingDirectory = Project::GetAssetDirectory().parent_path().parent_path();
		const auto& path = std::filesystem::exists(assetFilePath) ? assetFilePath : workingDirectory / asset->FilePath;

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

