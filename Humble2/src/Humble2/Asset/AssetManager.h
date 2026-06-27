#pragma once

#include "Asset.h"
#include "Resources/Handle.h"
#include "Resources/Pool.h"

#include "Renderer/Device.h"

#include "Core/Allocators.h"

#include "Utilities/JobSystem.h"
#include "Utilities/Allocators/PoolArena.h"
#include "Utilities/Collections/Collections.h"
#include "Utilities/Collections/StaticFunction.h"

#include <moodycamel/concurrentqueue.h>

namespace HBL2
{
	class Window;
	class EditorAssetManager;

	template<typename T>
	class ResourceTask
	{
	public:
		Handle<T> ResourceHandle = Handle<T>();

		ResourceTask<T>* Then(StaticFunction<void(Handle<T>), 64>&& workerThreadCb)
		{
			m_WorkerThreadCallback = std::move(workerThreadCb);
			return this;
		}

		void ThenOnMainThread(StaticFunction<void(Handle<T>), 64>&& mainThreadCb)
		{
			m_MainThreadCallback = std::move(mainThreadCb);			
		}

		inline const bool Finished() const { return m_Finished.load(std::memory_order_acquire); }

	private:
		friend class AssetManager;
		friend class EditorAssetManager;

		std::atomic_bool m_Finished = false;
		StaticFunction<void(Handle<T>), 64> m_WorkerThreadCallback;
		StaticFunction<void(Handle<T>), 64> m_MainThreadCallback;
	};

	struct AssetManagerSpecification
	{
		uint32_t Assets = 1024;
	};

	class HBL2_API AssetManager
	{
	public:
		static AssetManager* Instance;

		AssetManager() = default;
		virtual ~AssetManager() = default;

		void Initialize(const AssetManagerSpecification& spec);
		void Dispatch();
		void Clean();

		const AssetManagerSpecification& GetSpec() const;
		const AssetManagerSpecification& GetUsageStats();

		Handle<Asset> CreateMemoryOnlyAsset(const MemoryOnlyAssetDescriptor&& desc);
		void DeleteAsset(Handle<Asset> handle);
		Asset* GetAssetMetadata(Handle<Asset> handle) const;		

		template<typename T>
		Handle<T> GetAsset(UUID assetUUID)
		{
			return GetAsset<T>(GetHandleFromUUID(assetUUID));
		}
		
		template<typename T>
		Handle<T> GetAsset(Handle<Asset> handle)
		{
			if (!IsAssetValid(handle))
			{
				return Handle<T>();
			}

			Asset* asset = GetAssetMetadata(handle);
			if (asset->Loaded)
			{
				return Handle<T>::UnPack(asset->Indentifier);
			}
			else
			{
				uint32_t packedHandle = LoadAsset(handle);
				return Handle<T>::UnPack(packedHandle);
			}

			return Handle<T>();
		}

		template<typename T>
		ResourceTask<T>* GetAssetAsync(UUID assetUUID, JobContext* customJobCtx = nullptr)
		{
			return GetAssetAsync<T>(GetHandleFromUUID(assetUUID), customJobCtx);
		}

		template<typename T>
		ResourceTask<T>* GetAssetAsync(Handle<Asset> assetHandle, JobContext* customJobCtx = nullptr)
		{
			// Do not schedule job if the asset handle is invalid.
			if (!IsAssetValid(assetHandle))
			{
				return nullptr;
			}

			ResourceTask<T>* task = m_ResourceTaskPoolArena.AllocConstruct<ResourceTask<T>>();
			task->m_Finished.store(false, std::memory_order_release);

			// Do not schedule job if the asset is loaded.
			if (IsAssetLoaded(assetHandle))
			{
				task->ResourceHandle = GetAsset<T>(assetHandle);
				task->m_Finished.store(true, std::memory_order_release);

				if (task->m_WorkerThreadCallback)
				{
					task->m_WorkerThreadCallback(task->ResourceHandle);
				}

				if (task->m_MainThreadCallback)
				{
					m_MainThreadCallbacks.enqueue(
						StaticFunction<void(void), 128>([cb = std::move(task->m_MainThreadCallback), handle = task->ResourceHandle]() mutable
						{
							cb(handle);
						}));
				}

				return task;
			}

			JobContext& ctx = (customJobCtx == nullptr ? m_ResourceJobCtx : *customJobCtx);

			JobSystem::Get().Execute(ctx, [this, assetHandle, task]()
			{
				Device::Instance->SetContext(ContextType::FETCH);

				// NOTE: Keep an eye here, it may cause problems if we still load an asset while we change scenes!
				if (task != nullptr)
				{
					task->ResourceHandle = GetAsset<T>(assetHandle);
					task->m_Finished.store(true, std::memory_order_release);

					if (task->m_WorkerThreadCallback)
					{
						task->m_WorkerThreadCallback(task->ResourceHandle);
					}

					if (task->m_MainThreadCallback)
					{
						m_MainThreadCallbacks.enqueue(
							StaticFunction<void(void), 128>([cb = std::move(task->m_MainThreadCallback), handle = task->ResourceHandle]() mutable
							{
								cb(handle);
							}));
					}
				}

				Device::Instance->SetContext(ContextType::FLUSH_CLEAR);
			});

			return task;
		}

		void WaitForAsyncJobs(JobContext* customJobCtx = nullptr);

		template<typename T>
		void ReleaseResourceTask(ResourceTask<T>* task)
		{
			m_ResourceTaskPoolArena.Free(task);
		}

		Span<const Handle<Asset>> GetRegisteredAssets() { return { m_RegisteredAssets.data(), m_RegisteredAssets.size() }; }

        Handle<Asset> GetHandleFromUUID(UUID assetUUID);

		virtual void RegisterAssets() = 0;
		virtual void DeregisterAssets() = 0;
		virtual bool IsAssetValid(Handle<Asset> handle) = 0;
		virtual bool IsAssetLoaded(Handle<Asset> handle) = 0;

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) = 0;
		virtual void UnloadAsset(Handle<Asset> handle) = 0;

		AssetManagerSpecification m_Spec;

		Pool<Asset, Asset> m_AssetPool;

		PoolReservation* m_Reservation = nullptr;
		Arena m_PoolArena;
		PoolArena m_ResourceTaskPoolArena;

		JobContext m_ResourceJobCtx;

        HMap<UUID, Handle<Asset>> m_RegisteredAssetMap = MakeEmptyHMap<UUID, Handle<Asset>>();
		HMap<std::filesystem::path, UUID> m_RegisteredAssetPathToUUIDMap = MakeEmptyHMap<std::filesystem::path, UUID>();
		DArray<Handle<Asset>> m_RegisteredAssets = MakeEmptyDArray<Handle<Asset>>();

		moodycamel::ConcurrentQueue<StaticFunction<void(void), 128>> m_MainThreadCallbacks;
	};
}
