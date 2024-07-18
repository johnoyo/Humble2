#pragma once

#include "Handle.h"
#include "Pool.h"
#include "Types.h"
#include "TypeDescriptors.h"

namespace HBL2
{
	class AssetManager
	{
	public:
		static inline AssetManager* Instance;

		virtual ~AssetManager() = default;

		Handle<Asset> CreateAsset(const AssetDescriptor&& desc)
		{
			return m_AssetPool.Insert(Asset(desc));
		}
		void DeleteAsset(Handle<Asset> handle)
		{
			m_AssetPool.Remove(handle);
		}
		Asset* GetAsset(Handle<Asset> handle) const
		{
			return m_AssetPool.Get(handle);
		}
		
		virtual bool IsAssetValid(Handle<Asset> handle) = 0;
		virtual bool IsAssetLoaded(Handle<Asset> handle) = 0;

	private:
		Pool<Asset, Asset> m_AssetPool;
	};
}
