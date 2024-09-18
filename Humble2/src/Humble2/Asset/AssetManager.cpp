#include "AssetManager.h"

#include "Project\Project.h"

namespace HBL2
{
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
			const std::string& extension = entry.path().extension().string();
			auto relativePath = std::filesystem::relative(entry.path(), HBL2::Project::GetAssetDirectory());

			if (extension == ".png" || extension == ".jpg")
			{
				auto assetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "texture-asset",
					.filePath = relativePath,
					.type = AssetType::Texture,
				});
			}
			else if (extension == ".obj" || extension == ".gltf" || extension == ".glb" || extension == ".fbx")
			{
				auto assetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "mesh-asset",
					.filePath = relativePath,
					.type = AssetType::Mesh,
				});
			}
			else if (extension == ".hblmat")
			{
				auto assetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "material-asset",
					.filePath = relativePath,
					.type = AssetType::Material,
				});
			}
			else if (extension == ".hblshader")
			{
				auto assetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "shader-asset",
					.filePath = relativePath,
					.type = AssetType::Shader,
				});
			}
			else if (extension == ".humble")
			{
				auto assetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "scene-asset",
					.filePath = relativePath,
					.type = AssetType::Scene,
				});
			}
		}
	}

	void AssetManager::DeregisterAssets()
	{
		for (auto handle : m_RegisteredAssets)
		{
			AssetManager::Instance->DeleteAsset(handle);
		}

		m_RegisteredAssets.clear();
	}
}
