#include "RuntimeAssetManager.h"

#include "Serialization/AssetPackSerializer.h"
#include "Project/Project.h"

namespace HBL2
{
    void RuntimeAssetManager::Initialize(const AssetManagerSpecification& spec)
    {
        m_Spec = spec;

        m_AssetPool.Initialize(m_Spec.Assets);

        size_t registeredAssetMapBytes = FixedHashMap<UUID, Handle<Asset>>::RequiredBytes(m_Spec.Assets);
        size_t assetPackAssetsMapBytes = FixedHashMap<UUID, AssetPack::AssetInfo>::RequiredBytes(m_Spec.Assets);

        uint64_t bytes = ArenaLayout::Create()
            .AddRaw(registeredAssetMapBytes, alignof(std::pair<UUID, Handle<Asset>>))
            .Add<Handle<Asset>>(m_Spec.Assets)
            .AddRaw(assetPackAssetsMapBytes, alignof(std::pair<UUID, AssetPack::AssetInfo>))
            .Total();

        m_Reservation = Allocator::Arena.Reserve("AssetManagerPool", bytes);
        m_PoolArena.Initialize(&Allocator::Arena, bytes, m_Reservation);

        m_RegisteredAssetMap = FixedHashMap<UUID, Handle<Asset>>(&m_PoolArena, m_Spec.Assets);
        m_RegisteredAssets = FixedArray<Handle<Asset>>(&m_PoolArena, m_Spec.Assets);

        m_AssetPack.Assets = FixedHashMap<UUID, AssetPack::AssetInfo>(&m_PoolArena, m_Spec.Assets);

        m_AssetPackPath = Project::GetAssetDirectory() / "AssetPack.hblap";
    }

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
        // Load asset pack file.
        AssetPackSerializer serializer(&m_AssetPack);
        serializer.Deserialize(m_AssetPackPath);

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
        Asset* asset = GetAssetMetadata(handle);
        if (asset == nullptr)
        {
            return 0;
        }

        auto& assetInfo = m_AssetPack.Assets[asset->UUID];

        FileStreamReader stream(m_AssetPackPath);
        stream.SetStreamPosition(assetInfo.PackedOffset);

        switch (asset->Type)
        {
        case AssetType::Texture:
            asset->Indentifier = ImportTexture(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Shader:
            asset->Indentifier = ImportShader(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Material:
            asset->Indentifier = ImportMaterial(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Mesh:
            asset->Indentifier = ImportMesh(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Scene:
            asset->Indentifier = ImportScene(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Script:
            asset->Indentifier = ImportScript(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Sound:
            asset->Indentifier = ImportSound(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::Prefab:
            asset->Indentifier = ImportPrefab(asset, stream).Pack();
            asset->Loaded = (asset->Indentifier != 0);
            return asset->Indentifier;
        case AssetType::None:
            return 0;
        }

        return 0;
    }

    void RuntimeAssetManager::UnloadAsset(Handle<Asset> handle)
    {

    }

    Handle<Texture> RuntimeAssetManager::ImportTexture(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Texture>();
    }

    Handle<Shader> RuntimeAssetManager::ImportShader(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Shader>();
    }

    Handle<Material> RuntimeAssetManager::ImportMaterial(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Material>();
    }

    Handle<Mesh> RuntimeAssetManager::ImportMesh(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Mesh>();
    }

    Handle<Scene> RuntimeAssetManager::ImportScene(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Scene>();
    }

    Handle<Script> RuntimeAssetManager::ImportScript(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Script>();
    }

    Handle<Sound> RuntimeAssetManager::ImportSound(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Sound>();
    }

    Handle<Prefab> RuntimeAssetManager::ImportPrefab(Asset* asset, FileStreamReader& stream)
    {
        return Handle<Prefab>();
    }
}
