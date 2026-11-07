#pragma once

#include "AssetManager.h"

#include "Serialization/AssetPack.h"
#include "Serialization/FileStreamReader.h"

namespace HBL2
{
	class RuntimeAssetManager final : public AssetManager
	{
	public:
		virtual ~RuntimeAssetManager() = default;

		virtual void Initialize(const AssetManagerSpecification& spec) override;

		virtual bool IsAssetValid(Handle<Asset> handle) override;
		virtual bool IsAssetLoaded(Handle<Asset> handle) override;

		virtual void RegisterAssets() override;
		virtual void DeregisterAssets() override;

		Handle<Asset> CreateAsset(UUID assetUUID, const AssetDescriptor&& desc);

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) override;
		virtual void UnloadAsset(Handle<Asset> handle) override;

	private:
		Handle<Texture> ImportTexture(Asset* asset, FileStreamReader& stream);
		Handle<Shader> ImportShader(Asset* asset, FileStreamReader& stream);
		Handle<Material> ImportMaterial(Asset* asset, FileStreamReader& stream);
		Handle<Mesh> ImportMesh(Asset* asset, FileStreamReader& stream);
		Handle<Scene> ImportScene(Asset* asset, FileStreamReader& stream);
		Handle<Sound> ImportSound(Asset* asset, FileStreamReader& stream);
		Handle<Prefab> ImportPrefab(Asset* asset, FileStreamReader& stream);
		Handle<Script> ImportScript(Asset* asset, FileStreamReader& stream);

		void UnloadTexture(Asset* asset);
		void UnloadShader(Asset* asset);
		void UnloadMesh(Asset* asset);
		void UnloadMaterial(Asset* asset);
		void UnloadScene(Asset* asset);
		void UnloadSound(Asset* asset);
		void UnloadPrefab(Asset* asset);
		void UnloadScript(Asset* asset);

	private:
		AssetPack m_AssetPack;
		std::filesystem::path m_AssetPackPath;
	};
}