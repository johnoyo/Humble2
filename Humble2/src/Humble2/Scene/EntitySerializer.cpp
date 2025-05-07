#include "EntitySerializer.h"

#include "Utilities\YamlUtilities.h"
#include "UI\UserInterfaceUtilities.h"

namespace HBL2
{
	EntitySerializer::EntitySerializer(Scene* scene, entt::entity entity)
		: m_Scene(scene), m_Entity(entity)
	{
	}

	void EntitySerializer::Serialize(YAML::Emitter& out)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << (uint32_t)m_Entity;

		if (m_Scene->HasComponent<Component::Tag>(m_Entity))
		{
			out << YAML::Key << "Component::Tag";
			out << YAML::BeginMap;
			out << YAML::Key << "Tag" << YAML::Value << m_Scene->GetComponent<Component::Tag>(m_Entity).Name;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::ID>(m_Entity))
		{
			out << YAML::Key << "Component::ID";
			out << YAML::BeginMap;
			out << YAML::Key << "ID" << YAML::Value << m_Scene->GetComponent<Component::ID>(m_Entity).Identifier;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Transform>(m_Entity))
		{
			out << YAML::Key << "Component::Transform";
			out << YAML::BeginMap;

			auto& transform = m_Scene->GetComponent<Component::Transform>(m_Entity);

			out << YAML::Key << "Translation" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Link>(m_Entity))
		{
			out << YAML::Key << "Component::Link";
			out << YAML::BeginMap;

			auto& link = m_Scene->GetComponent<Component::Link>(m_Entity);

			out << YAML::Key << "Parent" << YAML::Value << (UUID)link.Parent;
			out << YAML::Key << "Children" << YAML::Value << link.Children;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Camera>(m_Entity))
		{
			out << YAML::Key << "Component::Camera";
			out << YAML::BeginMap;

			auto& camera = m_Scene->GetComponent<Component::Camera>(m_Entity);

			out << YAML::Key << "Enabled" << YAML::Value << camera.Enabled;
			out << YAML::Key << "Primary" << YAML::Value << camera.Primary;
			out << YAML::Key << "Far" << YAML::Value << camera.Far;
			out << YAML::Key << "Near" << YAML::Value << camera.Near;
			out << YAML::Key << "FOV" << YAML::Value << camera.Fov;
			out << YAML::Key << "Aspect Ratio" << YAML::Value << camera.AspectRatio;
			out << YAML::Key << "Exposure" << YAML::Value << camera.Exposure;
			out << YAML::Key << "Gamma" << YAML::Value << camera.Gamma;
			out << YAML::Key << "Zoom Level" << YAML::Value << camera.ZoomLevel;
			out << YAML::Key << "Type" << YAML::Value << (int)camera.Type;

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Sprite>(m_Entity))
		{
			out << YAML::Key << "Component::Sprite";
			out << YAML::BeginMap;

			auto& sprite = m_Scene->GetComponent<Component::Sprite>(m_Entity);

			out << YAML::Key << "Enabled" << YAML::Value << sprite.Enabled;

			const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

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

			out << YAML::Key << "Material" << YAML::Value << (materialAsset != nullptr ? materialAsset->UUID : (UUID)0);

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::StaticMesh>(m_Entity))
		{
			out << YAML::Key << "Component::StaticMesh";
			out << YAML::BeginMap;

			auto& staticMesh = m_Scene->GetComponent<Component::StaticMesh>(m_Entity);

			out << YAML::Key << "Enabled" << YAML::Value << staticMesh.Enabled;

			const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

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

			out << YAML::Key << "Material" << YAML::Value << (materialAsset != nullptr ? materialAsset->UUID : (UUID)0);

			out << YAML::Key << "Mesh" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << (meshAsset != nullptr ? meshAsset->UUID : (UUID)0);
			out << YAML::Key << "MeshIndex" << YAML::Value << staticMesh.MeshIndex;
			out << YAML::Key << "SubMeshIndex" << YAML::Value << staticMesh.SubMeshIndex;
			out << YAML::EndMap;

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Light>(m_Entity))
		{
			auto& light = m_Scene->GetComponent<Component::Light>(m_Entity);

			out << YAML::Key << "Component::Light";

			out << YAML::BeginMap;

			out << YAML::Key << "Enabled" << YAML::Value << light.Enabled;
			out << YAML::Key << "CastsShadows" << YAML::Value << light.CastsShadows;
			out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
			out << YAML::Key << "Color" << YAML::Value << light.Color;
			out << YAML::Key << "Type" << YAML::Value << (int)light.Type;
			out << YAML::Key << "Distance" << YAML::Value << light.Distance;
			out << YAML::Key << "InnerCutOff" << YAML::Value << light.InnerCutOff;
			out << YAML::Key << "OuterCutOff" << YAML::Value << light.OuterCutOff;
			out << YAML::Key << "ConstantBias" << YAML::Value << light.ConstantBias;
			out << YAML::Key << "SlopeBias" << YAML::Value << light.SlopeBias;
			out << YAML::Key << "FieldOfView" << YAML::Value << light.FieldOfView;

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::SkyLight>(m_Entity))
		{
			auto& light = m_Scene->GetComponent<Component::SkyLight>(m_Entity);

			out << YAML::Key << "Component::SkyLight";

			out << YAML::BeginMap;

			out << YAML::Key << "Enabled" << YAML::Value << light.Enabled;

			const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* textureAsset = nullptr;
			bool textureFound = false;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Texture && asset->Indentifier != 0 && asset->Indentifier == light.EquirectangularMap.Pack() && !textureFound)
				{
					textureFound = true;
					textureAsset = asset;
				}

