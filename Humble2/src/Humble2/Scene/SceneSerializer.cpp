#include "SceneSerializer.h"

#include "Utilities\UnityBuild.h"
#include "Utilities\NativeScriptUtilities.h"
#include "Utilities\YamlUtilities.h"

#include "EntitySerializer.h"

namespace HBL2
{
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
		m_Scene->GetRegistry()
			.view<entt::entity>()
			.each([&](entt::entity entity)
			{
				if (entity == entt::null)
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

		const std::vector<Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

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

		const auto& entityNodes = data["Entities"];
		if (entityNodes)
		{
			for (const auto& entityNode : entityNodes)
			{
				entt::entity deserializedEntity = entt::null;
				EntitySerializer entitySerializer(m_Scene, deserializedEntity);
				entitySerializer.Deserialize(entityNode);
			}
		}

		stream.close();

		return true;
	}
}