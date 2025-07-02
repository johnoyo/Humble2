#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include "Resources/Handle.h"
#include "Scene/Scene.h"

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

		static Entity Instantiate(Handle<Asset> assetHandle);
		static Entity Instantiate(Handle<Asset> assetHandle, const glm::vec3& position);

		static void Unpack(Entity instantiatedPrefabEntity);
		static void Save(Entity instantiatedPrefabEntity);
		static void Destroy(Entity instantiatedPrefabEntity);

		static void CreateMetadataFile(Handle<Asset> assetHandle, UUID baseEntityUUID, uint32_t version = 1);
		static void CreateMetadataFile(Asset* prefabAsset, UUID baseEntityUUID, uint32_t version = 1);

	private:
		static Entity Instantiate(Handle<Asset> assetHandle, Scene* scene);
		static Entity CloneSourcePrefab(Prefab* prefab, Scene* activeScene);

		inline const UUID GetBaseEntityUUID() const { return m_BaseEntityUUID; }

	private:
		UUID m_UUID = 0;
		uint32_t m_Version = 0;
		UUID m_BaseEntityUUID = 0;

		friend class PrefabSerializer;
		friend class SceneSerializer;
	};
}