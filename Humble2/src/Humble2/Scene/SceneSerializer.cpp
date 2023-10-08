#include "SceneSerializer.h"

#include "Renderer\Texture.h"

namespace YAML
{
	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();

			return true;
		}
	};
}

namespace HBL2
{
	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w <<YAML::EndSeq;
		return out;
	}

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

		if (m_Scene->HasComponent<Component::Camera>(entity))
		{
			out << YAML::Key << "Component::Camera";
			out << YAML::BeginMap;

			auto& camera = m_Scene->GetComponent<Component::Camera>(entity);

			out << YAML::Key << "Enabled" << YAML::Value << camera.Enabled;
			out << YAML::Key << "Primary" << YAML::Value << camera.Primary;
			out << YAML::Key << "Static" << YAML::Value << camera.Static;
			out << YAML::Key << "Far" << YAML::Value << camera.Far;
			out << YAML::Key << "Near" << YAML::Value << camera.Near;
			out << YAML::Key << "FOV" << YAML::Value << camera.Fov;
			out << YAML::Key << "Aspect Ratio" << YAML::Value << camera.AspectRatio;
			out << YAML::Key << "Zoom Level" << YAML::Value << camera.ZoomLevel;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::Sprite>(entity))
		{
			out << YAML::Key << "Component::Sprite";
			out << YAML::BeginMap;

			auto& sprite = m_Scene->GetComponent<Component::Sprite>(entity);

			out << YAML::Key << "Enabled" << YAML::Value << sprite.Enabled;
			out << YAML::Key << "Static" << YAML::Value << sprite.Static;
			out << YAML::Key << "Color" << YAML::Value << sprite.Color;
			out << YAML::Key << "Texture" << YAML::Value << sprite.Path;
			out << YAML::EndMap;
		}

		if (m_Scene->HasComponent<Component::StaticMesh>(entity))
		{
			out << YAML::Key << "Component::StaticMesh";
			out << YAML::BeginMap;

			auto& staticMesh = m_Scene->GetComponent<Component::StaticMesh>(entity);

			out << YAML::Key << "Enabled" << YAML::Value << staticMesh.Enabled;
			out << YAML::Key << "Static" << YAML::Value << staticMesh.Static;
			out << YAML::Key << "Path" << YAML::Value << staticMesh.Path;
			out << YAML::Key << "TexturePath" << YAML::Value << staticMesh.TexturePath;
			out << YAML::Key << "ShaderName" << YAML::Value << staticMesh.ShaderName;
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
		m_Scene->GetRegistry().each([&](entt::entity entity)
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
		out << YAML::EndMap;

		try 
		{
			std::filesystem::create_directories(filePath.parent_path());
		}
		catch (std::exception& e) 
		{
			HBL2_ERROR("Project directory creation failed.");
		}

		std::ofstream fOut(filePath);
		fOut << out.c_str();
	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filePath)
	{
		std::ifstream stream(filePath);
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
				uint32_t uuid = entity["Entity"].as<uint32_t>();

				std::string name;
				auto tagComponent = entity["Component::Tag"];
				if (tagComponent)
				{
					name = tagComponent["Tag"].as<std::string>();
				}

				HBL2_CORE_TRACE("Deserializing entity with ID = {0}, name = {1}", uuid, name);

				entt::entity deserializedEntity = m_Scene->CreateEntity(name);
				auto& camera = m_Scene->AddComponent<Component::EditorVisible>(deserializedEntity);

				auto transformComponent = entity["Component::Transform"];
				if (transformComponent)
				{
					auto& transform = m_Scene->GetComponent<Component::Transform>(deserializedEntity);
					transform.Translation = transformComponent["Translation"].as<glm::vec3>();
					transform.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					transform.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				auto cameraComponent = entity["Component::Camera"];
				if (cameraComponent)
				{
					auto& camera = m_Scene->AddComponent<Component::Camera>(deserializedEntity);
					camera.Enabled = cameraComponent["Enabled"].as<bool>();
					camera.Primary = cameraComponent["Primary"].as<bool>();
					camera.Static = cameraComponent["Static"].as<bool>();

					camera.Far = cameraComponent["Far"].as<float>();
					camera.Near = cameraComponent["Near"].as<float>();
					camera.Fov = cameraComponent["FOV"].as<float>();
					camera.AspectRatio = cameraComponent["Aspect Ratio"].as<float>();
					camera.ZoomLevel = cameraComponent["Zoom Level"].as<float>();
				}

				auto spriteComponent = entity["Component::Sprite"];
				if (spriteComponent)
				{
					auto& sprite = m_Scene->AddComponent<Component::Sprite>(deserializedEntity);

					sprite.Enabled = spriteComponent["Enabled"].as<bool>();
					sprite.Static = spriteComponent["Static"].as<bool>();
					sprite.Color = spriteComponent["Color"].as<glm::vec4>();
					sprite.Path = spriteComponent["Texture"].as<std::string>();
					sprite.TextureIndex = Texture::Get(sprite.Path)->GetID();
				}

				auto staticMeshComponent = entity["Component::StaticMesh"];
				if (staticMeshComponent)
				{
					auto& staticMesh = m_Scene->AddComponent<Component::StaticMesh>(deserializedEntity);
					staticMesh.Enabled = staticMeshComponent["Enabled"].as<bool>();
					staticMesh.Static = staticMeshComponent["Static"].as<bool>();
					staticMesh.Path = staticMeshComponent["Path"].as<std::string>();
					staticMesh.TexturePath = staticMeshComponent["TexturePath"].as<std::string>();
					staticMesh.ShaderName = staticMeshComponent["ShaderName"].as<std::string>();
					staticMesh.TextureIndex = Texture::Get(staticMesh.TexturePath)->GetID();
				}
			}
		}

		return true;
	}
}