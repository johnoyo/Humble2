#include "Prefab.h"

#include "Project/Project.h"
#include "Prefab/PrefabSerializer.h"

namespace HBL2
{
	Prefab::Prefab(const PrefabDescriptor&& desc)
		: m_UUID(desc.uuid), m_BaseEntityUUID(desc.baseEntityUUID), m_Version(desc.version)
	{

		Handle<Asset> prefabAssetHandle = AssetManager::Instance->GetHandleFromUUID(m_UUID);
		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(prefabAssetHandle);

		// Load the scene refs into this prefab instance.
		PrefabSerializer serializer(this);
		serializer.DeserializeReferences(Project::GetAssetFileSystemPath(prefabAsset->FilePath));
	}

	entt::entity Prefab::Instantiate(Handle<Asset> assetHandle)
	{
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			HBL2_CORE_ERROR("Asset handle provided is invalid, aborting prefab instantiation.");
			return entt::null;
		}

		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		if (prefabAsset->Type != AssetType::Prefab)
		{
			HBL2_CORE_ERROR("Asset handle provided is not a prefab, aborting prefab instantiation.");
			return entt::null;
		}

		// Get the asset handle (It will load the asset if not already loaded).
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);

		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Prefab asset is invalid, aborting prefab instantiation.");
			return entt::null;
		}

		// Deserialize the source prefab into the scene.
		PrefabSerializer prefabSerializer(prefab);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));
		prefabSerializer.SerializeReferences(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		// Retrieve scene.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		return CloneSourcePrefab(prefab, activeScene);
	}

	entt::entity Prefab::Instantiate(Handle<Asset> assetHandle, const glm::vec3& position)
	{
		// Instantiate the prefab normally.
		entt::entity clone = Instantiate(assetHandle);

		// Get the prefab resource from the asset.
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			return entt::null;
		}

		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);
		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			return entt::null;
		}

		// Set the prefab transform.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		if (activeScene == nullptr)
		{
			HBL2_CORE_ERROR("Cannot retrieve active scene, aborting set of prefab transform.");
			return entt::null;
		}

		auto& prefabTransform = activeScene->GetComponent<Component::Transform>(clone);
		prefabTransform.Translation = position;
	}

	void Prefab::Unpack(entt::entity instantiatedPrefabEntity)
	{
		// Retrieve active scene.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
		if (activeScene == nullptr)
		{
			HBL2_CORE_ERROR("Cannot retrieve active scene, aborting prefab unpacking process.");
			return;
		}

		// Get the prefab component in order to retrieve the prefab source asset from it.
		auto* prefabComponent = activeScene->TryGetComponent<HBL2::Component::PrefabInstance>(instantiatedPrefabEntity);

		if (prefabComponent == nullptr)
		{
			HBL2_CORE_ERROR("Aborting prefab save proccess, provided entity is not an instantiated prefab!");
			return;
		}

		Handle<Asset> prefabAssetHandle = AssetManager::Instance->GetHandleFromUUID(prefabComponent->Id);
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(prefabAssetHandle);
		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Error while trying to unpack prefab, cannot retrieve source prefab asset!");
			return;
		}

		// Remove the prefab component from the entity.
		activeScene->RemoveComponent<HBL2::Component::PrefabInstance>(instantiatedPrefabEntity);

		// Check if the prefab has any other references in the active scene.
		bool hasAnyOtherPrefabs = false;

		activeScene->GetRegistry()
			.view<Component::PrefabInstance>()
			.each([&](entt::entity entity, Component::PrefabInstance& prefab)
			{
				hasAnyOtherPrefabs = true;
			});

		// If it does not have any references, remove the scene uuid from its references.
		if (!hasAnyOtherPrefabs)
		{
			// Find scene asset uuid to remove it from the scene refs of the prefab.
			const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

				if (asset->Type == AssetType::Scene)
				{
					Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);
					Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

					if (activeScene == scene)
					{
						uint32_t indexToRemove = UINT32_MAX;

						for (int i = 0; i < prefab->m_SceneRefs.size(); i++)
						{
							if (asset->UUID == prefab->m_SceneRefs[i])
							{
								indexToRemove = i;
								break;
							}
						}

						if (indexToRemove == UINT32_MAX)
						{
							return;
						}

						uint32_t last = prefab->m_SceneRefs.size() - 1;

						// Swap the scene uuid that we want to remove with the last one.
						std::swap(prefab->m_SceneRefs[indexToRemove], prefab->m_SceneRefs[last]);

						// Remove the last element.
						prefab->m_SceneRefs.pop_back();
					}
				}
			}

			// Deserialize the source prefab into the scene.
			Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(prefabAssetHandle);

			PrefabSerializer prefabSerializer(prefab);
			prefabSerializer.SerializeReferences(Project::GetAssetFileSystemPath(prefabAsset->FilePath));
		}
	}

	void Prefab::Save(entt::entity instantiatedPrefabEntity)
	{
		// Retrieve active scene.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
		if (activeScene == nullptr)
		{
			HBL2_CORE_ERROR("Cannot retrieve active scene, aborting prefab unpacking process.");
			return;
		}

		// Get the prefab component in order to retrieve the prefab source asset from it.
		auto* prefabComponent = activeScene->TryGetComponent<HBL2::Component::PrefabInstance>(instantiatedPrefabEntity);

		if (prefabComponent == nullptr)
		{
			HBL2_CORE_ERROR("Aborting prefab save proccess, provided entity is not an instantiated prefab!");
			return;
		}

		Handle<Asset> prefabAssetHandle = AssetManager::Instance->GetHandleFromUUID(prefabComponent->Id);
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(prefabAssetHandle);
		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Error while trying to unpack prefab, cannot retrieve source prefab asset!");
			return;
		}

		// Serialize the source prefab from the provided instantiated prefab entity.
		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(prefabAssetHandle);

		PrefabSerializer prefabSerializer(prefab, instantiatedPrefabEntity);
		prefabSerializer.Serialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		// Update metadata file and base entity UUID.
		prefab->m_BaseEntityUUID = activeScene->GetComponent<Component::ID>(instantiatedPrefabEntity).Identifier;
		Prefab::CreateMetadataFile(prefabAsset, prefab->m_BaseEntityUUID, prefab->m_Version);

		// Update the instantiated prefab entities that exist in the scene.
		std::vector<entt::entity> otherInstantiatedPrefabEntities;
		std::vector<Component::Transform> otherInstantiatedPrefabEntitiesTransform;

		activeScene->GetRegistry()
			.view<Component::PrefabInstance, Component::Transform>()
			.each([&](entt::entity entity, Component::PrefabInstance& prefab, Component::Transform& tr)
			{
				if (prefab.Id == prefabAsset->UUID)
				{
					otherInstantiatedPrefabEntities.push_back(entity);
					otherInstantiatedPrefabEntitiesTransform.push_back(tr);
				}
			});

		HBL2_CORE_ASSERT(otherInstantiatedPrefabEntities.size() == otherInstantiatedPrefabEntitiesTransform.size(), "");

		for (int i = 0; i < otherInstantiatedPrefabEntities.size(); i++)
		{
			const auto entity = otherInstantiatedPrefabEntities[i];
			activeScene->DestroyEntity(entity);
		}

		for (int i = 0; i < otherInstantiatedPrefabEntities.size(); i++)
		{
			const auto entity = otherInstantiatedPrefabEntities[i];
			const auto& transform = otherInstantiatedPrefabEntitiesTransform[i];

			entt::entity clone = Prefab::Instantiate(prefabAssetHandle, activeScene);

			if (clone != entt::null)
			{
				auto& tr = activeScene->GetComponent<Component::Transform>(clone);
				tr.Translation = transform.Translation;
				tr.Rotation = transform.Rotation;
				tr.Scale = transform.Scale;
				tr.Static = transform.Static;

				auto& prefabComponent = activeScene->GetComponent<Component::PrefabInstance>(clone);
				prefabComponent.Version = prefab->m_Version;
			}
		}

	}

	entt::entity Prefab::Instantiate(Handle<Asset> assetHandle, Scene* scene)
	{
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			HBL2_CORE_ERROR("Asset handle provided is invalid, aborting prefab instantiation.");
			return entt::null;
		}

		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		if (prefabAsset->Type != AssetType::Prefab)
		{
			HBL2_CORE_ERROR("Asset handle provided is not a prefab, aborting prefab instantiation.");
			return entt::null;
		}

		// Get the asset handle (It will load the asset if not already loaded).
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);

		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Prefab asset is invalid, aborting prefab instantiation.");
			return entt::null;
		}

		// Deserialize the source prefab into the scene.
		PrefabSerializer prefabSerializer(prefab, scene);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));
		prefabSerializer.SerializeReferences(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		return CloneSourcePrefab(prefab, scene);
	}

	entt::entity Prefab::CloneSourcePrefab(Prefab* prefab, Scene* activeScene)
	{
		// Check scene.
		if (activeScene == nullptr)
		{
			HBL2_CORE_ERROR("Cannot retrieve active scene, aborting prefab cloning process of the instantiation.");
			return entt::null;
		}

		// Retrieve the base entity of the source instantiated prefab.
		UUID baseEntityUUID = prefab->GetBaseEntityUUID();
		entt::entity baseEntity = activeScene->FindEntityByUUID(baseEntityUUID);

		// Duplicate the source instantiated prefab.
		entt::entity clone = activeScene->DuplicateEntity(baseEntity);

		// Destroy the source instantiated entity of the prefab since now we have its clone.
		activeScene->DestroyEntity(baseEntity);

		return clone;
	}

	void Prefab::CreateMetadataFile(Handle<Asset> assetHandle, UUID baseEntityUUID, uint32_t version)
	{
		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		CreateMetadataFile(prefabAsset, baseEntityUUID, version);
	}

	void Prefab::CreateMetadataFile(Asset* prefabAsset, UUID baseEntityUUID, uint32_t version)
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
		out << YAML::Key << "Version" << YAML::Value << version;
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}
}