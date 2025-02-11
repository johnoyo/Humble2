#include "EditorAssetManager.h"

namespace HBL2
{
    uint32_t EditorAssetManager::LoadAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);
        return AssetImporter::Get().ImportAsset(asset);
    }

    void EditorAssetManager::UnloadAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);
        return AssetImporter::Get().UnloadAsset(asset);
    }

    void EditorAssetManager::DestroyAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);
        AssetImporter::Get().DestroyAsset(asset);
    }

    void EditorAssetManager::SaveAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);
        AssetImporter::Get().SaveAsset(asset);
    }

    bool EditorAssetManager::IsAssetValid(Handle<Asset> handle)
    {
        return handle.IsValid() && GetAssetMetadata(handle) != nullptr;
    }

    bool EditorAssetManager::IsAssetLoaded(Handle<Asset> handle)
    {
        if (!IsAssetValid(handle))
        {
            return false;
        }

        Asset* asset = GetAssetMetadata(handle);
        return asset->Loaded;
    }
}
