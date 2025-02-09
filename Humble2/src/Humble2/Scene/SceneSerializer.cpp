#include "SceneSerializer.h"

#include "Utilities\YamlUtilities.h"
#include "UI\UserInterfaceUtilities.h"

namespace HBL2
{
	SceneSerializer::SceneSerializer(Scene* scene) 
		: m_Scene(scene)
	{
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, entt::entity entity)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << (uint32_t)entity;

		if (m_Scene->HasComponent<Component::Tag>(entity))
		{
			out << YAML::Key << "Component::Tag";
			out << YAML::BeginMap;
			out << YAML::Key << "Tag" << YAML::Value << m_Scene->GetComponent<Component::Tag>(entity).Name;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::ID>(entity))
		{
			out << YAML::Key << "Component::ID";
			out << YAML::BeginMap;
			out << YAML::Key << "ID" << YAML::Value << m_Scene->GetComponent<Component::ID>(entity).Identifier;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Transform>(entity))
		{
			out << YAML::Key << "Component::Transform";
			out << YAML::BeginMap;

			auto& transform = m_Scene->GetComponent<Component::Transform>(entity);

			out << YAML::Key << "Translation" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Link>(entity))
		{
			out << YAML::Key << "Component::Link";
			out << YAML::BeginMap;

			auto& link = m_Scene->GetComponent<Component::Link>(entity);

			out << YAML::Key << "Parent" << YAML::Value << (UUID)link.Parent;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Camera>(entity))
		{
			out << YAML::Key << "Component::Camera";
			out << YAML::BeginMap;

			auto& camera = m_Scene->GetComponent<Component::Camera>(entity);

			out << YAML::Key << "Enabled" << YAML::Value << camera.Enabled;
			out << YAML::Key << "Primary" << YAML::Value << camera.Primary;
			out << YAML::Key << "Far" << YAML::Value << camera.Far;
			out << YAML::Key << "Near" << YAML::Value << camera.Near;
			out << YAML::Key << "FOV" << YAML::Value << camera.Fov;
			out << YAML::Key << "Aspect Ratio" << YAML::Value << camera.AspectRatio;
			out << YAML::Key << "Zoom Level" << YAML::Value << camera.ZoomLevel;
			out << YAML::Key << "Type" << YAML::Value << (int)camera.Type;

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Sprite_New>(entity))
		{
			out << YAML::Key << "Component::Sprite_New";
			out << YAML::BeginMap;

			auto& sprite = m_Scene->GetComponent<Component::Sprite_New>(entity);

			out << YAML::Key << "Enabled" << YAML::Value << sprite.Enabled;

			const std::vector<Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* materialAsset = nullptr;

			bool materialFound = false;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Material && asset->Indentifier != 0 && asset->Indentifier == sprite.Material.Pack() && !materialFound)
				{
					materialFound = true;
					materialAsset = asset;
				}

