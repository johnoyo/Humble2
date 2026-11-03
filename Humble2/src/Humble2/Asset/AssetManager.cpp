#include "AssetManager.h"

#include "Project/Project.h"
#include "Utilities/ShaderUtilities.h"
#include "Utilities/MeshUtilities.h"

namespace HBL2
{
	AssetManager* AssetManager::Instance = nullptr;

	void AssetManager::Dispatch()
	{
		{
			StaticFunction<void(void), 128> fn;
			while (m_MainThreadCallbacks.try_dequeue(fn))
			{
				fn();
			}
		}

		{
			StaticFunction<void(void), 64> fn;
			while (m_AssetDeleteCallbacks.try_dequeue(fn))
			{
				fn();
			}
		}
	}

	void AssetManager::Clean()
	{
		// Drain asset deletion queue.
		StaticFunction<void(void), 64> fn;
		while (m_AssetDeleteCallbacks.try_dequeue(fn))
		{
			fn();
		}

		DeregisterAssets();
	}

	const AssetManagerSpecification& AssetManager::GetSpec() const
	{
		return m_Spec;
	}

	const AssetManagerSpecification AssetManager::GetUsageStats()
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

		if (asset->Pinned)
		{
			m_AssetPool.Acquire(handle);
		}

		return handle;
	}

	void AssetManager::DeleteAsset(Handle<Asset> handle)
	{
		Asset* asset = GetAssetMetadata(handle);

		if (asset == nullptr)
		{
			return;
		}

		if (asset->Pinned)
		{
			Release(handle);
			asset->Pinned = false;

			return;
		}

		if (!m_AssetPool.IsAlive(handle))
		{
			m_AssetDeleteCallbacks.enqueue(StaticFunction<void(void), 64>([this, handle]()
			{
				UnloadAsset(handle);
			}));
		}
	}

	void AssetManager::DeleteAssetImmediate(Handle<Asset> handle)
	{
		Asset* asset = GetAssetMetadata(handle);

		if (asset == nullptr)
		{
			return;
		}

		if (asset->Pinned)
		{
			if (m_AssetPool.Release(handle))
			{
				UnloadAsset(handle);
			}
			asset->Pinned = false;

			return;
		}

		if (!m_AssetPool.IsAlive(handle))
		{
			UnloadAsset(handle);
		}
	}

	Asset* AssetManager::GetAssetMetadata(Handle<Asset> handle) const
	{
		return m_AssetPool.Get(handle);
	}

	void AssetManager::Acquire(Handle<Asset> handle)
	{
		m_AssetPool.Acquire(handle);
	}

	void AssetManager::Release(Handle<Asset> handle)
	{
		if (m_AssetPool.Release(handle))
		{
			m_AssetDeleteCallbacks.enqueue(StaticFunction<void(void), 64>([this, handle]()
			{
				UnloadAsset(handle);
                
                // Unloading an asset does not remove the asset from the pool.
                // So when we want to load the asset again, we will not be able to aquire a new ref
                // and it will cause a dangling reference, so we reopen the slot.
                m_AssetPool.Reopen(handle);
			}));
		}
	}

	void AssetManager::PinAsset(Handle<Asset> handle)
	{
		Asset* asset = GetAssetMetadata(handle);

		if (asset == nullptr)
		{
			return;
		}

		if (asset->Pinned)
		{
			return;
		}

		asset->Pinned = true;
		m_AssetPool.Acquire(handle);
	}

	void AssetManager::UnpinAsset(Handle<Asset> handle)
	{
		Asset* asset = GetAssetMetadata(handle);

		if (asset == nullptr)
		{
			return;
		}

		if (!asset->Pinned)
		{
			return;
		}

		asset->Pinned = false;
		m_AssetPool.Release(handle);
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
