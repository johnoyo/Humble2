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

		void Load(const PrefabDescriptor&& desc);
		void Unload();

		static Entity Instantiate(Handle<Asset> assetHandle);
		static Entity Instantiate(Handle<Asset> assetHandle, const glm::vec3& position);

		static Entity Instantiate(Handle<Prefab> prefabHandle);
		static Entity Instantiate(Handle<Prefab> prefabHandle, const glm::vec3& position);

		static void Destroy(Entity instantiatedPrefabEntity);

		inline const UUID GetUUID() const { return m_UUID; }
		inline const uint32_t GetVersion() const { return m_Version; }
		inline const UUID GetBaseEntityUUID() const { return m_BaseEntityUUID; }
		inline const Handle<Scene> GetSubSceneHandle() const { return m_SubSceneHandle; }

	private:
		static Entity CloneSourcePrefab(Prefab* prefab, Scene* activeScene);

	private:
		UUID m_UUID = 0;
		uint32_t m_Version = 0;
		UUID m_BaseEntityUUID = 0;
		Handle<Scene> m_SubSceneHandle;
		bool m_Loaded = false;

		friend class PrefabSerializer;
		friend class SceneSerializer;
	};
}