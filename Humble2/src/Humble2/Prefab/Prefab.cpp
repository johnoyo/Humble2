#include "Prefab.h"

#include "Project/Project.h"
#include "Prefab/PrefabSerializer.h"

namespace HBL2
{
	void Prefab::Instantiate(Handle<Asset> assetHandle)
	{
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			HBL2_CORE_ERROR("Asset handle provided is invalid, aborting prefab instantiation.");
			return;
		}

		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		if (prefabAsset->Type != AssetType::Prefab)
		{
			HBL2_CORE_ERROR("Asset handle provided is not a prefab, aborting prefab instantiation.");
			return;
		}

		// Get the asset handle (It will load the asset if not already loaded).
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);

		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Prefab asset is invalid, aborting prefab instantiation.");
			return;
		}

		PrefabSerializer prefabSerializer(prefab);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));
	}

	void Prefab::Instantiate(Handle<Asset> assetHandle, const glm::vec3& position)
	{
		// Instantiate the prefab normally.
		Instantiate(assetHandle);

		// Get the prefab resource from the asset.
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			return;
		}

		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);
		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			return;
		}

		// Set the prefab transform.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		if (activeScene == nullptr)
		{
			HBL2_CORE_ERROR("Cannot retrieve active scene, aborting set of prefab transform.");
			return;
		}

		UUID baseEntityUUID = prefab->GetBaseEntityUUID();
		entt::entity baseEntity = activeScene->FindEntityByUUID(baseEntityUUID);

		auto& prefabTransform = activeScene->GetComponent<Component::Transform>(baseEntity);
		prefabTransform.Translation = position;
	}

	void Prefab::Instantiate(Handle<Asset> assetHandle, Scene* scene)
	{
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			HBL2_CORE_ERROR("Asset handle provided is invalid, aborting prefab instantiation.");
			return;
		}

		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		if (prefabAsset->Type != AssetType::Prefab)
		{
			HBL2_CORE_ERROR("Asset handle provided is not a prefab, aborting prefab instantiation.");
			return;
		}

		// Get the asset handle (It will load the asset if not already loaded).
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);

		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Prefab asset is invalid, aborting prefab instantiation.");
			return;
		}

		PrefabSerializer prefabSerializer(prefab, scene);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));
	}

	void Prefab::CreateMetadataFile(Handle<Asset> assetHandle, UUID baseEntityUUID)
	{
		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		CreateMetadataFile(prefabAsset, baseEntityUUID);
	}

	void Prefab::CreateMetadataFile(Asset* prefabAsset, UUID baseEntityUUID)
	{
		if (prefabAsset == nullptr)
		{
			return;
		}

		const auto& relativePath = prefabAsset->FilePath;

		std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(relativePath).string() + ".hblprefab", 0);
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << prefabAsset->UUID;
		out << YAML::Key << "BaseEntityUUID" << YAML::Value << baseEntityUUID;
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}
}