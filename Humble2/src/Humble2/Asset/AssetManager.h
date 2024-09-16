#pragma once

#include "Asset.h"
#include "Resources\Handle.h"
#include "Resources\Pool.h"

namespace HBL2
{
	class AssetManager
	{
	public:
		static inline AssetManager* Instance;

		AssetManager() = default;
		virtual ~AssetManager() = default;

		Handle<Asset> CreateAsset(const AssetDescriptor&& desc)
		{
			auto handle = m_AssetPool.Insert(Asset(std::forward<const AssetDescriptor>(desc)));
			m_RegisteredAssets.push_back(handle);
			return handle;
		}
		void DeleteAsset(Handle<Asset> handle)
		{
			for (int i = 0; i < m_RegisteredAssets.size(); i++)
			{
				if (handle == m_RegisteredAssets[i])
				{
					m_RegisteredAssets.erase(m_RegisteredAssets.begin() + i);
					break;
				}
			}
			m_AssetPool.Remove(handle);
		}
		Asset* GetAssetMetadata(Handle<Asset> handle) const
		{
			return m_AssetPool.Get(handle);
		}

		template<typename T>
		Handle<T> GetAsset(UUID assetUUID)
		{
			Handle<Asset> assetHandle;

			for (auto handle : m_RegisteredAssets)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->UUID == assetUUID)
				{
					assetHandle = handle;
					break;
				}
			}			

			return GetAsset<T>(assetHandle);
		}
		
		template<typename T>
		Handle<T> GetAsset(Handle<Asset> handle)
		{
			if (!handle.IsValid())
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
		Handle<T> ReloadAsset(UUID assetUUID)
		{
			Handle<Asset> assetHandle;

			for (auto handle : m_RegisteredAssets)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->UUID == assetUUID)
				{
					assetHandle = handle;
					break;
				}
			}

			return ReloadAsset<T>(assetHandle);
		}

		template<typename T>
		Handle<T> ReloadAssetFromResourceHandle(Handle<T> handle)
		{
			if (!handle.IsValid())
			{
				return Handle<T>();
			}

			Handle<Asset> assetHandle;

			for (auto handle : m_RegisteredAssets)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Indentifier == handle.Pack())
				{
					assetHandle = handle;
					break;
				}
			}

			uint32_t packedHandle = LoadAsset(assetHandle);
			return Handle<T>::UnPack(packedHandle);
		}

		template<typename T>
		Handle<T> ReloadAsset(Handle<Asset> handle)
		{
			if (!handle.IsValid())
			{
				return Handle<T>();
			}

			uint32_t packedHandle = LoadAsset(handle);
			return Handle<T>::UnPack(packedHandle);
		}

		std::vector<Handle<Asset>>& GetRegisteredAssets() { return m_RegisteredAssets; }

		void SaveAsset(UUID assetUUID)
		{
			Handle<Asset> assetHandle;

			for (auto handle : m_RegisteredAssets)
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->UUID == assetUUID)
				{
					assetHandle = handle;
					break;
				}
			}

			return SaveAsset(assetHandle);
		}

		virtual void SaveAsset(Handle<Asset> handle) = 0;
		virtual bool IsAssetValid(Handle<Asset> handle) = 0;
		virtual bool IsAssetLoaded(Handle<Asset> handle) = 0;

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) = 0;

	private:
		Pool<Asset, Asset> m_AssetPool;
		std::vector<Handle<Asset>> m_RegisteredAssets;
	};
}
