#pragma once

#include "Asset.h"
#include "Resources\Handle.h"
#include "Resources\Pool.h"

#include "Renderer\Device.h"

#include "Core/Allocators.h"

#include "Utilities\JobSystem.h"
#include "Utilities/Collections/DynamicArray.h"
#include "Utilities/Collections/HashMap.h"
#include "Utilities/Collections/Collections.h"

#include "Utilities\Allocators\BinAllocator.h"

#include <concepts>

namespace HBL2
{
	class Window;

	template<typename T, typename F>
	concept AssetLoadCallback = std::invocable<F, Handle<T>> && std::same_as<std::invoke_result_t<F, Handle<T>>, void>;

	// Refactor in the future to this API:
	//	AssetManager::Instance->GetAssetAsync<Texture>(albedoMapAssetHandle)
	//		.Then([](Handle<Texture> h) {})
	//		.ThenOnMainThread([](Handle<Texture> h) {});
	template<typename T>
	class ResourceTask
	{
	public:
		Handle<T> ResourceHandle = Handle<T>();
		inline const bool Finished() const { return m_Finished.load(std::memory_order_acquire); }

	private:
		std::atomic_bool m_Finished = false;
		friend class AssetManager;
	};

	class HBL2_API AssetManager
	{
	public:
		static AssetManager* Instance;

		AssetManager() = default;
		virtual ~AssetManager() = default;

		void Initialize();

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
		
		// NOTE: The onLoadCallback will be called in the job thread, so any non thread safe code in the callback could be problematic.
		template<typename T, typename TCallback> requires AssetLoadCallback<T, TCallback>
		void GetAssetAsync(UUID assetUUID, TCallback&& onLoadCallback, JobContext* customJobCtx = nullptr)
		{
			GetAssetAsync<T>(GetHandleFromUUID(assetUUID), std::forward<TCallback>(onLoadCallback), customJobCtx);
		}

		// NOTE: The onLoadCallback will be called in the job thread, so any non thread safe code in the callback could be problematic.
		template<typename T, typename TCallback> requires AssetLoadCallback<T, TCallback>
		void GetAssetAsync(Handle<Asset> assetHandle, TCallback&& onLoadCallback, JobContext* customJobCtx = nullptr)
		{
			// Do not schedule job if the asset handle is invalid.
			if (!IsAssetValid(assetHandle))
			{
				return;
			}

			// Do not schedule job if the asset is loaded.
			if (IsAssetLoaded(assetHandle))
			{
				Handle<T> resourceHandle = GetAsset<T>(assetHandle);
				onLoadCallback(resourceHandle);
				return;
			}

			JobContext& ctx = (customJobCtx == nullptr ? m_ResourceJobCtx : *customJobCtx);

			JobSystem::Get().Execute(ctx, [this, assetHandle, cb = std::forward<TCallback>(onLoadCallback)]() mutable
			{
				Device::Instance->SetContext(ContextType::FETCH);

				Handle<T> resourceHandle = GetAsset<T>(assetHandle);
				cb(resourceHandle);

				Device::Instance->SetContext(ContextType::FLUSH_CLEAR);
			});
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

			ResourceTask<T>* task = Allocator::Persistent.Allocate<ResourceTask<T>>();
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

			ResourceTask<T>* task = Allocator::Persistent.Allocate<ResourceTask<T>>();
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
				}

				Device::Instance->SetContext(ContextType::FLUSH_CLEAR);
			});

			return task;
		}

		// NOTE: The onLoadCallback will be called in the job thread, so any non thread safe code in the callback could be problematic.
		template<typename T, typename TCallback> requires AssetLoadCallback<T, TCallback>
		void ReloadAssetAsync(UUID assetUUID, TCallback&& onLoadCallback, JobContext* customJobCtx = nullptr)
		{
			ReloadAssetAsync<T>(GetHandleFromUUID(assetUUID), std::forward<TCallback>(onLoadCallback), customJobCtx);
		}

		// NOTE: The onLoadCallback will be called in the job thread, so any non thread safe code in the callback could be problematic.
		template<typename T, typename TCallback> requires AssetLoadCallback<T, TCallback>
		void ReloadAssetAsync(Handle<Asset> assetHandle, TCallback&& onLoadCallback, JobContext* customJobCtx = nullptr)
		{
			// Do not schedule job if the asset handle is invalid.
			if (!IsAssetValid(assetHandle))
			{
				return;
			}

			// Load from scratch if the asset is not loaded.
			if (!IsAssetLoaded(assetHandle))
			{
				GetAssetAsync<T>(assetHandle, std::forward<TCallback>(onLoadCallback), customJobCtx);
				return;
			}

			JobContext& ctx = (customJobCtx == nullptr ? m_ResourceJobCtx : *customJobCtx);

			JobSystem::Get().Execute(ctx, [this, assetHandle, cb = std::forward<TCallback>(onLoadCallback)]() mutable
			{
				Device::Instance->SetContext(ContextType::FETCH);

				Handle<T> resourceHandle = ReloadAsset<T>(assetHandle);
				cb(resourceHandle);

				Device::Instance->SetContext(ContextType::FLUSH_CLEAR);
			});
		}

		void WaitForAsyncJobs(JobContext* customJobCtx = nullptr);

		Span<const Handle<Asset>> GetRegisteredAssets() { return { m_RegisteredAssets->data(), m_RegisteredAssets->size() }; }

		Handle<Asset> GetHandleFromUUID(UUID assetUUID);

		void SaveAsset(UUID assetUUID);

		virtual void SaveAsset(Handle<Asset> handle) = 0;
		virtual bool IsAssetValid(Handle<Asset> handle) = 0;
		virtual bool IsAssetLoaded(Handle<Asset> handle) = 0;

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) = 0;
		virtual uint32_t ReloadAsset(Handle<Asset> handle) = 0;
		virtual void UnloadAsset(Handle<Asset> handle) = 0;
		virtual bool DestroyAsset(Handle<Asset> handle) = 0;

	private:
		JobContext m_ResourceJobCtx;
		Pool<Asset, Asset> m_AssetPool;
		std::optional<HMap<UUID, Handle<Asset>>> m_RegisteredAssetMap;
		std::optional < DArray<Handle<Asset>>> m_RegisteredAssets;

		PoolReservation* m_Reservation = nullptr;
		Arena m_PoolArena;
	};
}
