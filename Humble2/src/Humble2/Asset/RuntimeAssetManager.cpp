#include "RuntimeAssetManager.h"

#include "Serialization/AssetPackSerializer.h"
#include "Project/Project.h"

namespace HBL2
{
    bool RuntimeAssetManager::IsAssetValid(Handle<Asset> handle)
    {
        return handle.IsValid() && GetAssetMetadata(handle) != nullptr;
    }

    bool RuntimeAssetManager::IsAssetLoaded(Handle<Asset> handle)
    {
        if (!IsAssetValid(handle))
        {
            return false;
        }

        Asset* asset = GetAssetMetadata(handle);
        return asset->Loaded;
    }

    void RuntimeAssetManager::RegisterAssets()
    {
        // Load asset pack index.
        const auto& assetPackFilePath = Project::GetAssetDirectory() / "AssetPack.hblap";

        AssetPackSerializer serializer(&m_AssetPack);
        serializer.DeserializeIndex(assetPackFilePath);

        // Register all the assets from the asset pack.
        for (const auto& [assetUUID, assetInfo] : m_AssetPack.Assets)
        {
            AssetType assetType = (AssetType)assetInfo.Type;

            switch (assetType)
            {
            case AssetType::Texture:
                CreateAsset(assetUUID, { .debugName = "texture-asset", .type = AssetType::Texture, });
                break;
            case AssetType::Shader:
                CreateAsset(assetUUID, { .debugName = "shader-asset", .type = AssetType::Shader, });
                break;
            case AssetType::Material:
                CreateAsset(assetUUID, { .debugName = "material-asset", .type = AssetType::Material, });
                break;
            case AssetType::Scene:
                CreateAsset(assetUUID, { .debugName = "scene-asset", .type = AssetType::Scene, });
                break;
            case AssetType::Mesh:
                CreateAsset(assetUUID, { .debugName = "mesh-asset", .type = AssetType::Mesh, });
                break;
            case AssetType::Script:
                CreateAsset(assetUUID, { .debugName = "script-asset", .type = AssetType::Script, });
                break;
            case AssetType::Sound:
                CreateAsset(assetUUID, { .debugName = "sound-asset", .type = AssetType::Sound, });
                break;
            case AssetType::Prefab:
                CreateAsset(assetUUID, { .debugName = "prefab-asset", .type = AssetType::Prefab, });
                break;
            case AssetType::None:
                HBL2_CORE_ERROR("Aborting asset creation, asset type is None.");
                break;
            }                
        }
    }

    void RuntimeAssetManager::DeregisterAssets()
    {
        WaitForAsyncJobs();

        // Delete assets.
        for (const auto handle : m_RegisteredAssets)
        {
            DeleteAssetImmediate(handle);
        }

        // Clear asset handle caches.
        m_RegisteredAssets.clear();
        m_RegisteredAssetPathToUUIDMap.clear();
        m_RegisteredAssetMap.clear();
    }

    Handle<Asset> RuntimeAssetManager::CreateAsset(UUID assetUUID, const AssetDescriptor&& desc)
    {
        auto handle = GetHandleFromUUID(assetUUID);

        if (IsAssetValid(handle))
        {
            return handle;
        }

        handle = m_AssetPool.Insert(Asset(std::forward<const AssetDescriptor>(desc)));

        Asset* asset = GetAssetMetadata(handle);
        asset->UUID = assetUUID;

        m_RegisteredAssets.push_back(handle);
        m_RegisteredAssetMap[asset->UUID] = handle;

        return handle;
    }

    uint32_t RuntimeAssetManager::LoadAsset(Handle<Asset> handle)
    {
        return 0;
    }

    void RuntimeAssetManager::UnloadAsset(Handle<Asset> handle)
    {

    }
}
