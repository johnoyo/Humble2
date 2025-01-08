#include "AssetManager.h"

#include "Project\Project.h"

namespace HBL2
{
	AssetManager* AssetManager::Instance = nullptr;

	void AssetManager::RegisterAssets()
	{
		// Create Asset directory if it does not exist.
		if (!std::filesystem::is_directory(HBL2::Project::GetAssetDirectory()))
		{
			try
			{
				std::filesystem::create_directories(HBL2::Project::GetAssetDirectory());
			}
			catch (std::exception& e)
			{
				HBL2_CORE_ERROR("Project directory creation failed: {0}", e.what());
				return;
			}
		}

		// Register assets that are inside the Assets directory.
		for (auto& entry : std::filesystem::recursive_directory_iterator(HBL2::Project::GetAssetDirectory()))
		{
			RegisterAsset(entry.path());
		}
	}

	Handle<Asset> AssetManager::RegisterAsset(const std::filesystem::path& assetPath)
	{
		const std::string& extension = assetPath.extension().string();
		auto relativePath = std::filesystem::relative(assetPath, HBL2::Project::GetAssetDirectory());

		Handle<Asset> assetHandle;

		if (extension == ".png" || extension == ".jpg")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "texture-asset",
				.filePath = relativePath,
				.type = AssetType::Texture,
			});
		}
		else if (extension == ".obj" || extension == ".gltf" || extension == ".glb" || extension == ".fbx" || extension == ".FBX")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "mesh-asset",
				.filePath = relativePath,
				.type = AssetType::Mesh,
			});
		}
		else if (extension == ".hblmat")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "material-asset",
				.filePath = relativePath,
				.type = AssetType::Material,
			});
		}
		else if (extension == ".shader")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "shader-asset",
				.filePath = relativePath,
				.type = AssetType::Shader,
			});
		}
		else if (extension == ".humble")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "scene-asset",
				.filePath = relativePath,
				.type = AssetType::Scene,
			});
		}

		return assetHandle;
	}

	void AssetManager::DeregisterAssets()
	{
		for (auto handle : m_RegisteredAssets)
		{
			AssetManager::Instance->DeleteAsset(handle);
		}

		m_RegisteredAssets.clear();
		m_RegisteredAssetMap.clear();
	}
}
