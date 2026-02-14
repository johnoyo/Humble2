#include "Prefab.h"

#include "Project/Project.h"
#include "Prefab/PrefabSerializer.h"

namespace HBL2
{
	struct PrefabInfo
	{
		Entity entity;
		Component::Tag tag;
		Component::Transform transform;
		Component::PrefabInstance prefab;
		UUID parent;
	};

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

		Entity instantiatedPrefabEntity = CloneSourcePrefab(prefab, activeScene);

		// Retrieve the base entity of the source instantiated prefab.
		UUID baseEntityUUID = prefab->GetBaseEntityUUID();
		Entity baseEntity = activeScene->FindEntityByUUID(baseEntityUUID);

		// Gather all prefab entities and their prefab uuid that are nested in this prefab.
		std::function<void(Entity, std::unordered_map<UUID, Entity>&)> collect = [&](Entity e, std::unordered_map<UUID, Entity>& nestedPrefabs)
		{
			if (e == Entity::Null)
			{
				return;
			}

			if (auto* pi = activeScene->TryGetComponent<Component::PrefabInstance>(e))
			{
				if (prefab->m_UUID != pi->Id)
				{
					nestedPrefabs[pi->Id] = e;
				}
			}

			auto* link = activeScene->TryGetComponent<Component::Link>(e);
			if (!link)
			{
				return;
			}

			for (UUID childUUID : link->Children)
			{
				Entity child = activeScene->FindEntityByUUID(childUUID);
				if (child != Entity::Null)
				{
					collect(child, nestedPrefabs);
				}
			}
		};

		std::unordered_map<UUID, Entity> handledPrefabInstances;
		handledPrefabInstances.reserve(16);
		collect(instantiatedPrefabEntity, handledPrefabInstances);

		for (const auto& [prefabUUID, entity] : handledPrefabInstances)
		{
			Handle<Asset> prefabAssetHandle = AssetManager::Instance->GetHandleFromUUID(prefabUUID);
			Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(prefabAssetHandle);
			Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(prefabAssetHandle);
			Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

			Prefab::Update(prefabAsset, prefab, activeScene, entity, true, true);
		}

		return instantiatedPrefabEntity;
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

		// Remove the prefab instance component from the base entity.
		activeScene->RemoveComponent<Component::PrefabInstance>(instantiatedPrefabEntity);

		// Before removing the PrefabEntity component check if is a nested prefab.
		// Since then we need the PrefabEntity components if is nested.
		if (auto* link = activeScene->TryGetComponent<Component::Link>(instantiatedPrefabEntity))
		{
			Entity parent = activeScene->FindEntityByUUID(link->Parent);

			if (parent != Entity::Null && activeScene->HasComponent<Component::PrefabEntity>(parent))
			{
				// The parent has a PrefabEntity component, which means he is part of a prefab,
				// which means the instantiatedPrefabEntity was a nested prefab, so return.
				return;
			}
		}

		// Remove the prefab entity component from all the entities in the sub tree.
		std::function<void(Entity)> unpack = [&](Entity e)
		{
			if (e == Entity::Null)
			{
				return;
			}

			activeScene->RemoveComponent<Component::PrefabEntity>(e);

			auto* link = activeScene->TryGetComponent<Component::Link>(e);
			if (!link)
			{
				return;
			}

			for (UUID childUUID : link->Children)
			{
				Entity child = activeScene->FindEntityByUUID(childUUID);
				if (child != Entity::Null)
				{
					unpack(child);
				}
			}
		};

