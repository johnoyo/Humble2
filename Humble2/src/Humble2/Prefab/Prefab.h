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
		uint32_t maxEntities = 4096;
		uint32_t maxComponents = 32;
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

		inline PrefabDescriptor& GetDescriptor() { return m_Descriptor; }
		inline const PrefabDescriptor& GetDescriptor() const { return m_Descriptor; }
		inline const Handle<Scene> GetSubSceneHandle() const { return m_SubSceneHandle; }

	private:
		static Entity CloneSourcePrefab(Prefab* prefab, Scene* activeScene);

	private:
		PrefabDescriptor m_Descriptor;

		Handle<Scene> m_SubSceneHandle;
		bool m_Loaded = false;
	};
}