#include "SceneSerializer.h"

#include "Utilities\UnityBuild.h"
#include "Utilities\NativeScriptUtilities.h"
#include "Utilities\YamlUtilities.h"

#include "EntitySerializer.h"

namespace HBL2
{
	struct PrefabInfo
	{
		Component::Tag tag;
		Component::Transform transform;
		Component::PrefabInstance prefab;
	};

	SceneSerializer::SceneSerializer(Scene* scene) 
		: m_Scene(scene)
	{
	}

	void SceneSerializer::Serialize(const std::filesystem::path& filePath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetName();
		out << YAML::Key << "Entities" << YAML::BeginSeq;
		m_Scene->View<Entity>()
			.Each([&](Entity entity)
			{
				if (entity == Entity::Null)
				{
					return;
				}

				if (m_Scene->GetComponent<Component::Tag>(entity).Name != "Hidden")
				{
					EntitySerializer entitySerializer(m_Scene, entity);
					entitySerializer.Serialize(out);
				}
			});
		out << YAML::EndSeq;

		std::vector<UUID> componentUUIDs;
		std::vector<UUID> helperScriptUUIDs;
		std::vector<UUID> systemUUIDs;

		const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

		for (auto handle : assetHandles)
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

			if (asset->Type == AssetType::Script)
			{
				Script* script = ResourceManager::Instance->GetScript(Handle<Script>::UnPack(asset->Indentifier));

				if (script != nullptr)
				{
					if (script->Type == ScriptType::SYSTEM)
					{
						systemUUIDs.push_back(asset->UUID);
					}
					else if (script->Type == ScriptType::COMPONENT)
					{
						componentUUIDs.push_back(asset->UUID);
					}
					else if (script->Type == ScriptType::HELPER_SCRIPT)
					{
						helperScriptUUIDs.push_back(asset->UUID);
					}
				}
			}
		}

		out << YAML::Key << "User Systems" << YAML::BeginSeq;
		for (UUID systemUUID : systemUUIDs)
		{
			out << YAML::Key << systemUUID << YAML::Value;
		}
		out << YAML::EndSeq;

		out << YAML::Key << "User Components" << YAML::BeginSeq;
		for (UUID componentUUID : componentUUIDs)
		{
			out << YAML::Key << componentUUID << YAML::Value;
		}
		out << YAML::EndSeq;

		out << YAML::Key << "User Helper Scripts" << YAML::BeginSeq;
		for (UUID helperScriptUUID : helperScriptUUIDs)
		{
			out << YAML::Key << helperScriptUUID << YAML::Value;
		}
		out << YAML::EndSeq;

		out << YAML::EndMap;

		try 
		{
			std::filesystem::create_directories(filePath.parent_path());
		}
		catch (std::exception& e) 
		{
			HBL2_ERROR("Project directory creation failed: {0}", e.what());
		}

		std::ofstream fOut(filePath);

		if (!fOut.is_open())
		{
			HBL2_CORE_ERROR("File not found: {0}", filePath.string());
			return;
		}