		unpack(instantiatedPrefabEntity);
	}

	void Prefab::Save(Entity instantiatedPrefabEntity)
	{
		// Retrieve active scene.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
		if (activeScene == nullptr)
		{
			HBL2_CORE_ERROR("Cannot retrieve active scene, aborting prefab save process.");
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
			HBL2_CORE_ERROR("Error while trying to save prefab, cannot retrieve source prefab asset!");
			return;
		}

		// Serialize the source prefab from the provided instantiated prefab entity.
		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(prefabAssetHandle);

		// Before duplicating instantiatedPrefabEntity ensure all descendants have PrefabEntity component.
		// This is to cover the case where a new entity was added to the prefab and then we save.
		std::function<void(Entity)> applyPrefabEntity = [&](Entity e)
		{
			if (e == Entity::Null)
			{
				return;
			}

			if (!activeScene->HasComponent<Component::PrefabEntity>(e))
			{
				auto& prefabEntity = activeScene->AddComponent<Component::PrefabEntity>(e);
				prefabEntity.EntityId = Random::UInt64();
			}

			auto* link = activeScene->TryGetComponent<Component::Link>(e);
			if (!link)
			{
				return;
			}

			for (UUID childUUID : link->Children)
			{
				Entity child = activeScene->FindEntityByUUID(childUUID);
				if (child != Entity::Null)
				{
					applyPrefabEntity(child);
				}
			}
		};
		applyPrefabEntity(instantiatedPrefabEntity);

		// Duplicate the entity prefab before serializing to ensure unique UUIDs.
		Entity clone = activeScene->DuplicateEntity(instantiatedPrefabEntity, EntityDuplicationNaming::DONT_APPEND_CLONE);

		// Serialize now that is has unique UUIDs.
		PrefabSerializer prefabSerializer(prefab, clone);
		prefabSerializer.Serialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		// Update metadata file and base entity UUID.
		prefab->m_BaseEntityUUID = activeScene->GetComponent<Component::ID>(clone).Identifier;
		Prefab::CreateMetadataFile(prefabAsset, prefab->m_BaseEntityUUID, prefab->m_Version);

		// Destroy the cloned prefab entity.
		activeScene->DestroyEntity(clone);

		Update(prefabAsset, prefab, activeScene, instantiatedPrefabEntity, false, false);
	}

	void Prefab::Revert(Entity instantiatedPrefabEntity)
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
		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(prefabAssetHandle);

		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(prefabAssetHandle);
		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Error while trying to revert prefab, cannot retrieve source prefab asset!");
			return;
		}

		auto* pi = activeScene->TryGetComponent<Component::PrefabInstance>(instantiatedPrefabEntity);
		if (!pi)
		{
			HBL2_CORE_ERROR("Error while trying to revert prefab, cannot retrieve prefab instance component!");
			return;
		}

		pi->Override = true;

		// Copy them intentionally, since we want to use them after the destruction of their entity.
		Component::Transform transform = activeScene->GetComponent<Component::Transform>(instantiatedPrefabEntity);
		Component::Link link = activeScene->GetComponent<Component::Link>(instantiatedPrefabEntity);

		// Deserialize the source prefab into the scene.
		PrefabSerializer prefabSerializer(prefab, activeScene);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		// Retrieve the base entity of the source instantiated prefab.
		UUID baseEntityUUID = prefab->GetBaseEntityUUID();
		Entity baseEntity = activeScene->FindEntityByUUID(baseEntityUUID);

		Entity clone = activeScene->DuplicateEntityWhilePreservingUUIDsFromEntityAndDestroy(baseEntity, instantiatedPrefabEntity);

		if (clone != Entity::Null)
		{
			// Reset parent of updated prefab, since it was overriden with the saved in the source prefab.
			auto& lk = activeScene->GetComponent<Component::Link>(clone);
			lk.Parent = link.Parent;
			lk.PrevParent = link.Parent;

			auto& tr = activeScene->GetComponent<Component::Transform>(clone);
			tr.Translation = transform.Translation;
			tr.Rotation = transform.Rotation;
			tr.Scale = transform.Scale;
			tr.Static = transform.Static;

			auto& prefabComponent = activeScene->GetComponent<Component::PrefabInstance>(clone);
			prefabComponent.Version = prefab->m_Version;
		}

		// Destroy the source prefab entity.
		activeScene->DestroyEntity(baseEntity);
	}

	void Prefab::Update(Asset* prefabAsset, Prefab* prefab, Scene* activeScene, Entity instantiatedPrefabEntity, bool checkVersion, bool preserveName)
	{
		// Update the instantiated prefab entities that exist in the scene.
		std::vector<PrefabInfo> instantiatedPrefabEntitiesInfo;

		activeScene->View<Component::ID, Component::PrefabInstance, Component::Transform, Component::Tag>()
			.Each([&](Entity entity, Component::ID& id, Component::PrefabInstance& prefab, Component::Transform& tr, Component::Tag& tag)
			{
				if (prefab.Id == prefabAsset->UUID && prefab.Override)
				{
					UUID parent = 0;
					if (auto* link = activeScene->TryGetComponent<Component::Link>(entity))
					{
						parent = link->Parent;
					}

					instantiatedPrefabEntitiesInfo.push_back({ entity, tag, tr, prefab, parent });
				}
			});

		// Deserialize the source prefab into the scene.
		PrefabSerializer prefabSerializer(prefab, activeScene);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		// Retrieve the base entity of the source instantiated prefab.
		UUID baseEntityUUID = prefab->GetBaseEntityUUID();
		Entity baseEntity = activeScene->FindEntityByUUID(baseEntityUUID);

		for (int i = 0; i < instantiatedPrefabEntitiesInfo.size(); i++)
		{
			const auto entity = instantiatedPrefabEntitiesInfo[i].entity;
			const auto& transform = instantiatedPrefabEntitiesInfo[i].transform;

			if (prefab->m_Version == instantiatedPrefabEntitiesInfo[i].prefab.Version && checkVersion)
			{
				continue;
			}

			Entity clone = activeScene->DuplicateEntityWhilePreservingUUIDsFromEntityAndDestroy(baseEntity, entity);

			if (clone != Entity::Null)
			{
				// Reset parent of updated prefab, since it was overriden with the saved in the source prefab.
				auto& link = activeScene->GetComponent<Component::Link>(clone);
				link.Parent = instantiatedPrefabEntitiesInfo[i].parent;
				link.PrevParent = link.Parent;

				auto& tr = activeScene->GetComponent<Component::Transform>(clone);
				tr.Translation = transform.Translation;
				tr.Rotation = transform.Rotation;
				tr.Scale = transform.Scale;
				tr.Static = transform.Static;

				if (preserveName)
				{
					auto& tagComponent = activeScene->GetComponent<Component::Tag>(clone);
					tagComponent.Name = instantiatedPrefabEntitiesInfo[i].tag.Name;
				}

				auto& prefabComponent = activeScene->GetComponent<Component::PrefabInstance>(clone);
				prefabComponent.Version = prefab->m_Version;
			}
		}

		// Destroy the source prefab entity.
		activeScene->DestroyEntity(baseEntity);
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

	void Prefab::ConvertEntityToPrefabPhase0(Entity entity, Scene* scene)
	{
		std::function<void(Entity)> convert = [&](Entity e)
		{
			if (e == Entity::Null)
			{
				return;
			}

			if (!scene->HasComponent<Component::PrefabEntity>(e))
			{
				auto& prefabEntity = scene->AddComponent<Component::PrefabEntity>(e);
				prefabEntity.EntityId = Random::UInt64();
			}

			auto* link = scene->TryGetComponent<Component::Link>(e);
			if (!link)
			{
				return;
			}

			for (UUID childUUID : link->Children)
			{
				Entity child = scene->FindEntityByUUID(childUUID);
				if (child != Entity::Null)
				{
					convert(child);
				}
			}
		};

		convert(entity);
	}

	bool Prefab::ConvertEntityToPrefabPhase1(Entity entity, Handle<Asset> assetHandle, Scene* scene)
	{
		if (!AssetManager::Instance->IsAssetValid(assetHandle))
		{
			HBL2_CORE_ERROR("Asset handle provided is invalid, aborting prefab instantiation.");
			return false;
		}

		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		if (prefabAsset->Type != AssetType::Prefab)
		{
			HBL2_CORE_ERROR("Asset handle provided is not a prefab, aborting prefab instantiation.");
			return false;
		}

		// Get the asset handle (It will load the asset if not already loaded).
		Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(assetHandle);

		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Prefab asset is invalid, aborting prefab instantiation.");
			return false;
		}

		auto& prefabInstance = scene->GetOrAddComponent<Component::PrefabInstance>(entity);
		prefabInstance.Id = prefab->m_UUID;
		prefabInstance.Version = prefab->m_Version;

		return true;
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