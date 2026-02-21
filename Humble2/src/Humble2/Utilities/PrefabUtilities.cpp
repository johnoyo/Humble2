#include "PrefabUtilities.h"

#include "Core/Context.h"
#include "Project/Project.h"
#include "Prefab/PrefabSerializer.h"
#include "Resources/ResourceManager.h"

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

    PrefabUtilities* PrefabUtilities::s_Instance = nullptr;

    PrefabUtilities& PrefabUtilities::Get()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "PrefabUtilities::s_Instance is null! Call PrefabUtilities::Initialize before use.");
        return *s_Instance;
    }

    void PrefabUtilities::Initialize()
    {
        HBL2_CORE_ASSERT(s_Instance == nullptr, "PrefabUtilities::s_Instance is not null! PrefabUtilities::Initialize has been called twice.");
        s_Instance = new PrefabUtilities;
    }

    void PrefabUtilities::Shutdown()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "PrefabUtilities::s_Instance is null!");

        delete s_Instance;
        s_Instance = nullptr;
    }

    void PrefabUtilities::Unpack(Entity instantiatedPrefabEntity)
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
		auto unpack = [&](auto&& self, Entity e)
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
					self(self, child);
				}
			}
		};

		unpack(unpack, instantiatedPrefabEntity);
    }
    
	void PrefabUtilities::Save(Entity instantiatedPrefabEntity)
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
		auto applyPrefabEntity = [&](auto&& self, Entity e)
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
					self(self, child);
				}
			}
		};
		applyPrefabEntity(applyPrefabEntity, instantiatedPrefabEntity);

		// Serialize prefab to disk.
		PrefabSerializer prefabSerializer(prefab, instantiatedPrefabEntity);
		prefabSerializer.Serialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		// Clear prefab contents.
		prefab->Unload();

		// Update metadata file, base entity UUID and sub scene.
		UUID newBaseEntityUUID = activeScene->GetComponent<Component::ID>(instantiatedPrefabEntity).Identifier;
		prefab->Load({
			.debugName = "PrefabScene",
			.uuid = prefab->GetUUID(),
			.baseEntityUUID = newBaseEntityUUID,
			.version = prefab->GetVersion(),
		});
		CreateMetadataFile(prefabAsset, prefab->GetBaseEntityUUID(), prefab->GetVersion());

		// Reload prefab contents.
		PrefabSerializer prefabDeserializer(prefab);
		prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(prefabAsset->FilePath));

		Update(prefabAsset, prefab, activeScene, instantiatedPrefabEntity, false, false);
    }

    void PrefabUtilities::Revert(Entity instantiatedPrefabEntity)
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

		// Retrieve the base entity of the source instantiated prefab.
		UUID baseEntityUUID = prefab->GetBaseEntityUUID();
		Scene* prefabSubScene = ResourceManager::Instance->GetScene(prefab->GetSubSceneHandle());
		Entity baseEntity = prefabSubScene->FindEntityByUUID(baseEntityUUID);

		Entity clone = activeScene->DuplicateEntityWhilePreservingUUIDsFromEntityAndDestroy(baseEntity, prefabSubScene, instantiatedPrefabEntity);

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
			prefabComponent.Version = prefab->GetVersion();
		}
    }

	void PrefabUtilities::Update(Asset* prefabAsset, Prefab* prefab, Scene* activeScene, Entity instantiatedPrefabEntity, bool checkVersion, bool preserveName)
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

		// Retrieve the base entity of the source instantiated prefab.
		UUID baseEntityUUID = prefab->GetBaseEntityUUID();
		Scene* prefabSubScene = ResourceManager::Instance->GetScene(prefab->GetSubSceneHandle());
		Entity baseEntity = prefabSubScene->FindEntityByUUID(baseEntityUUID);

		for (int i = 0; i < instantiatedPrefabEntitiesInfo.size(); i++)
		{
			const auto entity = instantiatedPrefabEntitiesInfo[i].entity;
			const auto& transform = instantiatedPrefabEntitiesInfo[i].transform;

			if (prefab->GetVersion() == instantiatedPrefabEntitiesInfo[i].prefab.Version && checkVersion)
			{
				continue;
			}

			Entity clone = activeScene->DuplicateEntityWhilePreservingUUIDsFromEntityAndDestroy(baseEntity, prefabSubScene, entity);

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
				prefabComponent.Version = prefab->GetVersion();
			}
		}
	}

    void PrefabUtilities::ConvertEntityToPrefabPhase0(Entity entity, Scene* scene)
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

    bool PrefabUtilities::ConvertEntityToPrefabPhase1(Entity entity, Handle<Asset> assetHandle, Scene* scene)
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
		prefabInstance.Id = prefab->GetBaseEntityUUID();
		prefabInstance.Version = prefab->GetVersion();

		return true;
    }

    void PrefabUtilities::CreateMetadataFile(Handle<Asset> assetHandle, UUID baseEntityUUID, uint32_t version)
    {
		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);
		CreateMetadataFile(prefabAsset, baseEntityUUID, version);
    }

    void PrefabUtilities::CreateMetadataFile(Asset* prefabAsset, UUID baseEntityUUID, uint32_t version)
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
