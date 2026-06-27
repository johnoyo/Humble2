#pragma once

#include "AssetManager.h"

#include "Utilities/ShaderUtilities.h"
#include "Utilities/TextureUtilities.h"
#include "Utilities/MeshUtilities.h"

#include "Scene/SceneSerializer.h"
#include "Project/Project.h"
#include "Prefab/Prefab.h"

#include "Utilities/Collections/Span.h"

#include <yaml-cpp/yaml.h>

namespace HBL2
{
	class HBL2_API EditorAssetManager final : public AssetManager
	{
	public:
		virtual ~EditorAssetManager() = default;

		virtual bool IsAssetValid(Handle<Asset> handle) override;
		virtual bool IsAssetLoaded(Handle<Asset> handle) override;

		virtual void RegisterAssets() override;
		virtual void DeregisterAssets() override;

		Handle<Asset> CreateAsset(const AssetDescriptor&& desc);
		Handle<Asset> RegisterAsset(const std::filesystem::path& assetPath);

		UUID GetUUIDFromPath(const std::filesystem::path& assetPath);

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
			task->m_Finished.store(false, std::memory_order_release);

			// Load from scratch if the asset is not loaded.
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

		void SaveAsset(UUID assetUUID);
		void SaveAsset(Handle<Asset> handle);
		void DestroyAsset(Handle<Asset> handle);

	protected:
		virtual uint32_t LoadAsset(Handle<Asset> handle) override;
		virtual void UnloadAsset(Handle<Asset> handle) override;

	private:
		uint32_t ReloadAsset(Handle<Asset> handle);

		void CreateTextureMetadata(Asset* asset);
		void CreateShaderMetadata(Asset* asset);
		void CreateMaterialMetadata(Asset* asset);
		void CreateMeshMetadata(Asset* asset);
		void CreateSceneMetadata(Asset* asset);
		void CreateScriptMetadata(Asset* asset);
		void CreateSoundMetadata(Asset* asset);
		void CreatePrefabMetadata(Asset* asset);

		Handle<Texture> ImportTexture(Asset* asset);
		Handle<Shader> ImportShader(Asset* asset);
		Handle<Material> ImportMaterial(Asset* asset);
		Handle<Mesh> ImportMesh(Asset* asset);
		Handle<Scene> ImportScene(Asset* asset);
		Handle<Script> ImportScript(Asset* asset);
		Handle<Sound> ImportSound(Asset* asset);
		Handle<Prefab> ImportPrefab(Asset* asset);

		Handle<Shader> ReimportShader(Asset* asset);
		Handle<Material> ReimportMaterial(Asset* asset);
		Handle<Mesh> ReimportMesh(Asset* asset);
		Handle<Prefab> ReimportPrefab(Asset* asset);

		void SaveMaterial(Asset* asset);
		void SaveScene(Asset* asset);
		void SaveTexture(Asset* asset);
		void SaveScript(Asset* asset);
		void SaveSound(Asset* asset);
		void SavePrefab(Asset* asset);

		bool DestroyTexture(Asset* asset);
		bool DestroyShader(Asset* asset);
		bool DestroyMaterial(Asset* asset);
		bool DestroyMesh(Asset* asset);
		bool DestroyScript(Asset* asset);
		bool DestroyScene(Asset* asset);
		bool DestroySound(Asset* asset);
		bool DestroyPrefab(Asset* asset);

		void UnloadTexture(Asset* asset);
		void UnloadShader(Asset* asset);
		void UnloadMesh(Asset* asset);
		void UnloadMaterial(Asset* asset);
		void UnloadScript(Asset* asset);
		void UnloadScene(Asset* asset);
		void UnloadSound(Asset* asset);
		void UnloadPrefab(Asset* asset);
	};
}
