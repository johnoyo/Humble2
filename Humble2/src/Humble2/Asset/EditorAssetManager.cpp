#include "EditorAssetManager.h"

namespace HBL2
{
    uint32_t EditorAssetManager::LoadAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);
        return AssetImporter::Get().ImportAsset(asset);
    }

    bool EditorAssetManager::IsAssetValid(Handle<Asset> handle)
    {
        return handle.IsValid();
    }

    bool EditorAssetManager::IsAssetLoaded(Handle<Asset> handle)
    {
        if (!handle.IsValid())
        {
            return false;
        }

        Asset* asset = GetAssetMetadata(handle);
        return asset->Loaded;
    }
}