				if (materialFound)
				{
					break;
				}
			}

			if (materialAsset != nullptr)
			{
				out << YAML::Key << "Material" << YAML::Value << materialAsset->UUID;
			}
			else
			{
				out << YAML::Key << "Material" << YAML::Value << (UUID)0;
			}

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::StaticMesh_New>(entity))
		{
			out << YAML::Key << "Component::StaticMesh_New";
			out << YAML::BeginMap;

			auto& staticMesh = m_Scene->GetComponent<Component::StaticMesh_New>(entity);

			out << YAML::Key << "Enabled" << YAML::Value << staticMesh.Enabled;

			const std::vector<Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* materialAsset = nullptr;
			Asset* meshAsset = nullptr;

			bool meshFound = false;
			bool materialFound = false;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Material && asset->Indentifier != 0 && asset->Indentifier == staticMesh.Material.Pack() && !materialFound)
				{
					materialFound = true;
					materialAsset = asset;
				}
				if (asset->Type == AssetType::Mesh && asset->Indentifier != 0 && asset->Indentifier == staticMesh.Mesh.Pack() && !meshFound)
				{
					meshFound = true;
					meshAsset = asset;
				}

				if (materialFound && meshFound)
				{
					break;
				}
			}

			if (materialAsset != nullptr)
			{
				out << YAML::Key << "Material" << YAML::Value << materialAsset->UUID;
			}
			else
			{
				out << YAML::Key << "Material" << YAML::Value << (UUID)0;
			}

			if (meshAsset != nullptr)
			{
				out << YAML::Key << "Mesh" << YAML::Value << meshAsset->UUID;
			}
			else
			{
				out << YAML::Key << "Mesh" << YAML::Value << (UUID)0;
			}

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Light>(entity))
		{
			auto& light = m_Scene->GetComponent<Component::Light>(entity);

			out << YAML::Key << "Component::Light";

			out << YAML::BeginMap;

			out << YAML::Key << "Enabled" << YAML::Value << light.Enabled;
			out << YAML::Key << "CastsShadows" << YAML::Value << light.CastsShadows;
			out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
			out << YAML::Key << "Attenuation" << YAML::Value << light.Attenuation;
			out << YAML::Key << "Color" << YAML::Value << light.Color;
			out << YAML::Key << "Type" << YAML::Value << (int)light.Type;

			out << YAML::EndMap;
		}

		for (auto meta_type : entt::resolve(m_Scene->GetMetaContext()))
		{
			const std::string& componentName = meta_type.second.info().name().data();

			const std::string& cleanedComponentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

			if (NativeScriptUtilities::Get().HasComponent(cleanedComponentName, m_Scene, entity))
			{
				auto componentMeta = NativeScriptUtilities::Get().GetComponent(cleanedComponentName, m_Scene, entity);
				out << YAML::Key << cleanedComponentName;

				out << YAML::BeginMap;
				EditorUtilities::Get().SerializeComponentToYAML(out, componentMeta, m_Scene);
				out << YAML::EndMap;
			}
		}

		out << YAML::EndMap;
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
					SerializeEntity(out, entity);
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
			return false;
		}

		std::string sceneName = data["Scene"].as<std::string>();
		HBL2_CORE_TRACE("Deserializing scene: {0}", sceneName);

		HBL2_CORE_TRACE("Deserializing user components of scene: {0}", sceneName);
		auto components = data["User Components"];
		if (components)
		{
			for (auto componentUUID : components)
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

		HBL2_CORE_TRACE("Deserializing user helper scripts of scene: {0}", sceneName);
		auto helperScripts = data["User Helper Scripts"];
		if (helperScripts)
		{
			for (auto helperScriptUUID : helperScripts)
			{
				Handle<Script> helperScriptHandle = AssetManager::Instance->GetAsset<Script>(helperScriptUUID.as<UUID>());
				
				if (!helperScriptHandle.IsValid())
				{
					HBL2_CORE_ERROR("Could not load helper script with UUID: {}", helperScriptUUID.as<UUID>());
				}
			}
		}

		HBL2_CORE_TRACE("Deserializing user systems of scene: {0}", sceneName);
		auto systems = data["User Systems"];
		if (systems)
		{
			for (auto systemUUID : systems)
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

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint32_t entityID = entity["Entity"].as<uint32_t>();

				std::string name;
				auto tagComponent = entity["Component::Tag"];
				if (tagComponent)
				{
					name = tagComponent["Tag"].as<std::string>();
				}

				UUID uuid;
				auto idComponent = entity["Component::ID"];
				if (idComponent)
				{
					uuid = idComponent["ID"].as<UUID>();
				}

				HBL2_CORE_TRACE("Deserializing entity-{0} with UUID = {1}, Name = {2}", entityID, uuid, name);

				entt::entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				auto& editorVisible = m_Scene->AddComponent<Component::EditorVisible>(deserializedEntity);

				auto transformComponent = entity["Component::Transform"];
				if (transformComponent)
				{
					auto& transform = m_Scene->GetComponent<Component::Transform>(deserializedEntity);
					transform.Translation = transformComponent["Translation"].as<glm::vec3>();
					transform.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					transform.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				auto linkComponent = entity["Component::Link"];
				if (linkComponent)
				{
					auto& link = m_Scene->AddComponent<Component::Link>(deserializedEntity);
					link.Parent = linkComponent["Parent"].as<UUID>();
				}

				auto cameraComponent = entity["Component::Camera"];
				if (cameraComponent)
				{
					auto& camera = m_Scene->AddComponent<Component::Camera>(deserializedEntity);
					camera.Enabled = cameraComponent["Enabled"].as<bool>();
					camera.Primary = cameraComponent["Primary"].as<bool>();

					camera.Far = cameraComponent["Far"].as<float>();
					camera.Near = cameraComponent["Near"].as<float>();
					camera.Fov = cameraComponent["FOV"].as<float>();
					camera.AspectRatio = cameraComponent["Aspect Ratio"].as<float>();
					camera.ZoomLevel = cameraComponent["Zoom Level"].as<float>();
					if (cameraComponent["Type"])
					{
						switch (cameraComponent["Type"].as<int>())
						{
						case 1:
							camera.Type = Component::Camera::Type::Perspective;
							break;
						case 2:
							camera.Type = Component::Camera::Type::Orthographic;
							break;
						default:
							break;
						}
					}
				}

				auto sprite_NewComponent = entity["Component::Sprite_New"];
				if (sprite_NewComponent)
				{
					auto& sprite = m_Scene->AddComponent<Component::Sprite_New>(deserializedEntity);
					sprite.Enabled = sprite_NewComponent["Enabled"].as<bool>();
					sprite.Material = AssetManager::Instance->GetAsset<Material>(sprite_NewComponent["Material"].as<UUID>());
				}

				auto staticMesh_NewComponent = entity["Component::StaticMesh_New"];
				if (staticMesh_NewComponent)
				{
					auto& staticMesh = m_Scene->AddComponent<Component::StaticMesh_New>(deserializedEntity);
					staticMesh.Enabled = staticMesh_NewComponent["Enabled"].as<bool>();
					staticMesh.Material = AssetManager::Instance->GetAsset<Material>(staticMesh_NewComponent["Material"].as<UUID>());
					staticMesh.Mesh = AssetManager::Instance->GetAsset<Mesh>(staticMesh_NewComponent["Mesh"].as<UUID>());
				}

				auto lightComponent = entity["Component::Light"];
				if (lightComponent)
				{
					auto& light = m_Scene->AddComponent<Component::Light>(deserializedEntity);
					light.Enabled = lightComponent["Enabled"].as<bool>();
					light.CastsShadows = lightComponent["CastsShadows"].as<bool>();
					light.Intensity = lightComponent["Intensity"].as<float>();
					light.Attenuation = lightComponent["Attenuation"].as<float>();
					light.Color = lightComponent["Color"].as<glm::vec3>();
					if (lightComponent["Type"])
					{
						switch (lightComponent["Type"].as<int>())
						{
						case 1:
							light.Type = Component::Light::Type::Directional;
							break;
						case 2:
							light.Type = Component::Light::Type::Point;
							break;
						default:
							break;
						}
					}
				}

				for (auto meta_type : entt::resolve(m_Scene->GetMetaContext()))
				{
					const std::string& componentName = meta_type.second.info().name().data();
					const std::string& cleanedComponentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

					auto userComponent = entity[cleanedComponentName];
					if (userComponent)
					{
						auto componentMeta = NativeScriptUtilities::Get().AddComponent(cleanedComponentName, m_Scene, deserializedEntity);
						EditorUtilities::Get().DeserializeComponentFromYAML(userComponent, componentMeta, m_Scene);
					}
				}
			}
		}

		return true;
	}
}