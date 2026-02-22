#pragma once

#include "Base.h"
#include "Asset\Asset.h"
#include "Scene\Scene.h"
#include "Resources\Handle.h"
#include "Prefab\Prefab.h"

namespace HBL2
{
	class HBL2_API PrefabUtilities
	{
	public:
		PrefabUtilities(const PrefabUtilities&) = delete;

		static PrefabUtilities& Get();

		static void Initialize();
		static void Shutdown();

		void Unpack(Entity instantiatedPrefabEntity);
		void Save(Entity instantiatedPrefabEntity);
		void Revert(Entity instantiatedPrefabEntity);
		void Update(Asset* prefabAsset, Prefab* prefab, Scene* activeScene, Entity instantiatedPrefabEntity, bool checkVersion, bool preserveName);

		void ConvertEntityToPrefabPhase0(Entity entity, Scene* scene);
		bool ConvertEntityToPrefabPhase1(Entity entity, Handle<Asset> assetHandle, Scene* scene);

		void CreateMetadataFile(Handle<Asset> assetHandle, UUID baseEntityUUID, uint32_t version = 1);
		void CreateMetadataFile(Asset* prefabAsset, UUID baseEntityUUID, uint32_t version = 1);

	private:
		PrefabUtilities() = default;

		static PrefabUtilities* s_Instance;
	};
}