#include "PrefabSerializer.h"

#include "Core\Context.h"
#include "Utilities\Log.h"
#include "Utilities\YamlUtilities.h"
#include "Resources\ResourceManager.h"

#include "Scene\EntitySerializer.h"

namespace HBL2
{
	PrefabSerializer::PrefabSerializer(Prefab* prefab)
		: m_Context(prefab)
	{
	}

	void PrefabSerializer::Serialize(const std::filesystem::path& path)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab" << YAML::BeginSeq;

		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		auto* link = activeScene->TryGetComponent<Component::Link>(m_Context->GetBaseEntity());

		if (link == nullptr)
		{
			EntitySerializer entitySerializer(activeScene, m_Context->GetBaseEntity());
			entitySerializer.Serialize(out);
		}
		else
		{
			SerializePrefab(activeScene, m_Context->GetBaseEntity(), out);
		}

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

		std::string prefabName = data["Prefab"].as<std::string>();
		HBL2_CORE_TRACE("Deserializing Prefab: {0}", prefabName);

		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		const auto& entityNodes = data["Prefab"];
		if (entityNodes)
		{
			for (const auto& entityNode : entityNodes)
			{
				EntitySerializer entitySerializer(activeScene, m_Context->GetBaseEntity());
				entitySerializer.Deserialize(entityNode);
			}
		}

		stream.close();
		return true;
	}

	void PrefabSerializer::SerializePrefab(Scene* ctx, entt::entity entity, YAML::Emitter& out)
	{
		EntitySerializer entitySerializer(ctx, entity);
		entitySerializer.Serialize(out);

		auto* link = ctx->TryGetComponent<Component::Link>(entity);

		if (link == nullptr)
		{
			return;
		}

		for (auto child : link->Children)
		{
			entt::entity childEntity = ctx->FindEntityByUUID(child);
			SerializePrefab(ctx, childEntity, out);
		}
	}
}
