#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include "Resources/Handle.h"

#include <entt.hpp>

namespace HBL2
{
	struct HBL2_API PrefabDescriptor
	{
		const char* debugName;
		UUID baseEntityUUID;
	};

	class HBL2_API Prefab
	{
	public:
		Prefab() = default;
		Prefab(const PrefabDescriptor&& desc)
		{
			m_BaseEntityUUID = desc.baseEntityUUID;
		}

		inline const UUID GetBaseEntityUUID() const { return m_BaseEntityUUID; }

		static void CreateMetadataFile(Handle<Asset> assetHandle, UUID baseEntityUUID);
		static void CreateMetadataFile(Asset* prefabAsset, UUID baseEntityUUID);

	private:
		UUID m_BaseEntityUUID = 0;
		std::vector<UUID> m_SceneRefs;
		std::vector<UUID> m_PrefabRefs;

		friend class PrefabSerializer;
	};
}