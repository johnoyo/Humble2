#include "AssetManager.h"

#include "Project/Project.h"
#include "Utilities/ShaderUtilities.h"
#include "Utilities/MeshUtilities.h"

namespace HBL2
{
	AssetManager* AssetManager::Instance = nullptr;

	void AssetManager::Initialize(const AssetManagerSpecification& spec)
	{
		m_Spec = spec;

		m_AssetPool.Initialize(m_Spec.Assets);

		uint32_t byteSize = (uint32_t)Allocator::CalculateSoAByteSize<std::filesystem::path, UUID, UUID, Handle<Asset>, Handle<Asset>>(2 * m_Spec.Assets) * 2;
		constexpr size_t resourceTasksByteSize = 192_B * 1024;
		constexpr size_t resourceTasksReserveBytes = resourceTasksByteSize * 2;

		m_Reservation = Allocator::Arena.Reserve("AssetManagerPool", byteSize + resourceTasksReserveBytes);
		m_PoolArena.Initialize(&Allocator::Arena, byteSize, m_Reservation);
		m_ResourceTaskPoolArena.Initialize(&Allocator::Arena, resourceTasksByteSize, 192, m_Reservation);

        m_RegisteredAssetMap = MakeHMap<UUID, Handle<Asset>>(m_PoolArena, m_Spec.Assets);
		m_RegisteredAssetPathToUUIDMap = MakeHMap<std::filesystem::path, UUID>(m_PoolArena, m_Spec.Assets);
		m_RegisteredAssets = MakeDArray<Handle<Asset>>(m_PoolArena, m_Spec.Assets);
	}

	void AssetManager::Dispatch()
	{
		StaticFunction<void(void), 128> fn;
		while (m_MainThreadCallbacks.try_dequeue(fn))
		{
			fn();
		}
	}

	void AssetManager::Clean()
	{
		DeregisterAssets();
	}

	const AssetManagerSpecification& AssetManager::GetSpec() const
	{
		return m_Spec;
	}

	const AssetManagerSpecification& AssetManager::GetUsageStats()
	{
		AssetManagerSpecification currentSpec =
		{
			.Assets = m_AssetPool.FreeSlotCount(),
		};

		return currentSpec;
	}

	Handle<Asset> AssetManager::CreateMemoryOnlyAsset(const MemoryOnlyAssetDescriptor&& desc)
	{
		Handle<Asset> handle = m_AssetPool.Insert(Asset(std::forward<const MemoryOnlyAssetDescriptor>(desc)));

		m_RegisteredAssets.push_back(handle);

		Asset* asset = GetAssetMetadata(handle);
		m_RegisteredAssetMap[asset->UUID] = handle;

		return handle;
	}

	void AssetManager::DeleteAsset(Handle<Asset> handle)
	{
		UnloadAsset(handle);
	}

	Asset* AssetManager::GetAssetMetadata(Handle<Asset> handle) const
	{
		return m_AssetPool.Get(handle);
	}

	void AssetManager::WaitForAsyncJobs(JobContext* customJobCtx)
	{
		JobContext& ctx = (customJobCtx == nullptr ? m_ResourceJobCtx : *customJobCtx);
		JobSystem::Get().Wait(ctx);
	}

	Handle<Asset> AssetManager::GetHandleFromUUID(UUID assetUUID)
	{
		Handle<Asset> assetHandle;

		auto it = m_RegisteredAssetMap.find(assetUUID);
		if (it != m_RegisteredAssetMap.end())
		{
			assetHandle = it->second;
		}

		return assetHandle;
	}
}
