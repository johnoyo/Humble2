#pragma once

#include "Asset.h"
#include "Resources\Handle.h"
#include "Resources\Pool.h"

#include <exception>
#include <future>

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
			m_AssetPool.Remove(handle);
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
		Handle<T> GetAssetAsync(UUID assetUUID)
		{
			// TODO
		}

		template<typename T>
		Handle<T> GetAssetAsync(Handle<Asset> assetHandle)
		{
			// TODO
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
