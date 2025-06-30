#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include "Resources/Handle.h"
#include "Scene/Scene.h"

#include <entt.hpp>

namespace HBL2
{
	struct PrefabDescriptor
	{
		const char* debugName;
		UUID uuid;
		UUID baseEntityUUID;
		uint32_t version;
	};

	class HBL2_API Prefab
	{
	public:
		Prefab() = default;
		Prefab(const PrefabDescriptor&& desc);

		static entt::entity Instantiate(Handle<Asset> assetHandle);
		static entt::entity Instantiate(Handle<Asset> assetHandle, const glm::vec3& position);

		static void CreateMetadataFile(Handle<Asset> assetHandle, UUID baseEntityUUID);
		static void CreateMetadataFile(Asset* prefabAsset, UUID baseEntityUUID);

	private:
		static entt::entity Instantiate(Handle<Asset> assetHandle, Scene* scene);
		static entt::entity CloneSourcePrefab(Prefab* prefab);

		inline const UUID GetBaseEntityUUID() const { return m_BaseEntityUUID; }

	private:
		UUID m_UUID = 0;
		uint32_t m_Version = 0;
		UUID m_BaseEntityUUID = 0;
		std::vector<UUID> m_SceneRefs;
		std::vector<UUID> m_PrefabRefs;

		friend class PrefabSerializer;
		friend class SceneSerializer;
	};
}