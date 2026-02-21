#include "PrefabSerializer.h"

#include "Core\Context.h"
#include "Utilities\Log.h"
#include "Utilities\YamlUtilities.h"
#include "Resources\ResourceManager.h"

#include "Script/BuildEngine.h"
#include "Scene\EntitySerializer.h"

namespace HBL2
{
	PrefabSerializer::PrefabSerializer(Prefab* prefab)
		: m_Context(prefab)
	{
	}

	PrefabSerializer::PrefabSerializer(Prefab* prefab, Entity instantiatedPrefabEntity)
		: m_Context(prefab), m_InstantiatedPrefabEntity(instantiatedPrefabEntity)
	{
	}

	void PrefabSerializer::Serialize(const std::filesystem::path& path)
	{
		/*		
		- Instantiatiation: (Spawn into the scene)
			- Instantiate the source prefab into the scene from the file.
			- Duplicate the instantiated entity so it has unique ID.
			- Delete initial instantiated entity.
		- Update: (the prefab is already instantiated and we want to update it since the source prefab changed.)
			- Check if the version inside the scene matches the prefab source asset version.
			- If they do not match
				- Spawn the prefab source asset entity into the scene.
				- Duplicate the prefab source asset entity to get the updated one, while preserving the UUIDs of the original.
				- Destroy instantiated prefab entity.
				- Delete prefab source entity.
		- Save:
			- Duplicate the instantiated prefab entity.
			- Serialize the source prefab from the provided duplicated instantiated prefab entity.
			- Update metadata file and base entity UUID.
			- Update the instantiated prefab entities that exist in the scene.
				- Destroy them and re-instantiate them while preserving their UUIDs.
		*/

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab" << YAML::BeginSeq;

		out << YAML::BeginMap;
		out << YAML::Key << "Entities" << YAML::BeginSeq;

		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
		Entity baseEntity;

		if (m_InstantiatedPrefabEntity != Entity::Null)
		{
			baseEntity = m_InstantiatedPrefabEntity;
		}
		else
		{
			// NOTE: This branch is only hit when we drag and drop an entity from the hierachy panel
			//		 to the content browser to create a new prefab.
			baseEntity = activeScene->FindEntityByUUID(m_Context->GetBaseEntityUUID());
		}

		// Add the prefab component if the base entity does not have it.
		if (!activeScene->HasComponent<Component::PrefabInstance>(baseEntity))
		{
			auto& prefab = activeScene->AddComponent<Component::PrefabInstance>(baseEntity);
			prefab.Id = m_Context->m_UUID;
			prefab.Version = m_Context->m_Version;
		}

		SerializePrefab(activeScene, baseEntity, out);

		out << YAML::EndSeq;

		// Serialize user component UUIDs.
		std::vector<UUID> componentUUIDs;
		const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

		for (auto handle : assetHandles)
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

			if (asset->Type == AssetType::Script)
			{
				Script* script = ResourceManager::Instance->GetScript(Handle<Script>::UnPack(asset->Indentifier));

				if (script != nullptr)
				{
					if (script->Type == ScriptType::COMPONENT)
					{
						componentUUIDs.push_back(asset->UUID);
					}
				}
			}
		}

		out << YAML::Key << "User Components" << YAML::BeginSeq;
		for (UUID componentUUID : componentUUIDs)
		{
			out << YAML::Key << componentUUID << YAML::Value;
		}
		out << YAML::EndSeq;

		out << YAML::EndMap;

		out << YAML::EndSeq;
		out << YAML::EndMap;

		try
		{
			std::filesystem::create_directories(path.parent_path());
		}
		catch (std::exception& e)
		{
			HBL2_CORE_ERROR("Project directory creation failed: {0}", e.what());
		}

		std::ofstream fOut(path);

		if (!fOut.is_open())
		{
			HBL2_CORE_ERROR("File not found: {0}", path.string());
			return;
		}

		fOut << out.c_str();
		fOut.close();

		m_Context->m_Version++;
	}

	bool PrefabSerializer::Deserialize(const std::filesystem::path& path)
	{
		std::ifstream stream(path);

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("File not found: {0}", path.string());
			return false;
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Prefab"].IsDefined())
		{
			HBL2_CORE_TRACE("Prefab not found: {0}", ss.str());
			stream.close();
			return false;
		}

		HBL2_CORE_TRACE("Deserializing Prefab at path: {0}", path);

		Scene* prefabSubScene = ResourceManager::Instance->GetScene(m_Context->m_SubSceneHandle);

		// Create the prefab entities and their components.
		const auto& prefabNode = data["Prefab"];

		auto components = prefabNode[0]["User Components"];

		// If we have user defined scripts but no dll exists, build it.
		if (components.size() > 0 && !BuildEngine::Instance->Exists())
		{
			HBL2_CORE_TRACE("No user defined scripts dll found for prefab: {}, building one now...", m_Context->m_UUID);
			BuildEngine::Instance->Build();
		}

		if (components)
		{
			HBL2_CORE_TRACE("Deserializing user components of prefab: {0}", m_Context->m_UUID);

			for (const auto& componentUUID : components)
			{
				Handle<Script> componentScriptHandle = AssetManager::Instance->GetAsset<Script>(componentUUID.as<UUID>());
				if (componentScriptHandle.IsValid())
				{
					Script* componentScript = ResourceManager::Instance->GetScript(componentScriptHandle);
					BuildEngine::Instance->RegisterComponent(componentScript->Name, prefabSubScene);
					HBL2_CORE_TRACE("Successfully resgistered user component: {0}", componentScript->Name);
				}
				else
				{
					HBL2_CORE_ERROR("Could not load component with UUID: {}", componentUUID.as<UUID>());
				}
			}
		}

		const auto& entityNodes = prefabNode[0]["Entities"];
		if (entityNodes)
		{
			for (const auto& entityNode : entityNodes)
			{
				EntitySerializer entitySerializer(prefabSubScene);
				entitySerializer.Deserialize(entityNode);
			}
		}

		stream.close();
		return true;
	}

	void PrefabSerializer::SerializePrefab(Scene* ctx, Entity entity, YAML::Emitter& out)
	{
		// Add the prefab entity component if the entity does not have it.
		if (!ctx->HasComponent<Component::PrefabEntity>(entity))
		{
			auto& prefabEntity = ctx->AddComponent<Component::PrefabEntity>(entity);
			prefabEntity.EntityId = Random::UInt64();
		}

		EntitySerializer entitySerializer(ctx, entity);
		entitySerializer.Serialize(out);

		auto* link = ctx->TryGetComponent<Component::Link>(entity);

		if (link == nullptr)
		{
			return;
		}

		for (auto child : link->Children)
		{
			Entity childEntity = ctx->FindEntityByUUID(child);
			SerializePrefab(ctx, childEntity, out);
		}
	}
}
