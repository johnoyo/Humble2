#pragma once

#include "Asset.h"
#include "Resources\Handle.h"
#include "Resources\Pool.h"

#include <exception>
#include <future>

namespace HBL2
{
	class HBL2_API AssetManager
	{
	public:
		static AssetManager* Instance;

		AssetManager() = default;
		virtual ~AssetManager() = default;

		Handle<Asset> CreateAsset(const AssetDescriptor&& desc)
		{
			auto handle = m_AssetPool.Insert(Asset(std::forward<const AssetDescriptor>(desc)));
			m_RegisteredAssets.push_back(handle);

			Asset* asset = GetAssetMetadata(handle);
			m_RegisteredAssetMap[asset->UUID] = handle;

			return handle;
		}
		void DeleteAsset(Handle<Asset> handle, bool destroy = false)
		{
			// NOTE(John): Maybe consider removing the handle from m_RegisteredAssets and m_RegisteredAssetMap??
			if (destroy)
			{
				DestroyAsset(handle);
			}
			else
			{
				UnloadAsset(handle);
			}

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
			return GetAsset<T>(GetHandleFromUUID(assetUUID));
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

		virtual void UnloadAsset(Handle<Asset> handle) = 0;

		virtual bool IsAssetValid(Handle<Asset> handle) = 0;
		virtual bool IsAssetLoaded(Handle<Asset> handle) = 0;

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) = 0;
		virtual void DestroyAsset(Handle<Asset> handle) = 0;

	private:
		Pool<Asset, Asset> m_AssetPool;
		std::unordered_map<UUID, Handle<Asset>> m_RegisteredAssetMap;
		std::vector<Handle<Asset>> m_RegisteredAssets;
	};
}