				if (textureFound)
				{
					break;
				}
			}

			out << YAML::Key << "EquirectangularMap" << YAML::Value << (textureAsset != nullptr ? textureAsset->UUID : (UUID)0);

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::AudioSource>(m_Entity))
		{
			out << YAML::Key << "Component::AudioSource";
			out << YAML::BeginMap;

			auto& soundSource = m_Scene->GetComponent<Component::AudioSource>(m_Entity);

			out << YAML::Key << "Enabled" << YAML::Value << soundSource.Enabled;

			const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

			Asset* soundAsset = nullptr;
			bool soundFound = false;

			for (auto handle : assetHandles)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Sound && asset->Indentifier != 0 && asset->Indentifier == soundSource.Sound.Pack() && !soundFound)
				{
					soundFound = true;
					soundAsset = asset;
				}

				if (soundFound)
				{
					break;
				}
			}

			out << YAML::Key << "Sound" << YAML::Value << (soundAsset != nullptr ? soundAsset->UUID : (UUID)0);

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Rigidbody2D>(m_Entity))
		{
			out << YAML::Key << "Component::Rigidbody2D";
			out << YAML::BeginMap;

			auto& rb2d = m_Scene->GetComponent<Component::Rigidbody2D>(m_Entity);

			out << YAML::Key << "Enabled" << YAML::Value << rb2d.Enabled;
			out << YAML::Key << "Type" << YAML::Value << (int)rb2d.Type;
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2d.FixedRotation;

			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::BoxCollider2D>(m_Entity))
		{
			out << YAML::Key << "Component::BoxCollider2D";
			out << YAML::BeginMap;

			auto& bc2d = m_Scene->GetComponent<Component::BoxCollider2D>(m_Entity);

			out << YAML::Key << "Enabled" << YAML::Value << bc2d.Enabled;
			out << YAML::Key << "Density" << YAML::Value << bc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2d.Restitution;
			out << YAML::Key << "Size" << YAML::Value << bc2d.Size;
			out << YAML::Key << "Offset" << YAML::Value << bc2d.Offset;

			out << YAML::EndMap;
		}

		for (auto meta_type : entt::resolve(m_Scene->GetMetaContext()))
		{
			const std::string& componentName = meta_type.second.info().name().data();

			const std::string& cleanedComponentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

			if (NativeScriptUtilities::Get().HasComponent(cleanedComponentName, m_Scene, m_Entity))
			{
				auto componentMeta = NativeScriptUtilities::Get().GetComponent(cleanedComponentName, m_Scene, m_Entity);
				out << YAML::Key << cleanedComponentName;

				out << YAML::BeginMap;
				EditorUtilities::Get().SerializeComponentToYAML(out, componentMeta, m_Scene);
				out << YAML::EndMap;
			}
		}

		out << YAML::EndMap;
	}

	bool EntitySerializer::Deserialize(const YAML::Node& entityNode)
	{
		if (!entityNode["Entity"].IsDefined())
		{
			return false;
		}

		uint32_t entityID = entityNode["Entity"].as<uint32_t>();

		std::string name;
		auto tagComponent = entityNode["Component::Tag"];
		if (tagComponent)
		{
			name = tagComponent["Tag"].as<std::string>();
		}

		UUID uuid;
		auto idComponent = entityNode["Component::ID"];
		if (idComponent)
		{
			uuid = idComponent["ID"].as<UUID>();
		}

		HBL2_CORE_TRACE("Deserializing entity-{0} with UUID = {1}, Name = {2}", entityID, uuid, name);

		m_Entity = m_Scene->CreateEntityWithUUID(uuid, name);

		auto& editorVisible = m_Scene->AddComponent<Component::EditorVisible>(m_Entity);

		auto transformComponent = entityNode["Component::Transform"];
		if (transformComponent)
		{
			auto& transform = m_Scene->GetComponent<Component::Transform>(m_Entity);
			transform.Translation = transformComponent["Translation"].as<glm::vec3>();
			transform.Rotation = transformComponent["Rotation"].as<glm::vec3>();
			transform.Scale = transformComponent["Scale"].as<glm::vec3>();
		}

		auto linkComponent = entityNode["Component::Link"];
		if (linkComponent)
		{
			auto& link = m_Scene->AddComponent<Component::Link>(m_Entity);
			link.Parent = linkComponent["Parent"].as<UUID>();
			link.PrevParent = link.Parent;
			if (linkComponent["Children"].IsDefined())
			{
				link.Children = linkComponent["Children"].as<std::vector<UUID>>();
			}
		}

		auto cameraComponent = entityNode["Component::Camera"];
		if (cameraComponent)
		{
			auto& camera = m_Scene->AddComponent<Component::Camera>(m_Entity);
			camera.Enabled = cameraComponent["Enabled"].as<bool>();
			camera.Primary = cameraComponent["Primary"].as<bool>();

			camera.Far = cameraComponent["Far"].as<float>();
			camera.Near = cameraComponent["Near"].as<float>();
			camera.Fov = cameraComponent["FOV"].as<float>();
			camera.AspectRatio = cameraComponent["Aspect Ratio"].as<float>();
			camera.Exposure = cameraComponent["Exposure"].as<float>();
			camera.Gamma = cameraComponent["Gamma"].as<float>();
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

		auto sprite_NewComponent = entityNode["Component::Sprite"];
		if (sprite_NewComponent)
		{
			auto& sprite = m_Scene->AddComponent<Component::Sprite>(m_Entity);
			sprite.Enabled = sprite_NewComponent["Enabled"].as<bool>();
			sprite.Material = AssetManager::Instance->GetAsset<Material>(sprite_NewComponent["Material"].as<UUID>());
		}

		auto staticMesh_NewComponent = entityNode["Component::StaticMesh"];
		if (staticMesh_NewComponent)
		{
			auto& staticMesh = m_Scene->AddComponent<Component::StaticMesh>(m_Entity);
			staticMesh.Enabled = staticMesh_NewComponent["Enabled"].as<bool>();
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(staticMesh_NewComponent["Material"].as<UUID>());

			if (staticMesh_NewComponent["Mesh"].IsDefined())
			{
				staticMesh.Mesh = AssetManager::Instance->GetAsset<Mesh>(staticMesh_NewComponent["Mesh"]["UUID"].as<UUID>());
				staticMesh.MeshIndex = staticMesh_NewComponent["Mesh"]["MeshIndex"].as<uint32_t>();
				staticMesh.SubMeshIndex = staticMesh_NewComponent["Mesh"]["SubMeshIndex"].as<uint32_t>();
			}
		}

		auto light_NewComponent = entityNode["Component::Light"];
		if (light_NewComponent)
		{
			auto& light = m_Scene->AddComponent<Component::Light>(m_Entity);
			light.Enabled = light_NewComponent["Enabled"].as<bool>();
			light.CastsShadows = light_NewComponent["CastsShadows"].as<bool>();
			light.Intensity = light_NewComponent["Intensity"].as<float>();
			light.Distance = light_NewComponent["Distance"].as<float>();
			light.InnerCutOff = light_NewComponent["InnerCutOff"].as<float>();
			light.OuterCutOff = light_NewComponent["OuterCutOff"].as<float>();
			light.ConstantBias = light_NewComponent["ConstantBias"].as<float>();
			light.SlopeBias = light_NewComponent["SlopeBias"].as<float>();
			light.FieldOfView = light_NewComponent["FieldOfView"].as<float>();
			light.Color = light_NewComponent["Color"].as<glm::vec3>();
			if (light_NewComponent["Type"])
			{
				switch (light_NewComponent["Type"].as<int>())
				{
				case 1:
					light.Type = Component::Light::Type::Directional;
					break;
				case 2:
					light.Type = Component::Light::Type::Point;
					break;
				case 3:
					light.Type = Component::Light::Type::Spot;
					break;
				default:
					break;
				}
			}
		}

		auto skyLight_NewComponent = entityNode["Component::SkyLight"];
		if (skyLight_NewComponent)
		{
			auto& skyLight = m_Scene->AddComponent<Component::SkyLight>(m_Entity);
			skyLight.Enabled = skyLight_NewComponent["Enabled"].as<bool>();
			skyLight.EquirectangularMap = AssetManager::Instance->GetAsset<Texture>(skyLight_NewComponent["EquirectangularMap"].as<UUID>());
		}

		auto soundSource_NewComponent = entityNode["Component::AudioSource"];
		if (soundSource_NewComponent)
		{
			auto& soundSource = m_Scene->AddComponent<Component::AudioSource>(m_Entity);
			soundSource.Enabled = soundSource_NewComponent["Enabled"].as<bool>();
			soundSource.Sound = AssetManager::Instance->GetAsset<Sound>(soundSource_NewComponent["Sound"].as<UUID>());
		}

		auto rb2d_NewComponent = entityNode["Component::Rigidbody2D"];
		if (rb2d_NewComponent)
		{
			auto& rb2d = m_Scene->AddComponent<Component::Rigidbody2D>(m_Entity);
			rb2d.Enabled = rb2d_NewComponent["Enabled"].as<bool>();
			if (rb2d_NewComponent["Type"])
			{
				switch (rb2d_NewComponent["Type"].as<int>())
				{
				case 0:
					rb2d.Type = Component::Rigidbody2D::BodyType::Static;
					break;
				case 1:
					rb2d.Type = Component::Rigidbody2D::BodyType::Dynamic;
					break;
				case 2:
					rb2d.Type = Component::Rigidbody2D::BodyType::Kinematic;
					break;
				default:
					rb2d.Type = Component::Rigidbody2D::BodyType::Static;
					break;
				}
			}
			rb2d.FixedRotation = rb2d_NewComponent["FixedRotation"].as<bool>();
		}

		auto bc2d_NewComponent = entityNode["Component::BoxCollider2D"];
		if (bc2d_NewComponent)
		{
			auto& bc2d = m_Scene->AddComponent<Component::BoxCollider2D>(m_Entity);
			bc2d.Enabled = bc2d_NewComponent["Enabled"].as<bool>();
			bc2d.Density = bc2d_NewComponent["Density"].as<float>();
			bc2d.Friction = bc2d_NewComponent["Friction"].as<float>();
			bc2d.Restitution = bc2d_NewComponent["Restitution"].as<float>();
			bc2d.Size = bc2d_NewComponent["Size"].as<glm::vec2>();
			bc2d.Offset = bc2d_NewComponent["Offset"].as<glm::vec2>();
		}

		for (auto meta_type : entt::resolve(m_Scene->GetMetaContext()))
		{
			const std::string& componentName = meta_type.second.info().name().data();
			const std::string& cleanedComponentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

			auto userComponent = entityNode[cleanedComponentName];
			if (userComponent)
			{
				auto componentMeta = NativeScriptUtilities::Get().AddComponent(cleanedComponentName, m_Scene, m_Entity);
				EditorUtilities::Get().DeserializeComponentFromYAML(userComponent, componentMeta, m_Scene);
			}
		}

		return true;
	}
}
