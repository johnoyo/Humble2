#include "PoolAccess.h"

#include "Asset/AssetManager.h"
#include "ResourceManager.h"

namespace HBL2
{
    void AcquireAsset(Handle<Asset> handle)
    {
        AssetManager::Instance->Acquire(handle);
    }

    void ReleaseAsset(Handle<Asset> handle)
    {
        AssetManager::Instance->Release(handle);
    }

    void AcquireResource(uint32_t packedHandle, ResourceType type)
    {
        ResourceManager::Instance->Acquire(packedHandle, type);
    }

    void ReleaseResource(uint32_t packedHandle, ResourceType type)
    {
        ResourceManager::Instance->Release(packedHandle, type);
    }
}
