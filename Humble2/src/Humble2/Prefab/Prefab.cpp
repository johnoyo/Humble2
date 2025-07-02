#include "Prefab.h"

#include "Project/Project.h"
#include "Prefab/PrefabSerializer.h"

namespace HBL2
{
	Prefab::Prefab(const PrefabDescriptor&& desc)
		: m_UUID(desc.uuid), m_BaseEntityUUID(desc.baseEntityUUID), m_Version(desc.version)
	{
	}

	Entity Prefab::Instantiate(Handle<Asset> assetHandle)
	{
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			HBL2_CORE_ERROR("Asset handle provided is invalid, aborting prefab instantiation.");
			return Entity::Null;
		}

		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		if (prefabAsset->Type != AssetType::Prefab)
		{
			HBL2_CORE_ERROR("Asset handle provided is not a prefab, aborting prefab instantiation.");
			return Entity::Null;
		}

		// Get the asset handle (It will load the asset if not already loaded).
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);

		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Prefab asset is invalid, aborting prefab instantiation.");
			return Entity::Null;
		}

		// Deserialize the source prefab into the scene.
		PrefabSerializer prefabSerializer(prefab);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		// Retrieve scene.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		return CloneSourcePrefab(prefab, activeScene);
	}

	Entity Prefab::Instantiate(Handle<Asset> assetHandle, const glm::vec3& position)
	{
		// Instantiate the prefab normally.
		Entity clone = Instantiate(assetHandle);

		// Get the prefab resource from the asset.
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			return Entity::Null;
		}

		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);
		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			return Entity::Null;
		}

		// Set the prefab transform.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		if (activeScene == nullptr)
		{
			HBL2_CORE_ERROR("Cannot retrieve active scene, aborting set of prefab transform.");
			return Entity::Null;
		}

		auto& prefabTransform = activeScene->GetComponent<Component::Transform>(clone);
		prefabTransform.Translation = position;
	}

	void Prefab::Unpack(Entity instantiatedPrefabEntity)
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
	}

	void Prefab::Save(Entity instantiatedPrefabEntity)
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
		std::vector<Entity> otherInstantiatedPrefabEntities;
		std::vector<Component::Transform> otherInstantiatedPrefabEntitiesTransform;

		// NOTE: In the future we may need to update the children list of the prefab if it has a parent.
		//		 Since we are deleting it and re-instantiating it the child uuid that the parent holds will be invalid.

		activeScene->View<Component::PrefabInstance, Component::Transform>()
			.Each([&](Entity entity, Component::PrefabInstance& prefab, Component::Transform& tr)
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

			Entity clone = Prefab::Instantiate(prefabAssetHandle, activeScene);

			if (clone != Entity::Null)
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

	void Prefab::Destroy(Entity instantiatedPrefabEntity)
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

		activeScene->DestroyEntity(instantiatedPrefabEntity);
	}

	Entity Prefab::Instantiate(Handle<Asset> assetHandle, Scene* scene)
	{
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			HBL2_CORE_ERROR("Asset handle provided is invalid, aborting prefab instantiation.");
			return Entity::Null;
		}

		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		if (prefabAsset->Type != AssetType::Prefab)
		{
			HBL2_CORE_ERROR("Asset handle provided is not a prefab, aborting prefab instantiation.");
			return Entity::Null;
		}

		// Get the asset handle (It will load the asset if not already loaded).
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);

		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Prefab asset is invalid, aborting prefab instantiation.");
			return Entity::Null;
		}

		// Deserialize the source prefab into the scene.
		PrefabSerializer prefabSerializer(prefab, scene);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		return CloneSourcePrefab(prefab, scene);
	}

	Entity Prefab::CloneSourcePrefab(Prefab* prefab, Scene* activeScene)
	{
		// Check scene.
		if (activeScene == nullptr)
		{
			HBL2_CORE_ERROR("Cannot retrieve active scene, aborting prefab cloning process of the instantiation.");
			return Entity::Null;
		}

		// Retrieve the base entity of the source instantiated prefab.
		UUID baseEntityUUID = prefab->GetBaseEntityUUID();
		Entity baseEntity = activeScene->FindEntityByUUID(baseEntityUUID);

		// Duplicate the source instantiated prefab.
		Entity clone = activeScene->DuplicateEntity(baseEntity);

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