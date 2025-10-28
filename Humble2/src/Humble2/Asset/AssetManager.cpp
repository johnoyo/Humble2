#include "AssetManager.h"

#include "Project\Project.h"
#include "Utilities/ShaderUtilities.h"
#include "Utilities/MeshUtilities.h"

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

		if (extension == ".png" || extension == ".jpg" || extension == ".tga" || extension == ".hdr")
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
		else if (extension == ".mp3" || extension == ".wav" || extension == ".ogg")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "sound-asset",
				.filePath = relativePath,
				.type = AssetType::Sound,
			});
		}
		else if (extension == ".mat")
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
		else if (extension == ".h")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "script-asset",
				.filePath = relativePath,
				.type = AssetType::Script,
			});
		}
		else if (extension == ".prefab")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "prefab-asset",
				.filePath = relativePath,
				.type = AssetType::Prefab,
			});
		}

		return assetHandle;
	}

	void AssetManager::DeregisterAssets()
	{
		WaitForAsyncJobs();
		
		const auto& builtInShaderAssets = ShaderUtilities::Get().GetBuiltInShaderAssets();

		for (const auto handle : m_RegisteredAssets)
		{
			// Skip if is a built in material or shader asset.
			bool isBuiltInAsset = false;

			for (const auto shaderAssetHandle : builtInShaderAssets)
			{
				if (handle == shaderAssetHandle) { isBuiltInAsset = true; break; }
			}

			if (handle == ShaderUtilities::Get().LitMaterialAsset) { isBuiltInAsset = true; }

			if (isBuiltInAsset)
			{
				continue;
			}

			// Delete assets.
			DeleteAsset(handle);
		}

		// Clear asset handle caches.
		m_RegisteredAssets.Clear();
		m_RegisteredAssetMap.Clear();

		// Reregister built in shader assets.
		for (const auto shaderAssetHandle : builtInShaderAssets)
		{
			m_RegisteredAssets.Add(shaderAssetHandle);
			Asset* asset = GetAssetMetadata(shaderAssetHandle);
			m_RegisteredAssetMap[asset->UUID] = shaderAssetHandle;
		}

		// Reregister built in material asset.
		if (ShaderUtilities::Get().LitMaterialAsset.IsValid())
		{
			m_RegisteredAssets.Add(ShaderUtilities::Get().LitMaterialAsset);
			Asset* asset = GetAssetMetadata(ShaderUtilities::Get().LitMaterialAsset);
			m_RegisteredAssetMap[asset->UUID] = ShaderUtilities::Get().LitMaterialAsset;
		}
	}
}
