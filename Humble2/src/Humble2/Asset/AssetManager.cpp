#include "AssetManager.h"

#include "Project\Project.h"
#include "Utilities/ShaderUtilities.h"
#include "Utilities/MeshUtilities.h"

namespace HBL2
{
	AssetManager* AssetManager::Instance = nullptr;

	void AssetManager::Initialize(const AssetManagerSpecification& spec)
	{
		m_Spec = spec;

		m_AssetPool.Initialize(m_Spec.Assets);

		uint32_t byteSize = Allocator::CalculateSoAByteSize<UUID, Handle<Asset>, Handle<Asset>>(2 * m_Spec.Assets) * 2;
		constexpr size_t resourceTasksReserveBytes = 16_KB;
		constexpr size_t resourceTasksByteSize = 6_B * 1024;

		m_Reservation = Allocator::Arena.Reserve("AssetManagerPool", byteSize + resourceTasksReserveBytes);
		m_PoolArena.Initialize(&Allocator::Arena, byteSize, m_Reservation);
		m_ResourceTaskPoolArena.Initialize(&Allocator::Arena, resourceTasksByteSize, 6, m_Reservation);

		m_RegisteredAssetMap = MakeHMap<UUID, Handle<Asset>>(m_PoolArena, m_Spec.Assets);
		m_RegisteredAssets = MakeDArray<Handle<Asset>>(m_PoolArena, m_Spec.Assets);
	}

	const AssetManagerSpecification& AssetManager::GetSpec() const
	{
		return m_Spec;
	}

	const AssetManagerSpecification& AssetManager::GetUsageStats()
	{
		AssetManagerSpecification currentSpec =
		{
			.Assets = m_AssetPool.FreeSlotCount(),
		};

		return currentSpec;
	}

	Handle<Asset> AssetManager::CreateAsset(const AssetDescriptor&& desc)
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

	void AssetManager::DeleteAsset(Handle<Asset> handle, bool destroy)
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

	Asset* AssetManager::GetAssetMetadata(Handle<Asset> handle) const
	{
		return m_AssetPool.Get(handle);
	}

	void AssetManager::RegisterAssets()
	{
		// Create Asset directory if it does not exist.
		if (!std::filesystem::is_directory(HBL2::Project::GetAssetDirectory()))
		{
			try
			{
				std::filesystem::create_directories(HBL2::Project::GetAssetDirectory());
			}
			catch (std::exception& e)
			{
				HBL2_CORE_ERROR("Project directory creation failed: {0}", e.what());
				return;
			}
		}

		// Register assets that are inside the Assets directory.
		for (auto& entry : std::filesystem::recursive_directory_iterator(HBL2::Project::GetAssetDirectory()))
		{
			RegisterAsset(entry.path());
		}
	}

	Handle<Asset> AssetManager::RegisterAsset(const std::filesystem::path& assetPath)
	{
		const std::string& extension = assetPath.extension().string();
		auto relativePath = std::filesystem::relative(assetPath, HBL2::Project::GetAssetDirectory());

		Handle<Asset> assetHandle;

		if (extension == ".png" || extension == ".jpg" || extension == ".tga" || extension == ".hdr")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "texture-asset",
				.filePath = relativePath,
				.type = AssetType::Texture,
			});
		}
		else if (extension == ".obj" || extension == ".gltf" || extension == ".glb" || extension == ".fbx" || extension == ".FBX")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "mesh-asset",
				.filePath = relativePath,
				.type = AssetType::Mesh,
			});
		}
		else if (extension == ".mp3" || extension == ".wav" || extension == ".ogg")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "sound-asset",
				.filePath = relativePath,
				.type = AssetType::Sound,
			});
		}
		else if (extension == ".mat")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "material-asset",
				.filePath = relativePath,
				.type = AssetType::Material,
			});
		}
		else if (extension == ".shader")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "shader-asset",
				.filePath = relativePath,
				.type = AssetType::Shader,
			});
		}
		else if (extension == ".humble")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "scene-asset",
				.filePath = relativePath,
				.type = AssetType::Scene,
			});
		}
		else if (extension == ".h")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "script-asset",
				.filePath = relativePath,
				.type = AssetType::Script,
			});
		}
		else if (extension == ".prefab")
		{
			assetHandle = AssetManager::Instance->CreateAsset({
				.debugName = "prefab-asset",
				.filePath = relativePath,
				.type = AssetType::Prefab,
			});
		}

		return assetHandle;
	}

	void AssetManager::DeregisterAssets()
	{
		WaitForAsyncJobs();
		
		const auto& builtInShaderAssets = ShaderUtilities::Get().GetBuiltInShaderAssets();
		const auto& builtInMeshAssets = MeshUtilities::Get().GetBuiltInMeshAssets();

		for (const auto handle : m_RegisteredAssets)
		{
			// Skip if is a built in material, shader or mesh asset.
			bool isBuiltInAsset = false;

			for (const auto shaderAssetHandle : builtInShaderAssets)
			{
				if (handle == shaderAssetHandle) { isBuiltInAsset = true; break; }
			}

			for (const auto meshAssetHandle : builtInMeshAssets)
			{
				if (handle == meshAssetHandle) { isBuiltInAsset = true; break; }
			}

			if (handle == ShaderUtilities::Get().LitMaterialAsset) { isBuiltInAsset = true; }

			if (isBuiltInAsset)
			{
				continue;
			}

			// Delete assets.
			DeleteAsset(handle);
		}

		// Clear asset handle caches.
		m_RegisteredAssets.clear();
		m_RegisteredAssetMap.clear();

		// Reregister built in shader assets.
		for (const auto shaderAssetHandle : builtInShaderAssets)
		{
			m_RegisteredAssets.push_back(shaderAssetHandle);
			Asset* asset = GetAssetMetadata(shaderAssetHandle);
			m_RegisteredAssetMap[asset->UUID] = shaderAssetHandle;
		}

		// Reregister built in material asset.
		if (ShaderUtilities::Get().LitMaterialAsset.IsValid())
		{
			m_RegisteredAssets.push_back(ShaderUtilities::Get().LitMaterialAsset);
			Asset* asset = GetAssetMetadata(ShaderUtilities::Get().LitMaterialAsset);
			m_RegisteredAssetMap[asset->UUID] = ShaderUtilities::Get().LitMaterialAsset;
		}
	}

	void AssetManager::WaitForAsyncJobs(JobContext* customJobCtx)
	{
		JobContext& ctx = (customJobCtx == nullptr ? m_ResourceJobCtx : *customJobCtx);
		JobSystem::Get().Wait(ctx);
	}

	Handle<Asset> AssetManager::GetHandleFromUUID(UUID assetUUID)
	{
		Handle<Asset> assetHandle;

		auto it = m_RegisteredAssetMap.find(assetUUID);
		if (it != m_RegisteredAssetMap.end())
		{
			assetHandle = it->second;
		}

		return assetHandle;
	}

	void AssetManager::SaveAsset(UUID assetUUID)
	{
		return SaveAsset(GetHandleFromUUID(assetUUID));
	}
}
