#include "Prefab.h"

#include "Project/Project.h"
#include "Prefab/PrefabSerializer.h"
#include "Utilities/PrefabUtilities.h"

namespace HBL2
{
	Prefab::Prefab(const PrefabDescriptor&& desc)
	{
		Load(std::forward<const PrefabDescriptor>(desc));
	}

	void Prefab::Load(const PrefabDescriptor&& desc)
	{
		if (m_Loaded)
		{
			return;
		}

		m_UUID = desc.uuid;
		m_Version = desc.version;
		m_BaseEntityUUID = desc.baseEntityUUID;

		m_SubSceneHandle = ResourceManager::Instance->CreateScene({ .name = desc.debugName });

		m_Loaded = true;
	}

	void Prefab::Unload()
	{
		if (!m_Loaded)
		{
			return;
		}

		m_BaseEntityUUID = 0;

		Scene* prefabSubScene = ResourceManager::Instance->GetScene(m_SubSceneHandle);

		if (prefabSubScene)
		{
			prefabSubScene->Clear();
		}

		ResourceManager::Instance->DeleteScene(m_SubSceneHandle);

		m_Loaded = false;
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

		return Instantiate(prefabHandle);
	}

	Entity Prefab::Instantiate(Handle<Asset> assetHandle, const glm::vec3& position)
	{
		// Instantiate the prefab normally.
		Entity clone = Instantiate(assetHandle);

		if (clone == Entity::Null)
		{
			return Entity::Null;
		}

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

	Entity Prefab::Instantiate(Handle<Prefab> prefabHandle)
	{
		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab == nullptr)
		{
			HBL2_CORE_ERROR("Prefab asset is invalid, aborting prefab instantiation.");
			return Entity::Null;
		}

		// Retrieve scene.
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		// Instantiate prefab entity into the active scene by cloning the source prefab entity.
		Entity instantiatedPrefabEntity = CloneSourcePrefab(prefab, activeScene);

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

			PrefabUtilities::Get().Update(prefabAsset, prefab, activeScene, entity, true, true);
		}

		return instantiatedPrefabEntity;
	}

	Entity Prefab::Instantiate(Handle<Prefab> prefabHandle, const glm::vec3& position)
	{
		// Instantiate the prefab normally.
		Entity clone = Instantiate(prefabHandle);

		if (clone == Entity::Null)
		{
			return Entity::Null;
		}

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
		Scene* prefabSubScene = ResourceManager::Instance->GetScene(prefab->m_SubSceneHandle);
		Entity baseEntity = prefabSubScene->FindEntityByUUID(baseEntityUUID);

		// Duplicate the source instantiated prefab.
		Entity clone = activeScene->DuplicateEntityFromScene(baseEntity, prefabSubScene);

		return clone;
	}
}