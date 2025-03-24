#pragma once

#include "Asset.h"
#include "Resources\Handle.h"
#include "Resources\Pool.h"

#include "Renderer\Device.h"

#include "Utilities\JobSystem.h"

#include <exception>
#include <future>

namespace HBL2
{
	class Window;

	template<typename T>
	class ResourceTask
	{
	public:
		Handle<T> ResourceHandle = Handle<T>();
		inline const bool Finished() const { return m_Finished; }

	private:
		bool m_Finished = false;
		friend class AssetManager;
	};

	class HBL2_API AssetManager
	{
	public:
		static AssetManager* Instance;

		AssetManager() = default;
		virtual ~AssetManager() = default;

		Handle<Asset> CreateAsset(const AssetDescriptor&& desc)
		{
			auto handle = GetHandleFromUUID(std::hash<std::string>()(desc.filePath.string()));

			if (IsAssetValid(handle))
			{
				return handle;
			}

			handle = m_AssetPool.Insert(Asset(std::forward<const AssetDescriptor>(desc)));
			m_RegisteredAssets.push_back(handle);

			Asset* asset = GetAssetMetadata(handle);
			m_RegisteredAssetMap[asset->UUID] = handle;

			return handle;
		}
		void DeleteAsset(Handle<Asset> handle, bool destroy = false)
		{
			if (destroy)
			{
				if (DestroyAsset(handle))
				{
					Asset* asset = GetAssetMetadata(handle);
					if (asset != nullptr)
					{
						m_RegisteredAssetMap.erase(asset->UUID);
					}

					auto assetIterator = std::find(m_RegisteredAssets.begin(), m_RegisteredAssets.end(), handle);
				
					if (assetIterator != m_RegisteredAssets.end())
					{
						m_RegisteredAssets.erase(assetIterator);
					}

					m_AssetPool.Remove(handle);
				}
			}
			else
			{
				UnloadAsset(handle);
			}

		}
		Asset* GetAssetMetadata(Handle<Asset> handle) const
		{
			return m_AssetPool.Get(handle);
		}

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
		ResourceTask<T>* GetAssetAsync(UUID assetUUID)
		{
			return GetAssetAsync<T>(GetHandleFromUUID(assetUUID));
		}

		template<typename T>
		ResourceTask<T>* GetAssetAsync(Handle<Asset> assetHandle, JobContext* customJobCtx = nullptr)
		{
			// Do not schedule job if the asset handle is invalid.
			if (!IsAssetValid(assetHandle))
			{
				return nullptr;
			}

			ResourceTask<T>* task = new ResourceTask<T>();
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
				Device::Instance->SetContext(Window::Instance->GetWorkerHandle());
				task->ResourceHandle = GetAsset<T>(assetHandle);
				task->m_Finished = true;
				Device::Instance->SetContext(nullptr);
			});

			return task;
		}

		void WaitForAsyncJobs(JobContext* customJobCtx = nullptr)
		{
			JobContext& ctx = (customJobCtx == nullptr ? m_ResourceJobCtx : *customJobCtx);
			JobSystem::Get().Wait(ctx);
		}

		std::vector<Handle<Asset>>& GetRegisteredAssets() { return m_RegisteredAssets; }

		Handle<Asset> GetHandleFromUUID(UUID assetUUID)
		{
			Handle<Asset> assetHandle;

			if (m_RegisteredAssetMap.find(assetUUID) != m_RegisteredAssetMap.end())
			{
				assetHandle = m_RegisteredAssetMap[assetUUID];
			}

			return assetHandle;
		}

		void SaveAsset(UUID assetUUID)
		{
			return SaveAsset(GetHandleFromUUID(assetUUID));
		}

		virtual void SaveAsset(Handle<Asset> handle) = 0;

		virtual bool IsAssetValid(Handle<Asset> handle) = 0;
		virtual bool IsAssetLoaded(Handle<Asset> handle) = 0;

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) = 0;
		virtual void UnloadAsset(Handle<Asset> handle) = 0;
		virtual bool DestroyAsset(Handle<Asset> handle) = 0;

	private:
		JobContext m_ResourceJobCtx;
		Pool<Asset, Asset> m_AssetPool;
		std::unordered_map<UUID, Handle<Asset>> m_RegisteredAssetMap;
		std::vector<Handle<Asset>> m_RegisteredAssets;
	};
}
