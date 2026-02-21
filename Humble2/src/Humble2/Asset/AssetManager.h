#pragma once

#include "Asset.h"
#include "Resources\Handle.h"
#include "Resources\Pool.h"

#include "Renderer\Device.h"

#include "Core\Allocators.h"

#include "Utilities\JobSystem.h"
#include "Utilities\Allocators\PoolArena.h"
#include "Utilities\Collections\Collections.h"
#include "Utilities\Collections\StaticFunction.h"

#include "Vendor\moodycamel\concurrentqueue.h"

namespace HBL2
{
	class Window;

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

		const AssetManagerSpecification& GetSpec() const;
		const AssetManagerSpecification& GetUsageStats();

		Handle<Asset> CreateAsset(const AssetDescriptor&& desc);
		void DeleteAsset(Handle<Asset> handle, bool destroy = false);
		Asset* GetAssetMetadata(Handle<Asset> handle) const;

		void RegisterAssets();
		Handle<Asset> RegisterAsset(const std::filesystem::path& assetPath);
		void DeregisterAssets();

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
			task->m_Finished = false;

			// Do not schedule job if the asset is loaded.
			if (IsAssetLoaded(assetHandle))
			{
				task->ResourceHandle = GetAsset<T>(assetHandle);
				task->m_Finished = true;
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

		template<typename T>
		Handle<T> ReloadAsset(UUID assetUUID)
		{
			return ReloadAsset<T>(GetHandleFromUUID(assetUUID));
		}

		template<typename T>
		Handle<T> ReloadAsset(Handle<Asset> handle)
		{
			if (!IsAssetValid(handle))
			{
				return Handle<T>();
			}

			Asset* asset = GetAssetMetadata(handle);
			if (asset->Loaded)
			{
				uint32_t packedHandle = ReloadAsset(handle);
				return Handle<T>::UnPack(packedHandle);
			}
			else
			{
				uint32_t packedHandle = LoadAsset(handle);
				return Handle<T>::UnPack(packedHandle);
			}

			return Handle<T>();
		}

		template<typename T>
		ResourceTask<T>* ReloadAssetAsync(UUID assetUUID, JobContext* customJobCtx = nullptr)
		{
			return ReloadAssetAsync<T>(GetHandleFromUUID(assetUUID), customJobCtx);
		}

		template<typename T>
		ResourceTask<T>* ReloadAssetAsync(Handle<Asset> assetHandle, JobContext* customJobCtx = nullptr)
		{
			// Do not schedule job if the asset handle is invalid.
			if (!IsAssetValid(assetHandle))
			{
				return nullptr;
			}

			ResourceTask<T>* task = m_ResourceTaskPoolArena.AllocConstruct<ResourceTask<T>>();
			task->m_Finished = false;

			// Load from scratch if the asset is loaded.
			if (!IsAssetLoaded(assetHandle))
			{
				return GetAssetAsync<T>(assetHandle, customJobCtx);
			}

			JobContext& ctx = (customJobCtx == nullptr ? m_ResourceJobCtx : *customJobCtx);

			JobSystem::Get().Execute(ctx, [this, assetHandle, task]()
			{
				Device::Instance->SetContext(ContextType::FETCH);

				// NOTE: Keep an eye here, it may cause problems if we still reload an asset while we change scenes!
				if (task != nullptr)
				{
					task->ResourceHandle = ReloadAsset<T>(assetHandle);
					task->m_Finished.store(true, std::memory_order_release);

					if (task->m_WorkerThreadCallback)
					{
						task->m_WorkerThreadCallback(task->ResourceHandle);
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

		void SaveAsset(UUID assetUUID);

		virtual void SaveAsset(Handle<Asset> handle) = 0;
		virtual bool IsAssetValid(Handle<Asset> handle) = 0;
		virtual bool IsAssetLoaded(Handle<Asset> handle) = 0;

	protected:
		AssetManagerSpecification m_Spec;

		virtual uint32_t LoadAsset(Handle<Asset> handle) = 0;
		virtual uint32_t ReloadAsset(Handle<Asset> handle) = 0;
		virtual void UnloadAsset(Handle<Asset> handle) = 0;
		virtual bool DestroyAsset(Handle<Asset> handle) = 0;

	private:
		JobContext m_ResourceJobCtx;
		Pool<Asset, Asset> m_AssetPool;

		PoolReservation* m_Reservation = nullptr;
		Arena m_PoolArena;
		PoolArena m_ResourceTaskPoolArena;

		HMap<UUID, Handle<Asset>> m_RegisteredAssetMap = MakeEmptyHMap<UUID, Handle<Asset>>();
		DArray<Handle<Asset>> m_RegisteredAssets = MakeEmptyDArray<Handle<Asset>>();

		moodycamel::ConcurrentQueue<StaticFunction<void(void), 128>> m_MainThreadCallbacks;
	};
}
