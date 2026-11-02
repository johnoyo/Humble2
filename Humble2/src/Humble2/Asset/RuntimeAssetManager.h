#pragma once

#include "AssetManager.h"

#include "Serialization/AssetPack.h"

namespace HBL2
{
	class RuntimeAssetManager final : public AssetManager
	{
	public:
		virtual ~RuntimeAssetManager() = default;

		virtual bool IsAssetValid(Handle<Asset> handle) override;
		virtual bool IsAssetLoaded(Handle<Asset> handle) override;

		virtual void RegisterAssets() override;
		virtual void DeregisterAssets() override;

		Handle<Asset> CreateAsset(UUID assetUUID, const AssetDescriptor&& desc);

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) override;
		virtual void UnloadAsset(Handle<Asset> handle) override;

	private:
		AssetPack m_AssetPack;
	};
}