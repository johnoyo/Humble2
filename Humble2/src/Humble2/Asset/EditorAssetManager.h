#pragma once

#include "AssetManager.h"
#include "AssetImporter.h"

namespace HBL2
{
	class EditorAssetManager final : public AssetManager
	{
	public:
		virtual ~EditorAssetManager() = default;

		virtual uint32_t LoadAsset(Handle<Asset> handle) override;
		virtual void DestroyAsset(Handle<Asset> handle) override;
		virtual void SaveAsset(Handle<Asset> handle) override;
		virtual bool IsAssetValid(Handle<Asset> handle) override;
		virtual bool IsAssetLoaded(Handle<Asset> handle) override;
	};
}