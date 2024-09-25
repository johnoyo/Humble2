#include "SceneSerializer.h"

#include "Renderer\Texture.h"
#include "Utilities\YamlUtilities.h"

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

			out << YAML::Key << "Parent" << YAML::Value << (uint32_t)link.Parent;
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
		out << YAML::Key << "Systems" << YAML::BeginSeq;

		for (ISystem* system : m_Scene->GetRuntimeSystems())
		{
			if (system->GetType() == SystemType::User)
			{
				out << YAML::Key << system->Name << YAML::Value;
			}
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
					auto& link = m_Scene->GetComponent<Component::Link>(deserializedEntity);
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
			}
		}

		auto systems = data["Systems"];
		if (systems)
		{
			for (auto system : systems)
			{
				const std::string& dllPath = "assets\\dlls\\" + system.as<std::string>() + "\\" + system.as<std::string>() + ".dll";
				ISystem* newSystem = NativeScriptUtilities::Get().LoadDLL(dllPath);

				HBL2_CORE_ASSERT(newSystem != nullptr, "Failed to load system.");

				if (m_Scene != nullptr)
				{
					newSystem->Name = system.as<std::string>();
					m_Scene->RegisterSystem(newSystem, SystemType::User);
				}
			}
		}

		return true;
	}
}