		fOut << out.c_str();
		fOut.close();
	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filePath)
	{
		std::ifstream stream(filePath);

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("File not found: {0}", filePath.string());
			return false;
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Scene"].IsDefined())
		{
			HBL2_CORE_TRACE("Scene not found: {0}", ss.str());
			stream.close();
			return false;
		}

		std::string sceneName = data["Scene"].as<std::string>();
		HBL2_CORE_TRACE("Deserializing scene: {0}", sceneName);

		auto components = data["User Components"];
		auto helperScripts = data["User Helper Scripts"];
		auto systems = data["User Systems"];

		// If we have user defined scripts but no dll exists, build it.
		if ((components.size() > 0 || systems.size() > 0 || helperScripts.size() > 0) && !UnityBuild::Get().Exists())
		{
			HBL2_CORE_TRACE("No user defined scripts dll found for scene: {}, building one now...", m_Scene->GetName());

			UnityBuild::Get().Combine();
			UnityBuild::Get().Build();
		}

		if (components)
		{
			HBL2_CORE_TRACE("Deserializing user components of scene: {0}", sceneName);

			for (const auto& componentUUID : components)
			{
				Handle<Script> componentScriptHandle = AssetManager::Instance->GetAsset<Script>(componentUUID.as<UUID>());
				if (componentScriptHandle.IsValid())
				{
					Script* componentScript = ResourceManager::Instance->GetScript(componentScriptHandle);
					NativeScriptUtilities::Get().RegisterComponent(componentScript->Name, m_Scene);
					HBL2_CORE_TRACE("Successfully resgistered user component: {0}", componentScript->Name);
				}
				else
				{
					HBL2_CORE_ERROR("Could not load component with UUID: {}", componentUUID.as<UUID>());
				}
			}
		}

		if (helperScripts)
		{
			HBL2_CORE_TRACE("Deserializing user helper scripts of scene: {0}", sceneName);

			for (const auto& helperScriptUUID : helperScripts)
			{
				Handle<Script> helperScriptHandle = AssetManager::Instance->GetAsset<Script>(helperScriptUUID.as<UUID>());
				
				if (!helperScriptHandle.IsValid())
				{
					HBL2_CORE_ERROR("Could not load helper script with UUID: {}", helperScriptUUID.as<UUID>());
				}
			}
		}

		if (systems)
		{
			HBL2_CORE_TRACE("Deserializing user systems of scene: {0}", sceneName);

			for (const auto& systemUUID : systems)
			{
				Handle<Script> systemScriptHandle = AssetManager::Instance->GetAsset<Script>(systemUUID.as<UUID>());
				if (systemScriptHandle.IsValid())
				{
					Script* systemScript = ResourceManager::Instance->GetScript(systemScriptHandle);
					NativeScriptUtilities::Get().RegisterSystem(systemScript->Name, m_Scene);
					HBL2_CORE_TRACE("Successfully resgistered user system: {0}", systemScript->Name);
				}
				else
				{
					HBL2_CORE_ERROR("Could not load system with UUID: {}", systemUUID.as<UUID>());
				}
			}
		}

		// Deserialize all the entities into the scene.
		const auto& entityNodes = data["Entities"];
		if (entityNodes)
		{
			for (const auto& entityNode : entityNodes)
			{
				Entity deserializedEntity = Entity::Null;
				EntitySerializer entitySerializer(m_Scene, deserializedEntity);
				entitySerializer.Deserialize(entityNode);
			}
		}

		// --- PREFABS HANDLING BELOW ---
		// 
		//	1. Gather all prefabs in the scene.
		//	2. If the prefab is out of date.
		//		2.1 Destroy the prefab.
		//		2.2 Re-instantiate it and set its transform and tag.
		//		2.3 Update its version.
		//	3. If the prefab is not out of date do nothing.

		// NOTE: In the future, we need to update the children list of the parent of the prefab if it has one.
		//		 Since, we are deleting it and re-instantiating it, so the child uuid that the parent holds will be invalid.

		// Gather all prefab entities and their info.
		std::vector<Entity> prefabs;
		std::vector<PrefabInfo> prefabsInfo;

		m_Scene->View<Component::PrefabInstance>()
			.Each([&](Entity entity, Component::PrefabInstance& prefab)
			{
				prefabs.push_back(entity);

				auto& tag = m_Scene->GetComponent<Component::Tag>(entity);
				auto& tr = m_Scene->GetComponent<Component::Transform>(entity);
				prefabsInfo.push_back({ tag, tr, prefab });
			});

		HBL2_CORE_ASSERT(prefabs.size() == prefabsInfo.size(), "Expected prefabs and prefabsInfo arrays to be the same size!");

		// Iterate over all the prefabs into the scene and delete their entities if needed.
		for (int i = 0; i < prefabs.size(); i++)
		{
			auto& prefabInfo = prefabsInfo[i];

			Handle<Asset> prefabAssetHandle = AssetManager::Instance->GetHandleFromUUID(prefabInfo.prefab.Id);
			Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(prefabAssetHandle);
			Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

			if (prefab == nullptr)
			{
				HBL2_CORE_ERROR("Error while trying to update prefab while loading the scene!");
				continue;
			}

			// If there was an change in the source prefab.
			if (prefab->m_Version != prefabInfo.prefab.Version)
			{
				m_Scene->DestroyEntity(prefabs[i]);
			}
		}

		// Iterate over all the prefabs stored before and instantiate them if needed.
		for (int i = 0; i < prefabsInfo.size(); i++)
		{
			auto& prefabInfo = prefabsInfo[i];

			Handle<Asset> prefabAssetHandle = AssetManager::Instance->GetHandleFromUUID(prefabInfo.prefab.Id);
			Handle<Prefab> prefabHandle = AssetManager::Instance->GetAsset<Prefab>(prefabAssetHandle);
			Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

			if (prefab == nullptr)
			{
				HBL2_CORE_ERROR("Error while trying to update prefab while loading the scene!");
				continue;
			}

			// If there was an change in the source prefab.
			if (prefab->m_Version != prefabInfo.prefab.Version)
			{
				Entity clone = Prefab::Instantiate(prefabAssetHandle, m_Scene);

				if (clone != Entity::Null)
				{
					auto& tr = m_Scene->GetComponent<Component::Transform>(clone);
					tr.Translation = prefabInfo.transform.Translation;
					tr.Rotation = prefabInfo.transform.Rotation;
					tr.Scale = prefabInfo.transform.Scale;
					tr.Static = prefabInfo.transform.Static;

					auto& tagComponent = m_Scene->GetComponent<Component::Tag>(clone);
					tagComponent.Name = prefabInfo.tag.Name;

					auto& prefabComponent = m_Scene->GetComponent<Component::PrefabInstance>(clone);
					prefabComponent.Version = prefab->m_Version;
				}
			}
		}

		stream.close();

		return true;
	}
}