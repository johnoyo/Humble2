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
		entt::entity baseEntity = activeScene->FindEntityByUUID(m_Context->GetBaseEntityUUID());

		// Find scene asset uuid to store it in the scene refs of the prefab.
		const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

		for (auto handle : assetHandles)
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

			if (asset->Type == AssetType::Scene)
			{
				if (Context::ActiveScene == Handle<Scene>::UnPack(asset->Indentifier))
				{
					m_Context->m_SceneRefs.push_back(asset->UUID);
					break;
				}
			}
		}

		// Add the prefab component if the base entity does not have it.
		if (!activeScene->HasComponent<Component::Prefab>(baseEntity))
		{
			auto& prefab = activeScene->AddComponent<Component::Prefab>(baseEntity);
			prefab.Id = Random::UInt64();
		}

		// Check if the entity has any children through the link component.
		auto* link = activeScene->TryGetComponent<Component::Link>(baseEntity);

		if (link == nullptr)
		{
			EntitySerializer entitySerializer(activeScene, baseEntity);
			entitySerializer.Serialize(out);
		}
		else
		{
			SerializePrefab(activeScene, baseEntity, out);
		}

		out << YAML::BeginMap;
		out << YAML::Key << "References" << YAML::BeginSeq;

		out << YAML::BeginMap;
		out << YAML::Key << "Scenes" << YAML::BeginSeq;
		for (UUID sceneUUID : m_Context->m_SceneRefs)
		{
			out << YAML::Key << sceneUUID << YAML::Value;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		out << YAML::BeginMap;
		out << YAML::Key << "Prefabs" << YAML::BeginSeq;
		for (UUID prefabUUID : m_Context->m_PrefabRefs)
		{
			out << YAML::Key << prefabUUID << YAML::Value;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

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
		entt::entity baseEntity = activeScene->FindEntityByUUID(m_Context->GetBaseEntityUUID());

		const auto& entityNodes = data["Prefab"];
		if (entityNodes)
		{
			for (const auto& entityNode : entityNodes)
			{
				EntitySerializer entitySerializer(activeScene, baseEntity);
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
