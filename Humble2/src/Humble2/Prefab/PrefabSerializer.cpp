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

	PrefabSerializer::PrefabSerializer(Prefab* prefab, Scene* scene)
		: m_Context(prefab), m_Scene(scene)
	{
	}

	PrefabSerializer::PrefabSerializer(Prefab* prefab, entt::entity instantiatedPrefabEntity)
		: m_Context(prefab), m_InstantiatedPrefabEntity(instantiatedPrefabEntity)
	{
	}

	void PrefabSerializer::Serialize(const std::filesystem::path& path)
	{
		/*		
		- Instantiatiation: (Spawn into the scene)
			- Instantiate the prefab into the scene from the file.
			- Duplicate the instantiated entity so it has unique ID.
			- Delete initial instantiated entity.
		- Update: (the prefab is already instantiated and we want to update it since the source prefab changed.)
			- ...
		*/

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab" << YAML::BeginSeq;

		out << YAML::BeginMap;
		out << YAML::Key << "Entities" << YAML::BeginSeq;

		Scene* activeScene = GetScene();
		entt::entity baseEntity;

		if (m_InstantiatedPrefabEntity != entt::null)
		{
			baseEntity = m_InstantiatedPrefabEntity;
		}
		else
		{
			// NOTE: This branch is only hit when we drag and drop an entity from the hierachy panel
			//		 to the content browser to create a new prefab.
			baseEntity = activeScene->FindEntityByUUID(m_Context->GetBaseEntityUUID());
		}

		// Find scene asset uuid to store it in the scene refs of the prefab.
		const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

		for (auto handle : assetHandles)
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

			if (asset->Type == AssetType::Scene)
			{
				Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);
				Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

				if (activeScene == scene)
				{
					if (std::find(m_Context->m_SceneRefs.begin(), m_Context->m_SceneRefs.end(), asset->UUID) != m_Context->m_SceneRefs.end())
					{
						continue;
					}

					m_Context->m_SceneRefs.push_back(asset->UUID);
					break;
				}
			}
		}

		// Add the prefab component if the base entity does not have it.
		if (!activeScene->HasComponent<Component::PrefabInstance>(baseEntity))
		{
			auto& prefab = activeScene->AddComponent<Component::PrefabInstance>(baseEntity);
			prefab.Id = m_Context->m_UUID;
			prefab.Version = m_Context->m_Version;
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

		out << YAML::EndSeq;
		out << YAML::EndMap;

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

		m_Context->m_Version++;
	}

	void PrefabSerializer::SerializeReferences(const std::filesystem::path& path)
	{
		YAML::Node root = YAML::LoadFile(path.string());
		YAML::Node prefabEntry = root["Prefab"][1];

		YAML::Node refsSeq(YAML::NodeType::Sequence);

		// Reconstruct the scene refs node.
		YAML::Node scenesMap(YAML::NodeType::Map);
		scenesMap["Scenes"] = YAML::Node(YAML::NodeType::Sequence);
		for (UUID sceneUUID : m_Context->m_SceneRefs)
		{
			scenesMap["Scenes"].push_back(sceneUUID);
		}
		refsSeq.push_back(scenesMap);

		// Reconstruct the prefab refs node.
		YAML::Node prefabsMap(YAML::NodeType::Map);
		prefabsMap["Prefabs"] = YAML::Node(YAML::NodeType::Sequence);
		for (UUID prefabUUID : m_Context->m_PrefabRefs)
		{
			prefabsMap["Prefabs"].push_back(prefabUUID);
		}
		refsSeq.push_back(prefabsMap);

		// Reconstruct the refs node.
		prefabEntry["References"] = refsSeq;

		std::ofstream fout(path.string());
		if (!fout.is_open())
		{
			HBL2_CORE_ERROR("Failed to open prefab for writing: {0}", path.string());
			return;
		}

		fout << root;
		fout.close();
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

		Scene* activeScene = GetScene();

		// Find scene asset uuid to store it in the scene refs of the prefab.
		const Span<const Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

		for (auto handle : assetHandles)
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

			if (asset->Type == AssetType::Scene)
			{
				Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);
				Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

				if (activeScene == scene)
				{
					if (std::find(m_Context->m_SceneRefs.begin(), m_Context->m_SceneRefs.end(), asset->UUID) != m_Context->m_SceneRefs.end())
					{
						continue;
					}

					m_Context->m_SceneRefs.push_back(asset->UUID);
					break;
				}
			}
		}

		// Create the prefab entities and their components.
		const auto& prefabNode = data["Prefab"];

		const auto& entityNodes = prefabNode[0]["Entities"];
		if (entityNodes)
		{
			for (const auto& entityNode : entityNodes)
			{
				EntitySerializer entitySerializer(activeScene, entt::null);
				entitySerializer.Deserialize(entityNode);
			}
		}

		// Update the prefab scene references.
		const auto& refs = prefabNode[1]["References"];
		if (refs)
		{
			const auto& sceneRefs = refs[0]["Scenes"];

			for (const auto& sceneRef : sceneRefs)
			{
				if (std::find(m_Context->m_SceneRefs.begin(), m_Context->m_SceneRefs.end(), sceneRef.as<UUID>()) != m_Context->m_SceneRefs.end())
				{
					continue;
				}

				m_Context->m_SceneRefs.push_back(sceneRef.as<UUID>());
			}
		}

		stream.close();
		return true;
	}

	bool PrefabSerializer::DeserializeReferences(const std::filesystem::path& path)
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

		// Get the prefab node.
		const auto& prefabNode = data["Prefab"];

		// Set the prefab scene references.
		const auto& refs = prefabNode[1]["References"];
		if (refs)
		{
			const auto& sceneRefs = refs[0]["Scenes"];

			for (const auto& sceneRef : sceneRefs)
			{
				if (std::find(m_Context->m_SceneRefs.begin(), m_Context->m_SceneRefs.end(), sceneRef.as<UUID>()) != m_Context->m_SceneRefs.end())
				{
					continue;
				}

				m_Context->m_SceneRefs.push_back(sceneRef.as<UUID>());
			}
		}

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

	Scene* PrefabSerializer::GetScene()
	{
		if (m_Scene != nullptr)
		{
			return m_Scene;
		}

		return ResourceManager::Instance->GetScene(Context::ActiveScene);
	}
}
