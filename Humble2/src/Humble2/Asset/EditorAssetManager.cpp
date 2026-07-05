#include "EditorAssetManager.h"

#include "Script/BuildEngine.h"
#include "Utilities/YamlUtilities.h"
#include "Utilities/Collections/Collections.h"
#include "Utilities/Collections/StaticDArray.h"

#include "Systems/HierachySystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/RenderingSystem.h"
#include "Systems/SoundSystem.h"
#include "Systems/Physics2dSystem.h"
#include "Systems/Physics3dSystem.h"
#include "Systems/TerrainSystem.h"
#include "Systems/AnimationCurveSystem.h"

#include "Prefab/PrefabSerializer.h"

namespace HBL2
{
	using packed_size = ShaderDescriptor::RenderPipeline::packed_size;

    uint32_t EditorAssetManager::LoadAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);
		if (asset == nullptr)
		{
			return 0;
		}

		switch (asset->Type)
		{
		case AssetType::Texture:
			asset->Indentifier = ImportTexture(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Shader:
			asset->Indentifier = ImportShader(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Material:
			asset->Indentifier = ImportMaterial(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Mesh:
			asset->Indentifier = ImportMesh(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Scene:
			asset->Indentifier = ImportScene(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Script:
			asset->Indentifier = ImportScript(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Sound:
			asset->Indentifier = ImportSound(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Prefab:
			asset->Indentifier = ImportPrefab(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		}

		return 0;
    }

	uint32_t EditorAssetManager::ReloadAsset(Handle<Asset> handle)
	{
		Asset* asset = GetAssetMetadata(handle);
		if (asset == nullptr)
		{
			return 0;
		}

		switch (asset->Type)
		{
		case AssetType::Shader:
			asset->Indentifier = ReimportShader(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Material:
			asset->Indentifier = ReimportMaterial(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Mesh:
			asset->Indentifier = ReimportMesh(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		case AssetType::Prefab:
			asset->Indentifier = ReimportPrefab(asset).Pack();
			asset->Loaded = (asset->Indentifier != 0);
			return asset->Indentifier;
		}

		return 0;
	}

    void EditorAssetManager::UnloadAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);

		if (asset == nullptr)
		{
			return;
		}

		switch (asset->Type)
		{
		case AssetType::Texture:
			UnloadTexture(asset);
			break;
		case AssetType::Shader:
			UnloadShader(asset);
			break;
		case AssetType::Mesh:
			UnloadMesh(asset);
			break;
		case AssetType::Material:
			UnloadMaterial(asset);
			break;
		case AssetType::Script:
			UnloadScript(asset);
			break;
		case AssetType::Scene:
			UnloadScene(asset);
			break;
		case AssetType::Sound:
			UnloadSound(asset);
			break;
		case AssetType::Prefab:
			UnloadPrefab(asset);
			break;
		}
    }

    void EditorAssetManager::DestroyAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);

		if (asset == nullptr)
		{
			return;
		}

		bool destroyResult = false;

		switch (asset->Type)
		{
		case AssetType::Texture:
			destroyResult = DestroyTexture(asset);
			break;
		case AssetType::Shader:
			destroyResult = DestroyShader(asset);
			break;
		case AssetType::Material:
			destroyResult = DestroyMaterial(asset);
			break;
		case AssetType::Mesh:
			destroyResult = DestroyMesh(asset);
			break;
		case AssetType::Script:
			destroyResult = DestroyScript(asset);
			break;
		case AssetType::Scene:
			destroyResult = DestroyScene(asset);
			break;
		case AssetType::Sound:
			destroyResult = DestroySound(asset);
			break;
		case AssetType::Prefab:
			destroyResult = DestroyPrefab(asset);
			break;
		}

		if (destroyResult)
		{
			m_RegisteredAssetMap.erase(asset->UUID);
			m_RegisteredAssetPathToUUIDMap.erase(asset->FilePath);

			auto assetIterator = std::find(m_RegisteredAssets.begin(), m_RegisteredAssets.end(), handle);

			if (assetIterator != m_RegisteredAssets.end())
			{
				m_RegisteredAssets.erase(assetIterator);
			}

			m_AssetPool.Remove(handle);
		}
    }

	void EditorAssetManager::SaveAsset(UUID assetUUID)
	{
		return SaveAsset(GetHandleFromUUID(assetUUID));
	}

	void EditorAssetManager::SaveAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);

        if (asset->Loaded == false)
        {
            LoadAsset(handle);
        }

		if (asset == nullptr)
		{
			return;
		}

		switch (asset->Type)
		{
		case AssetType::Material:
			SaveMaterial(asset);
			break;
		case AssetType::Texture:
			SaveTexture(asset);
			break;
		case AssetType::Scene:
			SaveScene(asset);
			break;
		case AssetType::Script:
			SaveScript(asset);
			break;
		case AssetType::Sound:
			SaveSound(asset);
			break;
		case AssetType::Prefab:
			SavePrefab(asset);
			break;
		}
    }

    bool EditorAssetManager::IsAssetValid(Handle<Asset> handle)
    {
        return handle.IsValid() && GetAssetMetadata(handle) != nullptr;
    }

    bool EditorAssetManager::IsAssetLoaded(Handle<Asset> handle)
    {
        if (!IsAssetValid(handle))
        {
            return false;
        }

        Asset* asset = GetAssetMetadata(handle);
        return asset->Loaded;
    }

	void EditorAssetManager::RegisterAssets()
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

	void EditorAssetManager::DeregisterAssets()
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
			else if (handle == ShaderUtilities::Get().UnlitMaterialAsset) { isBuiltInAsset = true; }

			if (isBuiltInAsset)
			{
				continue;
			}

			// Delete assets.
			DeleteAsset(handle);
		}

		// Clear asset handle caches.
		m_RegisteredAssets.clear();
		m_RegisteredAssetPathToUUIDMap.clear();
		m_RegisteredAssetMap.clear();

		// Reregister built in shader assets.
		for (const auto shaderAssetHandle : builtInShaderAssets)
		{
			m_RegisteredAssets.push_back(shaderAssetHandle);
			Asset* asset = GetAssetMetadata(shaderAssetHandle);
			m_RegisteredAssetPathToUUIDMap[asset->FilePath] = asset->UUID;
			m_RegisteredAssetMap[asset->UUID] = shaderAssetHandle;
		}

		// Reregister built-in lit material asset.
		if (ShaderUtilities::Get().LitMaterialAsset.IsValid())
		{
			m_RegisteredAssets.push_back(ShaderUtilities::Get().LitMaterialAsset);
			Asset* asset = GetAssetMetadata(ShaderUtilities::Get().LitMaterialAsset);
			m_RegisteredAssetPathToUUIDMap[asset->FilePath] = asset->UUID;
			m_RegisteredAssetMap[asset->UUID] = ShaderUtilities::Get().LitMaterialAsset;
		}

		// Reregister built-in unlit material asset.
		if (ShaderUtilities::Get().UnlitMaterialAsset.IsValid())
		{
			m_RegisteredAssets.push_back(ShaderUtilities::Get().UnlitMaterialAsset);
			Asset* asset = GetAssetMetadata(ShaderUtilities::Get().UnlitMaterialAsset);
			m_RegisteredAssetPathToUUIDMap[asset->FilePath] = asset->UUID;
			m_RegisteredAssetMap[asset->UUID] = ShaderUtilities::Get().UnlitMaterialAsset;
		}
	}

	Handle<Asset> EditorAssetManager::CreateAsset(const AssetDescriptor&& desc)
	{
		auto assetUUID = GetUUIDFromPath(desc.filePath);
		auto handle = GetHandleFromUUID(assetUUID);

		if (IsAssetValid(handle))
		{
			return handle;
		}

		handle = m_AssetPool.Insert(Asset(std::forward<const AssetDescriptor>(desc)));

		Asset* asset = GetAssetMetadata(handle);		

		switch (asset->Type)
		{
		case AssetType::Texture:
			CreateTextureMetadata(asset);
			break;
		case AssetType::Shader:
			CreateShaderMetadata(asset);
			break;
		case AssetType::Material:
			CreateMaterialMetadata(asset);
			break;
		case AssetType::Scene:
			CreateSceneMetadata(asset);
			break;
		case AssetType::Mesh:
			CreateMeshMetadata(asset);
			break;
		case AssetType::Script:
			CreateScriptMetadata(asset);
			break;
		case AssetType::Sound:
			CreateSoundMetadata(asset);
			break;
		case AssetType::Prefab:
			CreatePrefabMetadata(asset);
			break;
		case AssetType::None:
			HBL2_CORE_ERROR("Aborting asset creation, asset type is None.");
			return Handle<Asset>();
		}

		m_RegisteredAssets.push_back(handle);
		m_RegisteredAssetPathToUUIDMap[asset->FilePath] = asset->UUID;
		m_RegisteredAssetMap[asset->UUID] = handle;

		return handle;
	}

	Handle<Asset> EditorAssetManager::RegisterAsset(const std::filesystem::path& assetPath)
	{
		const std::string& extension = assetPath.extension().string();
		auto relativePath = std::filesystem::relative(assetPath, HBL2::Project::GetAssetDirectory());

		Handle<Asset> assetHandle;

		if (extension == ".png" || extension == ".jpg" || extension == ".tga" || extension == ".hdr")
		{
			assetHandle = CreateAsset({
				.debugName = "texture-asset",
				.filePath = relativePath,
				.type = AssetType::Texture,
			});
		}
		else if (extension == ".obj" || extension == ".gltf" || extension == ".glb" || extension == ".fbx" || extension == ".FBX")
		{
			assetHandle = CreateAsset({
				.debugName = "mesh-asset",
				.filePath = relativePath,
				.type = AssetType::Mesh,
			});
		}
		else if (extension == ".mp3" || extension == ".wav" || extension == ".ogg")
		{
			assetHandle = CreateAsset({
				.debugName = "sound-asset",
				.filePath = relativePath,
				.type = AssetType::Sound,
			});
		}
		else if (extension == ".mat")
		{
			assetHandle = CreateAsset({
				.debugName = "material-asset",
				.filePath = relativePath,
				.type = AssetType::Material,
			});
		}
		else if (extension == ".slang")
		{
			assetHandle = CreateAsset({
				.debugName = "shader-asset",
				.filePath = relativePath,
				.type = AssetType::Shader,
			});
		}
		else if (extension == ".humble")
		{
			assetHandle = CreateAsset({
				.debugName = "scene-asset",
				.filePath = relativePath,
				.type = AssetType::Scene,
			});
		}
		else if (extension == ".h")
		{
			assetHandle = CreateAsset({
				.debugName = "script-asset",
				.filePath = relativePath,
				.type = AssetType::Script,
			});
		}
		else if (extension == ".prefab")
		{
			assetHandle = CreateAsset({
				.debugName = "prefab-asset",
				.filePath = relativePath,
				.type = AssetType::Prefab,
			});
		}

		return assetHandle;
	}

	UUID EditorAssetManager::GetUUIDFromPath(const std::filesystem::path& assetPath)
	{
		UUID assetUUID = 0;

		auto it = m_RegisteredAssetPathToUUIDMap.find(assetPath);
		if (it != m_RegisteredAssetPathToUUIDMap.end())
		{
			assetUUID = it->second;
		}

		return assetUUID;
	}

	/// Create methods

	void EditorAssetManager::CreateTextureMetadata(Asset* asset)
	{
		const auto& path = HBL2::Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& filePath = path.string() + ".hbltexture";

		// Check if the metadata exists, if it does retrieve the UUID and assign it to the asset.
		if (std::filesystem::exists(filePath))
		{
			std::ifstream stream(filePath);

			if (!stream.is_open())
			{
				HBL2_CORE_ERROR("Texture metadata file not found: {0}", filePath);
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (!data["Texture"].IsDefined())
			{
				HBL2_CORE_ERROR("Texture not found: {0}", asset->DebugName);
				stream.close();
				return;
			}

			auto textureProperties = data["Texture"];
			if (textureProperties)
			{
				asset->UUID = textureProperties["UUID"].as<UUID>();
			}

			return;
		}

		// Ensure parent path exists.
		if (!std::filesystem::exists(path.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(path.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Texture metadata directory creation failed: {0}", e.what());
			}
		}

		// If the metadata file does not exist, generate new UUID and assign it to the asset.
		asset->UUID = Random::UInt64();
	}

	void EditorAssetManager::CreateShaderMetadata(Asset* asset)
	{
		const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetAssetDirectory().parent_path().parent_path();
		const auto& path = std::filesystem::exists(filesystemPath) ? filesystemPath : workingDirectory / asset->FilePath;
		const auto& metaFilePath = path.string() + ".hblshader";

		// Check if the metadata exists, if it does retrieve the UUID and assign it to the asset.
		if (std::filesystem::exists(metaFilePath))
		{
			std::ifstream stream(metaFilePath);

			if (!stream.is_open())
			{
				HBL2_CORE_ERROR("Shader metadata file not found: {0}", metaFilePath);
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (!data["Shader"].IsDefined())
			{
				HBL2_CORE_ERROR("Shader not found: {0}", asset->DebugName);
				stream.close();
				return;
			}

			auto shaderProperties = data["Shader"];
			if (shaderProperties)
			{
				asset->UUID = shaderProperties["UUID"].as<UUID>();
			}

			return;
		}

		// Ensure parent path exists.
		if (!std::filesystem::exists(path.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(path.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Shader metadata directory creation failed: {0}", e.what());
			}
		}

		// If the metadata file does not exist, generate new UUID and assign it to the asset.
		asset->UUID = Random::UInt64();
	}

	void EditorAssetManager::CreateMaterialMetadata(Asset* asset)
	{
		const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetProjectDirectory().parent_path();
		const auto& path = std::filesystem::exists(filesystemPath) ? filesystemPath : workingDirectory / asset->FilePath;
		const auto& metaFilePath = path.string() + ".hblmat";

		// Check if the metadata exists, if it does retrieve the UUID and assign it to the asset.
		if (std::filesystem::exists(metaFilePath))
		{
			std::ifstream stream(metaFilePath);

			if (!stream.is_open())
			{
				HBL2_CORE_ERROR("Material metadata file not found: {0}", metaFilePath);
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (!data["Material"].IsDefined())
			{
				HBL2_CORE_ERROR("Material not found: {0}", asset->DebugName);
				stream.close();
				return;
			}

			auto materialProperties = data["Material"];
			if (materialProperties)
			{
				asset->UUID = materialProperties["UUID"].as<UUID>();
			}

			return;
		}

		// If the metadata file does not exist, generate new UUID and assign it to the asset.
		asset->UUID = Random::UInt64();
	}

	void EditorAssetManager::CreateMeshMetadata(Asset* asset)
	{
		const auto& assetFilePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetAssetDirectory().parent_path().parent_path();
		const auto& path = std::filesystem::exists(assetFilePath) ? assetFilePath : workingDirectory / asset->FilePath;
		const auto& metaFilePath = path.string() + ".hblmesh";

		// Check if the metadata exists, if it does retrieve the UUID and assign it to the asset.
		if (std::filesystem::exists(metaFilePath))
		{
			std::ifstream stream(metaFilePath);

			if (!stream.is_open())
			{
				HBL2_CORE_ERROR("Mesh metadata file not found: {0}", metaFilePath);
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (!data["Mesh"].IsDefined())
			{
				HBL2_CORE_ERROR("Mesh not found: {0}", asset->DebugName);
				stream.close();
				return;
			}

			auto meshProperties = data["Mesh"];
			if (meshProperties)
			{
				asset->UUID = meshProperties["UUID"].as<UUID>();
			}

			return;
		}

		// Ensure parent path exists.
		if (!std::filesystem::exists(path.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(path.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Mesh metadata directory creation failed: {0}", e.what());
			}
		}

		// If the metadata file does not exist, generate new UUID and assign it to the asset.
		asset->UUID = Random::UInt64();
	}

	void EditorAssetManager::CreateSceneMetadata(Asset* asset)
	{
		const auto& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& metaFilePath = filePath.string() + ".hblscene";

		// Check if the metadata exists, if it does retrieve the UUID and assign it to the asset.
		if (std::filesystem::exists(metaFilePath))
		{
			std::ifstream stream(metaFilePath);

			if (!stream.is_open())
			{
				HBL2_CORE_ERROR("Scene metadata file not found: {0}", metaFilePath);
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (!data["Scene"].IsDefined())
			{
				HBL2_CORE_ERROR("Scene not found: {0}", asset->DebugName);
				stream.close();
				return;
			}

			auto sceneProperties = data["Scene"];
			if (sceneProperties)
			{
				asset->UUID = sceneProperties["UUID"].as<UUID>();
			}

			return;
		}

		// Ensure parent path exists.
		if (!std::filesystem::exists(filePath.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(filePath.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Scene metadata directory creation failed: {0}", e.what());
			}
		}

		// If the metadata file does not exist, generate new UUID and assign it to the asset.
		asset->UUID = Random::UInt64();
	}

	void EditorAssetManager::CreateScriptMetadata(Asset* asset)
	{
		const auto& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& metaFilePath = filePath.string() + ".hblscript";

		// Check if the metadata exists, if it does retrieve the UUID and assign it to the asset.
		if (std::filesystem::exists(metaFilePath))
		{
			std::ifstream stream(metaFilePath);

			if (!stream.is_open())
			{
				HBL2_CORE_ERROR("Script metadata file not found: {0}", metaFilePath);
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (!data["Script"].IsDefined())
			{
				HBL2_CORE_ERROR("Script not found: {0}", asset->DebugName);
				stream.close();
				return;
			}

			auto scriptProperties = data["Script"];
			if (scriptProperties)
			{
				asset->UUID = scriptProperties["UUID"].as<UUID>();
			}

			return;
		}

		// Ensure parent path exists.
		if (!std::filesystem::exists(filePath.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(filePath.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Script metadata directory creation failed: {0}", e.what());
			}
		}

		// If the metadata file does not exist, generate new UUID and assign it to the asset.
		asset->UUID = Random::UInt64();
	}

	void EditorAssetManager::CreateSoundMetadata(Asset* asset)
	{
		const auto& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& metaFilePath = filePath.string() + ".hblsound";

		// Check if the metadata exists, if it does retrieve the UUID and assign it to the asset.
		if (std::filesystem::exists(metaFilePath))
		{
			std::ifstream stream(metaFilePath);

			if (!stream.is_open())
			{
				HBL2_CORE_ERROR("Sound metadata file not found: {0}", metaFilePath);
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (!data["Sound"].IsDefined())
			{
				HBL2_CORE_ERROR("Sound not found: {0}", asset->DebugName);
				stream.close();
				return;
			}

			auto soundProperties = data["Sound"];
			if (soundProperties)
			{
				asset->UUID = soundProperties["UUID"].as<UUID>();
			}

			return;
		}

		// Ensure parent path exists.
		if (!std::filesystem::exists(filePath.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(filePath.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Sound metadata directory creation failed: {0}", e.what());
			}
		}

		// If the metadata file does not exist, generate new UUID and assign it to the asset.
		asset->UUID = Random::UInt64();
	}

	void EditorAssetManager::CreatePrefabMetadata(Asset* asset)
	{
		const auto& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& metaFilePath = filePath.string() + ".hblprefab";

		// Check if the metadata exists, if it does retrieve the UUID and assign it to the asset.
		if (std::filesystem::exists(metaFilePath))
		{
			std::ifstream stream(metaFilePath);

			if (!stream.is_open())
			{
				HBL2_CORE_ERROR("Prefab metadata file not found: {0}", metaFilePath);
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf();

			YAML::Node data = YAML::Load(ss.str());
			if (!data["Prefab"].IsDefined())
			{
				HBL2_CORE_ERROR("Prefab not found: {0}", asset->DebugName);
				stream.close();
				return;
			}

			auto prefabProperties = data["Prefab"];
			if (prefabProperties)
			{
				asset->UUID = prefabProperties["UUID"].as<UUID>();
			}

			return;
		}

		// Ensure parent path exists.
		if (!std::filesystem::exists(filePath.parent_path()))
		{
			try
			{
				std::filesystem::create_directories(filePath.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Prefab metadata directory creation failed: {0}", e.what());
			}
		}

		// If the metadata file does not exist, generate new UUID and assign it to the asset.
		asset->UUID = Random::UInt64();
	}

	/// Import methods

	Handle<Texture> EditorAssetManager::ImportTexture(Asset* asset)
	{
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture");

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Texture metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture");
			return Handle<Texture>();
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Texture"].IsDefined())
		{
			HBL2_CORE_ERROR("Texture not found: {0}", asset->DebugName);
			stream.close();
			return Handle<Texture>();
		}

		auto textureProperties = data["Texture"];
		if (textureProperties)
		{
			// Load the texture
			TextureSettings textureSettings =
			{
				.Flip = textureProperties["Flip"].as<bool>(),
			};
			void* textureData = TextureUtilities::Get().Load(Project::GetAssetFileSystemPath(asset->FilePath).string(), textureSettings);

			const std::string& textureName = asset->FilePath.filename().stem().string();

			// Create the texture
			auto texture = ResourceManager::Instance->CreateTexture({
				.debugName = strdup(std::format("{}-texture", textureName).c_str()),
				.dimensions = { textureSettings.Width, textureSettings.Height, 1 },
				.format = textureSettings.PixelFormat,
				.internalFormat = textureSettings.PixelFormat,
				.usage = { TextureUsage::SAMPLED, TextureUsage::COPY_DST },
				.aspect = TextureAspect::COLOR,
				.sampler = { .filter = TextureFilter::LINEAR },
				.initialData = textureData,
			});

			stream.close();
			return texture;
		}

		HBL2_CORE_ERROR("Texture not found: {0}", asset->DebugName);

		stream.close();
		return Handle<Texture>();
	}

	Handle<Shader> EditorAssetManager::ImportShader(Asset* asset)
	{
		const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetAssetDirectory().parent_path().parent_path();
		const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : workingDirectory / asset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

		std::ifstream stream(shaderPath.string() + ".hblshader");

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Shader metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblshader");
			return Handle<Shader>();
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Shader"].IsDefined())
		{
			HBL2_CORE_ERROR("Shader not found: {0}", asset->DebugName);
			stream.close();
			return Handle<Shader>();
		}

		const std::string& shaderName = asset->FilePath.filename().stem().string();

		Handle<BindGroupLayout> globalBindGroupLayout;

		StaticDArray<ShaderDescriptor::RenderPipeline::PackedVariant, 16> shaderVariants;
		uint32_t type = 0;

		const auto& shaderProperties = data["Shader"];
		if (shaderProperties)
		{
			type = shaderProperties["Type"].as<uint32_t>();

			switch (type)
			{
			case 0:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout2D();
				break;
			case 1:
			case 2:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout3D();
				break;
			default:
				HBL2_CORE_ERROR("Unknown Shader type: {0}", asset->DebugName);
				stream.close();
				return Handle<Shader>();
			}

			// Retrieve shader variants.
			const auto& shaderVariantsProperty = shaderProperties["Variants"];

			if (shaderVariantsProperty)
			{
				for (const YAML::Node& variantNode : shaderVariantsProperty)
				{
					ShaderDescriptor::RenderPipeline::PackedVariant variant = {};

					// Retrieve raster state.
					const auto& rasterStateProp = variantNode["RasterState"];

					if (rasterStateProp)
					{
						variant.topology = (packed_size)(Topology)rasterStateProp["Topology"].as<int>();
						variant.polygonMode = (packed_size)(PolygonMode)rasterStateProp["PolygonMode"].as<int>();
						variant.cullMode = (packed_size)(CullMode)rasterStateProp["CullMode"].as<int>();
						variant.frontFace = (packed_size)(FrontFace)rasterStateProp["FrontFace"].as<int>();
					}

					// Retrieve blend state.
					const auto& blendStateProp = variantNode["BlendState"];

					if (blendStateProp)
					{
						variant.colorOutput = blendStateProp["ColorOutputEnabled"].as<bool>();
						variant.blendEnabled = blendStateProp["Enabled"].as<bool>();

						// TODO: Remove if statements in the future.
						if (blendStateProp["ColorOp"].IsDefined())			variant.colorOp = (packed_size)(BlendOperation)blendStateProp["ColorOp"].as<int>();
						if (blendStateProp["AlphaOp"].IsDefined())			variant.alphaOp = (packed_size)(BlendOperation)blendStateProp["AlphaOp"].as<int>();
						if (blendStateProp["SrcColorFactor"].IsDefined())	variant.srcColorFactor = (packed_size)(BlendFactor)blendStateProp["SrcColorFactor"].as<int>();
						if (blendStateProp["DstColorFactor"].IsDefined())	variant.dstColorFactor = (packed_size)(BlendFactor)blendStateProp["DstColorFactor"].as<int>();
						if (blendStateProp["SrcAlphaFactor"].IsDefined())	variant.srcAlphaFactor = (packed_size)(BlendFactor)blendStateProp["SrcAlphaFactor"].as<int>();
						if (blendStateProp["DstAlphaFactor"].IsDefined())	variant.dstAlphaFactor = (packed_size)(BlendFactor)blendStateProp["DstAlphaFactor"].as<int>();
					}

					// Retrieve depth state.
					const auto& depthStateProp = variantNode["DepthState"];

					if (depthStateProp)
					{
						variant.depthEnabled = depthStateProp["Enabled"].as<bool>();
						variant.depthWrite = depthStateProp["WriteEnabled"].as<bool>();
						variant.stencilEnabled = depthStateProp["StencilEnabled"].as<bool>();
						variant.depthCompare = (packed_size)(Compare)depthStateProp["DepthTest"].as<int>();
					}

					// Retrieve shader constants
					const auto& shaderConstantsStateProp = variantNode["ShaderConstantsState"];

					if (shaderConstantsStateProp)
					{
						variant.shaderConstantBool0 = shaderConstantsStateProp["ShaderConstantBool0"].as<bool>();
						variant.shaderConstantBool1 = shaderConstantsStateProp["ShaderConstantBool1"].as<bool>();
						variant.shaderConstantBool2 = shaderConstantsStateProp["ShaderConstantBool2"].as<bool>();
						variant.shaderConstantBool3 = shaderConstantsStateProp["ShaderConstantBool3"].as<bool>();
						variant.shaderConstantBool4 = shaderConstantsStateProp["ShaderConstantBool4"].as<bool>();
						variant.shaderConstantBool5 = shaderConstantsStateProp["ShaderConstantBool5"].as<bool>();
						variant.shaderConstantBool6 = shaderConstantsStateProp["ShaderConstantBool6"].as<bool>();
						variant.shaderConstantBool7 = shaderConstantsStateProp["ShaderConstantBool7"].as<bool>();
					}

					shaderVariants.push_back(variant);
				}
			}
		}

		// Compile Shader.
		ShaderReflectionData outReflectionData;
		const auto& compilationData = ShaderUtilities::Get().Compile(shaderPath.string(), &outReflectionData);

		if (!compilationData.IsValid())
		{
			HBL2_CORE_ERROR("Shader asset: {0}, at path: {1}, could not be compiled. Returning invalid shader.", asset->DebugName, shaderPath.string());
			stream.close();
			return ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::INVALID);
		}

		// Retrieve shader bind group.
		Handle<BindGroup> shaderBindGroup;
		if (shaderProperties && shaderProperties["BindGroup"].IsDefined())
		{
			for (const auto& descriptorSet : outReflectionData.descriptorSets)
			{
				if (descriptorSet.set != 1)
				{
					continue;
				}

				JobContext shaderTextureCtx;

				StaticDArray<ResourceTask<Texture>*, 8> textureTasks;
				StaticDArray<BindGroupDescriptor::TextureEntry, 8> textureBindings;
				StaticDArray<BindGroupDescriptor::BufferEntry, 8> bufferBindings;

				uint32_t bindingIndex = 0;

				for (const auto& b : descriptorSet.bindings)
				{
					if (b.type == ResourceType::UniformBuffer)
					{
						std::vector<uint8_t> uniformBufferBytes(b.size);

						const auto& bufferProp = shaderProperties["BindGroup"][bindingIndex];

						if (bufferProp.IsDefined())
						{
							for (const auto& m : b.members)
							{
								const auto& memberProp = bufferProp[b.name][m.name];

								if (!memberProp.IsDefined())
								{
									continue;
								}

								uint8_t* memberPtr = uniformBufferBytes.data() + m.offset;

								switch (m.typeInfo.base)
								{
								case MemberBaseType::Float:
								{
									if (m.typeInfo.isArray)
									{
										const uint32_t stride = m.typeInfo.arrayCount > 0 ? m.size / m.typeInfo.arrayCount : 0;

										for (uint32_t i = 0; i < m.typeInfo.arrayCount; ++i)
										{
											if (!memberProp[i].IsDefined())
											{
												continue;
											}

											float* f = reinterpret_cast<float*>(memberPtr + i * stride);

											if (m.typeInfo.cols == 1)
											{
												*f = memberProp[i].as<float>();
											}
											else if (m.typeInfo.cols == 2)
											{
												const glm::vec2& vec2 = memberProp[i].as<glm::vec2>();

												f[0] = vec2.x;
												f[1] = vec2.y;
											}
											else if (m.typeInfo.cols == 3)
											{
												const glm::vec3& vec3 = memberProp[i].as<glm::vec3>();

												f[0] = vec3.x;
												f[1] = vec3.y;
												f[2] = vec3.z;
											}
											else if (m.typeInfo.cols == 4)
											{
												const glm::vec4& vec4 = memberProp[i].as<glm::vec4>();

												f[0] = vec4.x;
												f[1] = vec4.y;
												f[2] = vec4.z;
												f[3] = vec4.w;
											}
										}
									}
									else
									{
										float* f = reinterpret_cast<float*>(memberPtr);

										if (m.typeInfo.cols == 1)
										{
											*f = memberProp.as<float>();
										}
										else if (m.typeInfo.cols == 2)
										{
											const glm::vec2& vec2 = memberProp.as<glm::vec2>();

											f[0] = vec2.x;
											f[1] = vec2.y;
										}
										else if (m.typeInfo.cols == 3)
										{
											const glm::vec3& vec3 = memberProp.as<glm::vec3>();

											f[0] = vec3.x;
											f[1] = vec3.y;
											f[2] = vec3.z;
										}
										else if (m.typeInfo.cols == 4)
										{
											const glm::vec4& vec4 = memberProp.as<glm::vec4>();

											f[0] = vec4.x;
											f[1] = vec4.y;
											f[2] = vec4.z;
											f[3] = vec4.w;
										}
									}
									break;
								}
								}
							}

							auto userBuffer = ResourceManager::Instance->CreateBuffer({
								.debugName = "shader-uniform-buffer",
								.usage = BufferUsage::UNIFORM,
								.usageHint = BufferUsageHint::DYNAMIC,
								.memoryUsage = MemoryUsage::GPU_CPU,
								.byteSize = (uint32_t)b.size,
								.initialData = (void*)uniformBufferBytes.data(),
							});

							bufferBindings.push_back({ .buffer = userBuffer, });
						}
					}
					else if (b.type == ResourceType::SampledTexture)
					{
						const auto& textureProp = shaderProperties["BindGroup"][bindingIndex];

						if (textureProp.IsDefined())
						{
							UUID textureMapUUID = textureProp[b.name].as<UUID>();

							auto* task = AssetManager::Instance->GetAssetAsync<Texture>(textureMapUUID, &shaderTextureCtx);
							textureTasks.push_back(task);
						}
					}

					bindingIndex++;
				}

				AssetManager::Instance->WaitForAsyncJobs(&shaderTextureCtx);

				for (auto* task : textureTasks)
				{
					if (task != nullptr)
					{
						textureBindings.push_back({ task->ResourceHandle });
						AssetManager::Instance->ReleaseResourceTask(task);
					}
					else
					{
						textureBindings.push_back({ Handle<Texture>() });
					}
				}

				// If there is only one texture and is not set, use the built in white texture.
				if (textureBindings.size() == 1)
				{
					if (!textureBindings[0].texture.IsValid())
					{
						textureBindings[0].texture = TextureUtilities::Get().WhiteTexture;
					}
				}

				shaderBindGroup = ResourceManager::Instance->CreateBindGroup({
					.debugName = "shader-bind-group",
					.layout = outReflectionData.GetBindGroupLayout(1),
                    .textures = { textureBindings.data(), textureBindings.size() },
                    .buffers = { bufferBindings.data(), bufferBindings.size() },
				});
			}
		}
		else
		{
			HBL2_CORE_ERROR("Shader {0} has an ill-formed metadata file.", shaderPath);
		}

		// Create resource.
		auto shader = ResourceManager::Instance->CreateShader({
			.debugName = strdup(std::format("{}-shader", shaderName).c_str()),
			.VS { .code = compilationData.vertexShaderCode.AsSpan(), .entryPoint = outReflectionData.entryPoints[0].name.c_str() },
			.FS { .code = compilationData.fragmentShaderCode.AsSpan(), .entryPoint = outReflectionData.entryPoints[1].name.c_str() },
			.bindGroups {
				globalBindGroupLayout,							// Global bind group			(0)
				outReflectionData.GetBindGroupLayout(1),		// Global user bind group		(1)
				outReflectionData.GetBindGroupLayout(2),		// Material / user bind group	(2)
				Renderer::Instance->GetDynamicBindingsLayout(),	// Draw bind group				(3)
			},
			.renderPipeline {
				.vertexBufferBindings = outReflectionData.vertexBufferBindings,
				.variants = { shaderVariants.data(), shaderVariants.size() },
                .specializationConstantsPerVariant = outReflectionData.GetSpecializationConstantsPerVariant({ shaderVariants.data(), shaderVariants.size() }),
			},
			.renderPass = Renderer::Instance->GetRenderingRenderPass(),
			.shaderBindGroup = shaderBindGroup,
		});

		stream.close();
		return shader;
	}

	Handle<Material> EditorAssetManager::ImportMaterial(Asset* asset)
	{
		// Open metadata file.
		const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetProjectDirectory().parent_path();
		const std::filesystem::path& materialPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : workingDirectory / asset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

		std::ifstream metaDataStream(materialPath.string() + ".hblmat");

		if (!metaDataStream.is_open())
		{
			HBL2_CORE_ERROR("Material metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblmat");
			return Handle<Material>();
		}

		std::stringstream ssMetadata;
		ssMetadata << metaDataStream.rdbuf();

		YAML::Node dataMetadata = YAML::Load(ssMetadata.str());
		if (!dataMetadata["Material"].IsDefined())
		{
			HBL2_CORE_ERROR("Material not found in metadata file: {0}", ssMetadata.str());
			metaDataStream.close();
			return Handle<Material>();
		}

		uint32_t type = UINT32_MAX;

		auto materialMetadataProperties = dataMetadata["Material"];
		if (materialMetadataProperties)
		{
			type = materialMetadataProperties["Type"].as<uint32_t>();
		}

		metaDataStream.close();

		// Open regular material file.
		std::ifstream stream(materialPath);

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Material file not found: {0}", materialPath);
			return Handle<Material>();
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Material"].IsDefined())
		{
			HBL2_CORE_ERROR("Material not found: {0}", ss.str());
			stream.close();
			return Handle<Material>();
		}

		auto materialProperties = data["Material"];
		if (materialProperties)
		{
			UUID shaderUUID = materialProperties["Shader"].as<UUID>();

			ShaderDescriptor::RenderPipeline::PackedVariant variantDesc = {};

			if (materialProperties["RasterState"].IsDefined())
			{
				variantDesc.topology = (packed_size)(Topology)materialProperties["RasterState"]["Topology"].as<int>();
				variantDesc.polygonMode = (packed_size)(PolygonMode)materialProperties["RasterState"]["PolygonMode"].as<int>();
				variantDesc.cullMode = (packed_size)(CullMode)materialProperties["RasterState"]["CullMode"].as<int>();
				variantDesc.frontFace = (packed_size)(FrontFace)materialProperties["RasterState"]["FrontFace"].as<int>();
			}

			if (materialProperties["BlendState"].IsDefined())
			{
				variantDesc.colorOutput = materialProperties["BlendState"]["ColorOutputEnabled"].as<bool>();
				variantDesc.blendEnabled = materialProperties["BlendState"]["Enabled"].as<bool>();
			}

			if (materialProperties["DepthState"].IsDefined())
			{
				variantDesc.depthEnabled = materialProperties["DepthState"]["Enabled"].as<bool>();
				variantDesc.depthWrite = materialProperties["DepthState"]["WriteEnabled"].as<bool>();
				variantDesc.stencilEnabled = materialProperties["DepthState"]["StencilEnabled"].as<bool>();
				variantDesc.depthCompare = (packed_size)(Compare)materialProperties["DepthState"]["DepthTest"].as<int>();
			}

			if (materialProperties["ShaderConstantsState"].IsDefined())
			{
				variantDesc.shaderConstantBool0 = materialProperties["ShaderConstantsState"]["ShaderConstantBool0"].as<bool>();
				variantDesc.shaderConstantBool1 = materialProperties["ShaderConstantsState"]["ShaderConstantBool1"].as<bool>();
				variantDesc.shaderConstantBool2 = materialProperties["ShaderConstantsState"]["ShaderConstantBool2"].as<bool>();
				variantDesc.shaderConstantBool3 = materialProperties["ShaderConstantsState"]["ShaderConstantBool3"].as<bool>();
				variantDesc.shaderConstantBool4 = materialProperties["ShaderConstantsState"]["ShaderConstantBool4"].as<bool>();
				variantDesc.shaderConstantBool5 = materialProperties["ShaderConstantsState"]["ShaderConstantBool5"].as<bool>();
				variantDesc.shaderConstantBool6 = materialProperties["ShaderConstantsState"]["ShaderConstantBool6"].as<bool>();
				variantDesc.shaderConstantBool7 = materialProperties["ShaderConstantsState"]["ShaderConstantBool7"].as<bool>();
			}

			auto shaderAssetHandle = AssetManager::Instance->GetHandleFromUUID(shaderUUID);
			Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(shaderAssetHandle);

			if (shaderAsset == nullptr)
			{
				HBL2_CORE_ERROR("Shader with UUID: {0}, of material: {1}, not found!", shaderUUID, materialPath);
				stream.close();

				return Handle<Material>();
			}

			auto shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderAssetHandle);

			const auto& filesystemPath = Project::GetAssetFileSystemPath(shaderAsset->FilePath);
			const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : workingDirectory / shaderAsset->FilePath;

			// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
			//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

			auto shaderReflectionData = ShaderUtilities::Get().Reflect(shaderPath.string());

			Handle<BindGroup> materialBindGroup;

			for (const auto& descriptorSet : shaderReflectionData.descriptorSets)
			{
				if (descriptorSet.set != 2)
				{
					continue;
				}

				JobContext materialTextureCtx;

				StaticDArray<ResourceTask<Texture>*, 8> textureTasks;
				StaticDArray<BindGroupDescriptor::TextureEntry, 8> textureBindings;
				StaticDArray<BindGroupDescriptor::BufferEntry, 8> bufferBindings;

				for (const auto& b : descriptorSet.bindings)
				{
					if (b.type == ResourceType::UniformBuffer)
					{
						std::vector<uint8_t> uniformBufferBytes(b.size);

						const auto& bufferProp = materialProperties[b.name];

						if (bufferProp.IsDefined())
						{
							for (const auto& m : b.members)
							{
								const auto& memberProp = bufferProp[m.name];

								if (!memberProp.IsDefined())
								{
									continue;
								}

								uint8_t* memberPtr = uniformBufferBytes.data() + m.offset;

								switch (m.typeInfo.base)
								{
								case MemberBaseType::Float:
									{
										if (m.typeInfo.isArray)
										{
											const uint32_t stride = m.typeInfo.arrayCount > 0 ? m.size / m.typeInfo.arrayCount : 0;

											for (uint32_t i = 0; i < m.typeInfo.arrayCount; ++i)
											{
												if (!memberProp[i].IsDefined())
												{
													continue;
												}

												float* f = reinterpret_cast<float*>(memberPtr + i * stride);

												if (m.typeInfo.cols == 1)
												{
													*f = memberProp[i].as<float>();
												}
												else if (m.typeInfo.cols == 2)
												{
													const glm::vec2& vec2 = memberProp[i].as<glm::vec2>();

													f[0] = vec2.x;
													f[1] = vec2.y;
												}
												else if (m.typeInfo.cols == 3)
												{
													const glm::vec3& vec3 = memberProp[i].as<glm::vec3>();

													f[0] = vec3.x;
													f[1] = vec3.y;
													f[2] = vec3.z;
												}
												else if (m.typeInfo.cols == 4)
												{
													const glm::vec4& vec4 = memberProp[i].as<glm::vec4>();

													f[0] = vec4.x;
													f[1] = vec4.y;
													f[2] = vec4.z;
													f[3] = vec4.w;
												}
											}
										}
										else
										{
											float* f = reinterpret_cast<float*>(memberPtr);

											if (m.typeInfo.cols == 1)
											{
												*f = memberProp.as<float>();
											}
											else if (m.typeInfo.cols == 2)
											{
												const glm::vec2& vec2 = memberProp.as<glm::vec2>();

												f[0] = vec2.x;
												f[1] = vec2.y;
											}
											else if (m.typeInfo.cols == 3)
											{
												const glm::vec3& vec3 = memberProp.as<glm::vec3>();

												f[0] = vec3.x;
												f[1] = vec3.y;
												f[2] = vec3.z;
											}
											else if (m.typeInfo.cols == 4)
											{
												const glm::vec4& vec4 = memberProp.as<glm::vec4>();

												f[0] = vec4.x;
												f[1] = vec4.y;
												f[2] = vec4.z;
												f[3] = vec4.w;
											}
										}
										break;
									}
								}
							}

							auto userBuffer = ResourceManager::Instance->CreateBuffer({
								.debugName = "material-uniform-buffer",
								.usage = BufferUsage::UNIFORM,
								.usageHint = BufferUsageHint::DYNAMIC,
								.memoryUsage = MemoryUsage::GPU_CPU,
								.byteSize = (uint32_t)b.size,
								.initialData = (void*)uniformBufferBytes.data(),
							});

							bufferBindings.push_back({ .buffer = userBuffer, });
						}
					}
					else if (b.type == ResourceType::SampledTexture)
					{
						const auto& textureProp = materialProperties[b.name];

						if (textureProp.IsDefined())
						{
							UUID textureMapUUID = textureProp.as<UUID>();

							auto* task = AssetManager::Instance->GetAssetAsync<Texture>(textureMapUUID, &materialTextureCtx);
							textureTasks.push_back(task);
						}
					}
				}

				AssetManager::Instance->WaitForAsyncJobs(&materialTextureCtx);

				for (auto* task : textureTasks)
				{
					if (task != nullptr)
					{
						textureBindings.push_back({ task->ResourceHandle });
						AssetManager::Instance->ReleaseResourceTask(task);
					}
					else
					{
						textureBindings.push_back({ Handle<Texture>() });
					}
				}

				// If there is only one texture and is not set, use the built in white texture.
				if (textureBindings.size() == 1)
				{
					if (!textureBindings[0].texture.IsValid())
					{
						textureBindings[0].texture = TextureUtilities::Get().WhiteTexture;
					}
				}

				materialBindGroup = ResourceManager::Instance->CreateBindGroup({
					.debugName = "material-bind-group",
					.layout = shaderReflectionData.GetBindGroupLayout(2),
                    .textures = { textureBindings.data(), textureBindings.size() },
                    .buffers = { bufferBindings.data(), bufferBindings.size() },
				});
			}

			const auto& builtInShaderAssets = ShaderUtilities::Get().GetBuiltInShaderAssets();

			for (const auto shaderAssetHandle : builtInShaderAssets)
			{
				Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(shaderAssetHandle);

				if (shaderHandle.Pack() == shaderAsset->Indentifier)
				{
					shaderUUID = shaderAsset->UUID;
					break;
				}
			}

			HBL2_CORE_ASSERT(shaderHandle.IsValid(), "Error while trying to load shader of material!");
			HBL2_CORE_ASSERT(shaderUUID != 0, "Error while trying to load shader of material!");

			ResourceManager::Instance->GetOrAddShaderVariant(shaderHandle, variantDesc);
			ShaderUtilities::Get().UpdateShaderVariantMetadataFile(shaderUUID, variantDesc);

			const std::string& materialName = asset->FilePath.filename().stem().string();
			uint32_t dynamicUniformBufferRange = (type == 0 ? sizeof(PerDrawDataSprite) : sizeof(PerDrawData));

			auto drawBindings = ResourceManager::Instance->CreateBindGroup({
				.debugName = strdup(std::format("{}-bind-group", materialName).c_str()),
				.layout = Renderer::Instance->GetDynamicBindingsLayout(),
				.buffers = { { .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer(), .range = dynamicUniformBufferRange }, }
			});

			auto material = ResourceManager::Instance->CreateMaterial({
				.debugName = strdup(std::format("{}-material", materialName).c_str()),
				.shader = shaderHandle,
				.drawBindGroup = drawBindings,
				.materialBindGroup = materialBindGroup,
			});

			Material* mat = ResourceManager::Instance->GetMaterial(material);
			mat->VariantHash = variantDesc;

			stream.close();
			return material;			
		}

		stream.close();
		return Handle<Material>();
	}

	Handle<Scene> EditorAssetManager::ImportScene(Asset* asset)
	{
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblscene");

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Scene metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblscene");
			return Handle<Scene>();
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Scene"].IsDefined())
		{
			HBL2_CORE_TRACE("Scene not found: {0}", ss.str());
			return Handle<Scene>();
		}

		SceneDescriptor desc = {};

		auto sceneProperties = data["Scene"];
		if (sceneProperties)
		{
			desc.name = asset->FilePath.filename().stem().string();
			desc.maxEntities = sceneProperties["MaxEntities"].as<uint32_t>();
			desc.maxComponents = sceneProperties["MaxComponents"].as<uint32_t>();
			desc.maxSystems = sceneProperties["MaxSystems"].as<uint32_t>();
			desc.maxJobsPerSystem = sceneProperties["MaxJobsPerSystem"].as<uint32_t>();
			desc.maxStructuralCommandsPerFramePerThread = sceneProperties["MaxStructuralCommandsPerFramePerThread"].as<uint32_t>();
			desc.useStructuralCommandBuffer = sceneProperties["UseStructuralCommandBuffer"].as<bool>();
		}

		auto sceneHandle = ResourceManager::Instance->CreateScene(std::move(desc));

		Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Scene asset \"{0}\" is invalid, aborting scene load.", asset->FilePath.filename().stem().string());
			return sceneHandle;
		}

		scene->RegisterSystem<HierachySystem>();
		scene->RegisterSystem<CameraSystem>(SystemType::Runtime);
		scene->RegisterSystem<TerrainSystem>();
		scene->RegisterSystem<RenderingSystem>();
		scene->RegisterSystem<SoundSystem>(SystemType::Runtime);
		scene->RegisterSystem<Physics2dSystem>(SystemType::Runtime);
		scene->RegisterSystem<Physics3dSystem>(SystemType::Runtime);
		scene->RegisterSystem<AnimationCurveSystem>();

		SceneSerializer sceneSerializer(scene);
		sceneSerializer.Deserialize(Project::GetAssetFileSystemPath(asset->FilePath));

		return sceneHandle;
	}

	Handle<Script> EditorAssetManager::ImportScript(Asset* asset)
	{
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblscript");
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Script"].IsDefined())
		{
			HBL2_CORE_TRACE("Script not found: {0}", ss.str());
			return Handle<Script>();
		}

		auto scriptProperties = data["Script"];
		if (scriptProperties)
		{
			const std::string& scriptName = asset->FilePath.filename().stem().string();
			uint32_t type = scriptProperties["Type"].as<uint32_t>();

			auto script = ResourceManager::Instance->CreateScript({
				.debugName = scriptName.c_str(),
				.type = (ScriptType)type,
				.path = asset->FilePath,
			});

			return script;
		}

		return Handle<Script>();
	}

	Handle<Sound> EditorAssetManager::ImportSound(Asset* asset)
	{
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblsound");

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Sound metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblsound");
			return Handle<Sound>();
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Sound"].IsDefined())
		{
			HBL2_CORE_ERROR("Sound not found: {0}", asset->DebugName);
			stream.close();
			return Handle<Sound>();
		}

		auto soundProperties = data["Sound"];
		if (soundProperties)
		{
			const std::string& soundName = asset->FilePath.filename().stem().string();

			// Create the texture
			auto sound = ResourceManager::Instance->CreateSound({
				.debugName = strdup(std::format("{}-sound", soundName).c_str()),
				.path = asset->FilePath,
			});

			stream.close();
			return sound;
		}

		HBL2_CORE_ERROR("Sound not found: {0}", asset->DebugName);

		stream.close();
		return Handle<Sound>();
	}

	Handle<Prefab> EditorAssetManager::ImportPrefab(Asset* asset)
	{
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblprefab");
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Prefab"].IsDefined())
		{
			HBL2_CORE_TRACE("Prefab meta data file not valid: {0}", ss.str());
			return Handle<Prefab>();
		}

		const auto& prefabProperties = data["Prefab"];
		if (prefabProperties)
		{
			const std::string& prefabName = asset->FilePath.filename().stem().string();
			UUID baseEntityUUID = prefabProperties["BaseEntityUUID"].as<UUID>();
			uint32_t version = prefabProperties["Version"].as<uint32_t>();
			uint32_t maxEntities = prefabProperties["MaxEntities"].as<uint32_t>();
			uint32_t maxComponents = prefabProperties["MaxComponents"].as<uint32_t>();

			auto prefabHandle = ResourceManager::Instance->CreatePrefab({
				.debugName = prefabName.c_str(),
				.uuid = asset->UUID,
				.baseEntityUUID = baseEntityUUID,
				.version = version,
				.maxEntities = maxEntities,
				.maxComponents = maxComponents,
			});

			Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

			if (prefab == nullptr)
			{
				HBL2_CORE_ERROR("Prefab asset \"{0}\" is invalid, aborting prefab load.", asset->FilePath.filename().stem().string());
				return Handle<Prefab>();
			}

			// Deserialize the source prefab into the prefabs' sub-scene.
			PrefabSerializer prefabSerializer(prefab);
			prefabSerializer.Deserialize(Project::GetAssetFileSystemPath(asset->FilePath));

			return prefabHandle;
		}

		return Handle<Prefab>();
	}

	Handle<Mesh> EditorAssetManager::ImportMesh(Asset* asset)
	{
		const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetAssetDirectory().parent_path().parent_path();
		const std::filesystem::path& meshPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : workingDirectory / asset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

		std::ifstream stream(meshPath.string() + ".hblmesh");
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Mesh"].IsDefined())
		{
			HBL2_CORE_TRACE("Mesh not found: {0}", ss.str());
			stream.close();

			return Handle<Mesh>();
		}

		auto meshProperties = data["Mesh"];
		if (meshProperties)
		{
			Handle<Mesh> mesh = MeshUtilities::Get().Load(meshPath);
			stream.close();
			return mesh;
		}

		stream.close();

		return Handle<Mesh>();
	}

	/// Reimport methods

	Handle<Shader> EditorAssetManager::ReimportShader(Asset* asset)
	{
		Handle<Shader> shaderHandle = Handle<Shader>::UnPack(asset->Indentifier);

		const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetAssetDirectory().parent_path().parent_path();
		const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : workingDirectory / asset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

		std::ifstream stream(shaderPath.string() + ".hblshader");

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Shader metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblshader");
			return Handle<Shader>();
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Shader"].IsDefined())
		{
			HBL2_CORE_ERROR("Shader not found: {0}", asset->DebugName);
			stream.close();
			return Handle<Shader>();
		}

		const std::string& shaderName = asset->FilePath.filename().stem().string();

		Handle<BindGroupLayout> globalBindGroupLayout;
		Handle<BindGroupLayout> drawBindGroupLayout;

		StaticDArray<ShaderDescriptor::RenderPipeline::PackedVariant, 16> shaderVariants;

		const auto& shaderProperties = data["Shader"];
		if (shaderProperties)
		{
			uint32_t type = shaderProperties["Type"].as<uint32_t>();

			switch (type)
			{
			case 0:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout2D();
				break;
			case 1:
			case 2:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout3D();
				break;
			default:
				HBL2_CORE_ERROR("Unknown Shader type: {0}", asset->DebugName);
				stream.close();
				return Handle<Shader>();
			}

			// Retrieve shader variants.
			const auto& shaderVariantsProperty = shaderProperties["Variants"];

			if (shaderVariantsProperty)
			{
				for (const YAML::Node& variantNode : shaderVariantsProperty)
				{
					ShaderDescriptor::RenderPipeline::PackedVariant variant = {};

					// Retrieve raster state.
					const auto& rasterStateProp = variantNode["RasterState"];

					if (rasterStateProp)
					{
						variant.topology = (packed_size)(Topology)rasterStateProp["Topology"].as<int>();
						variant.polygonMode = (packed_size)(PolygonMode)rasterStateProp["PolygonMode"].as<int>();
						variant.cullMode = (packed_size)(CullMode)rasterStateProp["CullMode"].as<int>();
						variant.frontFace = (packed_size)(FrontFace)rasterStateProp["FrontFace"].as<int>();
					}

					// Retrieve blend state.
					const auto& blendStateProp = variantNode["BlendState"];

					if (blendStateProp)
					{
						variant.colorOutput = blendStateProp["ColorOutputEnabled"].as<bool>();
						variant.blendEnabled = blendStateProp["Enabled"].as<bool>();

						// TODO: Remove if statements in the future.
						if (blendStateProp["ColorOp"].IsDefined())			variant.colorOp = (packed_size)(BlendOperation)blendStateProp["ColorOp"].as<int>();
						if (blendStateProp["AlphaOp"].IsDefined())			variant.alphaOp = (packed_size)(BlendOperation)blendStateProp["AlphaOp"].as<int>();
						if (blendStateProp["SrcColorFactor"].IsDefined())	variant.srcColorFactor = (packed_size)(BlendFactor)blendStateProp["SrcColorFactor"].as<int>();
						if (blendStateProp["DstColorFactor"].IsDefined())	variant.dstColorFactor = (packed_size)(BlendFactor)blendStateProp["DstColorFactor"].as<int>();
						if (blendStateProp["SrcAlphaFactor"].IsDefined())	variant.srcAlphaFactor = (packed_size)(BlendFactor)blendStateProp["SrcAlphaFactor"].as<int>();
						if (blendStateProp["DstAlphaFactor"].IsDefined())	variant.dstAlphaFactor = (packed_size)(BlendFactor)blendStateProp["DstAlphaFactor"].as<int>();
					}

					// Retrieve depth state.
					const auto& depthStateProp = variantNode["DepthState"];

					if (depthStateProp)
					{
						variant.depthEnabled = depthStateProp["Enabled"].as<bool>();
						variant.depthWrite = depthStateProp["WriteEnabled"].as<bool>();
						variant.stencilEnabled = depthStateProp["StencilEnabled"].as<bool>();
						variant.depthCompare = (packed_size)(Compare)depthStateProp["DepthTest"].as<int>();
					}

					// Retrieve shader constants
					const auto& shaderConstantsStateProp = variantNode["ShaderConstantsState"];

					if (shaderConstantsStateProp)
					{
						variant.shaderConstantBool0 = shaderConstantsStateProp["ShaderConstantBool0"].as<bool>();
						variant.shaderConstantBool1 = shaderConstantsStateProp["ShaderConstantBool1"].as<bool>();
						variant.shaderConstantBool2 = shaderConstantsStateProp["ShaderConstantBool2"].as<bool>();
						variant.shaderConstantBool3 = shaderConstantsStateProp["ShaderConstantBool3"].as<bool>();
						variant.shaderConstantBool4 = shaderConstantsStateProp["ShaderConstantBool4"].as<bool>();
						variant.shaderConstantBool5 = shaderConstantsStateProp["ShaderConstantBool5"].as<bool>();
						variant.shaderConstantBool6 = shaderConstantsStateProp["ShaderConstantBool6"].as<bool>();
						variant.shaderConstantBool7 = shaderConstantsStateProp["ShaderConstantBool7"].as<bool>();
					}

					shaderVariants.push_back(variant);
				}
			}
		}

		// Compile Shader.
		ShaderReflectionData outReflectionData;
		const auto& compilationData = ShaderUtilities::Get().Compile(shaderPath.string(), &outReflectionData, true);

		if (!compilationData.IsValid())
		{
			HBL2_CORE_ERROR("Shader asset: {0}, at path: {1}, could not be compiled. Returning invalid shader.", asset->DebugName, shaderPath.string());
			stream.close();
			return ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::INVALID);
		}

		// Retrieve shader bind group.
		Handle<BindGroup> shaderBindGroup;
		if (shaderProperties && shaderProperties["BindGroup"].IsDefined())
		{
			for (const auto& descriptorSet : outReflectionData.descriptorSets)
			{
				if (descriptorSet.set != 1)
				{
					continue;
				}

				StaticDArray<BindGroupDescriptor::TextureEntry, 8> textureBindings;
				StaticDArray<BindGroupDescriptor::BufferEntry, 8> bufferBindings;

				uint32_t bindingIndex = 0;

				for (const auto& b : descriptorSet.bindings)
				{
					if (b.type == ResourceType::UniformBuffer)
					{
						std::vector<uint8_t> uniformBufferBytes(b.size);

						const auto& bufferProp = shaderProperties["BindGroup"][bindingIndex];

						if (bufferProp.IsDefined())
						{
							for (const auto& m : b.members)
							{
								const auto& memberProp = bufferProp[b.name][m.name];

								if (!memberProp.IsDefined())
								{
									continue;
								}

								uint8_t* memberPtr = uniformBufferBytes.data() + m.offset;

								switch (m.typeInfo.base)
								{
								case MemberBaseType::Float:
								{
									if (m.typeInfo.isArray)
									{
										const uint32_t stride = m.typeInfo.arrayCount > 0 ? m.size / m.typeInfo.arrayCount : 0;

										for (uint32_t i = 0; i < m.typeInfo.arrayCount; ++i)
										{
											if (!memberProp[i].IsDefined())
											{
												continue;
											}

											float* f = reinterpret_cast<float*>(memberPtr + i * stride);

											if (m.typeInfo.cols == 1)
											{
												*f = memberProp[i].as<float>();
											}
											else if (m.typeInfo.cols == 2)
											{
												const glm::vec2& vec2 = memberProp[i].as<glm::vec2>();


												f[0] = vec2.x;
												f[1] = vec2.y;
											}
											else if (m.typeInfo.cols == 3)
											{
												const glm::vec3& vec3 = memberProp[i].as<glm::vec3>();

												f[0] = vec3.x;
												f[1] = vec3.y;
												f[2] = vec3.z;
											}
											else if (m.typeInfo.cols == 4)
											{
												const glm::vec4& vec4 = memberProp[i].as<glm::vec4>();

												f[0] = vec4.x;
												f[1] = vec4.y;
												f[2] = vec4.z;
												f[3] = vec4.w;
											}
										}
									}
									else
									{
										float* f = reinterpret_cast<float*>(memberPtr);

										if (m.typeInfo.cols == 1)
										{
											*f = memberProp.as<float>();
										}
										else if (m.typeInfo.cols == 2)
										{
											const glm::vec2& vec2 = memberProp.as<glm::vec2>();

											f[0] = vec2.x;
											f[1] = vec2.y;
										}
										else if (m.typeInfo.cols == 3)
										{
											const glm::vec3& vec3 = memberProp.as<glm::vec3>();

											f[0] = vec3.x;
											f[1] = vec3.y;
											f[2] = vec3.z;
										}
										else if (m.typeInfo.cols == 4)
										{
											const glm::vec4& vec4 = memberProp.as<glm::vec4>();

											f[0] = vec4.x;
											f[1] = vec4.y;
											f[2] = vec4.z;
											f[3] = vec4.w;
										}
									}
									break;
								}
								}
							}

							auto userBuffer = ResourceManager::Instance->CreateBuffer({
								.debugName = "shader-uniform-buffer",
								.usage = BufferUsage::UNIFORM,
								.usageHint = BufferUsageHint::DYNAMIC,
								.memoryUsage = MemoryUsage::GPU_CPU,
								.byteSize = (uint32_t)b.size,
								.initialData = (void*)uniformBufferBytes.data(),
							});

							bufferBindings.push_back({ .buffer = userBuffer, });
						}
					}
					else if (b.type == ResourceType::SampledTexture)
					{
						const auto& textureProp = shaderProperties["BindGroup"][bindingIndex];

						if (textureProp.IsDefined())
						{
							UUID textureMapUUID = textureProp[b.name].as<UUID>();

							auto handle = AssetManager::Instance->GetAsset<Texture>(textureMapUUID);
							textureBindings.push_back({ handle });
						}
					}

					bindingIndex++;
				}

				// If there is only one texture and is not set, use the built in white texture.
				if (textureBindings.size() == 1)
				{
					if (!textureBindings[0].texture.IsValid())
					{
						textureBindings[0].texture = TextureUtilities::Get().WhiteTexture;
					}
				}

				shaderBindGroup = ResourceManager::Instance->CreateBindGroup({
					.debugName = "shader-bind-group",
					.layout = outReflectionData.GetBindGroupLayout(1),
                    .textures = { textureBindings.data(), textureBindings.size() },
                    .buffers = { bufferBindings.data(), bufferBindings.size() },
				});
			}
		}
		else
		{
			HBL2_CORE_TRACE("Shader {0} has an ill-formed metadata file.", shaderPath);
		}

		// Create resource.
		ResourceManager::Instance->RecompileShader(shaderHandle, {
			.debugName = strdup(std::format("{}-shader", shaderName).c_str()),
			.VS { .code = compilationData.vertexShaderCode.AsSpan(), .entryPoint = outReflectionData.entryPoints[0].name.c_str() },
			.FS { .code = compilationData.fragmentShaderCode.AsSpan(), .entryPoint = outReflectionData.entryPoints[1].name.c_str() },
			.bindGroups {
				globalBindGroupLayout,							// Global bind group			(0)
				outReflectionData.GetBindGroupLayout(1),		// Global user bind group		(1)
				outReflectionData.GetBindGroupLayout(2),		// Material / user bind group	(2)
				Renderer::Instance->GetDynamicBindingsLayout(),	// Draw bind group				(3)
			},
			.renderPipeline {
				.vertexBufferBindings = outReflectionData.vertexBufferBindings,
				.variants = { shaderVariants.data(), shaderVariants.size() },
                .specializationConstantsPerVariant = outReflectionData.GetSpecializationConstantsPerVariant({ shaderVariants.data(), shaderVariants.size() }),
			},
			.renderPass = Renderer::Instance->GetRenderingRenderPass(),
			.shaderBindGroup = shaderBindGroup,
		});

		stream.close();
		return shaderHandle;
	}

	Handle<Material> EditorAssetManager::ReimportMaterial(Asset* asset)
	{
		Handle<Material> materialHandle = Handle<Material>::UnPack(asset->Indentifier);

		const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetProjectDirectory().parent_path();
		const std::filesystem::path& materialPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : workingDirectory / asset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

		if (!materialHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return Handle<Material>();
		}

		Material* mat = ResourceManager::Instance->GetMaterial(materialHandle);
		const std::string& materialName = asset->FilePath.filename().stem().string();

		std::fstream ioStream(Project::GetAssetFileSystemPath(asset->FilePath), std::ios::in | std::ios::out);

		if (!ioStream.is_open())
		{
			HBL2_CORE_ERROR("Material file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath));
			return Handle<Material>();
		}

		std::stringstream ss;
		ss << ioStream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Material"].IsDefined())
		{
			HBL2_CORE_TRACE("Material file: {0}, is not in correct format!", ss.str());
			ioStream.close();
			return Handle<Material>();
		}

		auto materialProperties = data["Material"];
		if (materialProperties)
		{
			// Shader reload.
			UUID shaderUUID = materialProperties["Shader"].as<UUID>();
			Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderUUID);

			// If the shader is the same, reload it first.
			if (shaderHandle == mat->Shader)
			{
				shaderHandle = ReloadAsset<Shader>(shaderUUID);
			}

			// Now that we have the up to date shader, do a full reimport of the material.
			uint32_t type = UINT32_MAX;
			bool autoImported = false;

			// Gather metadata.
			{
				std::ifstream metaDataStream(materialPath.string() + ".hblmat");

				if (!metaDataStream.is_open())
				{
					HBL2_CORE_ERROR("Material metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblmat");
					return Handle<Material>();
				}

				std::stringstream ssMetadata;
				ssMetadata << metaDataStream.rdbuf();

				YAML::Node dataMetadata = YAML::Load(ssMetadata.str());
				if (!dataMetadata["Material"].IsDefined())
				{
					HBL2_CORE_TRACE("Material not found in metadata file: {0}", ssMetadata.str());
					metaDataStream.close();
					return Handle<Material>();
				}

				auto materialMetadataProperties = dataMetadata["Material"];
				if (materialMetadataProperties)
				{
					type = materialMetadataProperties["Type"].as<uint32_t>();
					autoImported = materialMetadataProperties["AutoImported"].as<bool>();
				}

				metaDataStream.close();
			}

			auto shaderAssetHandle = AssetManager::Instance->GetHandleFromUUID(shaderUUID);
			Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(shaderAssetHandle);

			const auto& filesystemPath = Project::GetAssetFileSystemPath(shaderAsset->FilePath);
			const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : workingDirectory / shaderAsset->FilePath;

			// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
			//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

			auto shaderReflectionData = ShaderUtilities::Get().Reflect(shaderPath.string());

			Handle<BindGroup> materialBindGroup;

			for (const auto& descriptorSet : shaderReflectionData.descriptorSets)
			{
				if (descriptorSet.set != 2)
				{
					continue;
				}

				StaticDArray<BindGroupDescriptor::TextureEntry, 8> textureBindings;
				StaticDArray<BindGroupDescriptor::BufferEntry, 8> bufferBindings;

				for (const auto& b : descriptorSet.bindings)
				{
					if (b.type == ResourceType::UniformBuffer)
					{
						std::vector<uint8_t> uniformBufferBytes(b.size);

						const auto& bufferProp = materialProperties[b.name];

						if (bufferProp.IsDefined())
						{
							for (const auto& m : b.members)
							{
								const auto& memberProp = bufferProp[m.name];

								if (!memberProp.IsDefined())
								{
									continue;
								}

								uint8_t* memberPtr = uniformBufferBytes.data() + m.offset;

								switch (m.typeInfo.base)
								{
								case MemberBaseType::Float:
									{
										if (m.typeInfo.isArray)
										{
											const uint32_t stride = m.typeInfo.arrayCount > 0 ? m.size / m.typeInfo.arrayCount : 0;

											for (uint32_t i = 0; i < m.typeInfo.arrayCount; ++i)
											{
												if (!memberProp[i].IsDefined())
												{
													continue;
												}

												float* f = reinterpret_cast<float*>(memberPtr + i * stride);

												if (m.typeInfo.cols == 1)
												{
													*f = memberProp[i].as<float>();
												}
												else if (m.typeInfo.cols == 2)
												{
													const glm::vec2& vec2 = memberProp[i].as<glm::vec2>();

													f[0] = vec2.x;
													f[1] = vec2.y;
												}
												else if (m.typeInfo.cols == 3)
												{
													const glm::vec3& vec3 = memberProp[i].as<glm::vec3>();

													f[0] = vec3.x;
													f[1] = vec3.y;
													f[2] = vec3.z;
												}
												else if (m.typeInfo.cols == 4)
												{
													const glm::vec4& vec4 = memberProp[i].as<glm::vec4>();

													f[0] = vec4.x;
													f[1] = vec4.y;
													f[2] = vec4.z;
													f[3] = vec4.w;
												}
											}
										}
										else
										{
											float* f = reinterpret_cast<float*>(memberPtr);

											if (m.typeInfo.cols == 1)
											{
												*f = memberProp.as<float>();
											}
											else if (m.typeInfo.cols == 2)
											{
												const glm::vec2& vec2 = memberProp.as<glm::vec2>();

												f[0] = vec2.x;
												f[1] = vec2.y;
											}
											else if (m.typeInfo.cols == 3)
											{
												const glm::vec3& vec3 = memberProp.as<glm::vec3>();

												f[0] = vec3.x;
												f[1] = vec3.y;
												f[2] = vec3.z;
											}
											else if (m.typeInfo.cols == 4)
											{
												const glm::vec4& vec4 = memberProp.as<glm::vec4>();

												f[0] = vec4.x;
												f[1] = vec4.y;
												f[2] = vec4.z;
												f[3] = vec4.w;
											}
										}
										break;
									}
								}
							}

							auto userBuffer = ResourceManager::Instance->CreateBuffer({
								.debugName = "user-uniform-buffer",
								.usage = BufferUsage::UNIFORM,
								.usageHint = BufferUsageHint::DYNAMIC,
								.memoryUsage = MemoryUsage::GPU_CPU,
								.byteSize = (uint32_t)b.size,
								.initialData = (void*)uniformBufferBytes.data(),
							});

							bufferBindings.push_back({ .buffer = userBuffer, });
						}
					}
					else if (b.type == ResourceType::SampledTexture)
					{
						const auto& textureProp = materialProperties[b.name];

						if (textureProp.IsDefined())
						{
							UUID textureMapUUID = textureProp.as<UUID>();

							auto handle = AssetManager::Instance->GetAsset<Texture>(textureMapUUID);
							textureBindings.push_back({ handle });
						}
					}
				}

				// If there is only one texture and is not set, use the built in white texture.
				if (textureBindings.size() == 1)
				{
					if (!textureBindings[0].texture.IsValid())
					{
						textureBindings[0].texture = TextureUtilities::Get().WhiteTexture;
					}
				}

				materialBindGroup = ResourceManager::Instance->CreateBindGroup({
					.debugName = "user-bind-group",
					.layout = shaderReflectionData.GetBindGroupLayout(2),
                    .textures = { textureBindings.data(), textureBindings.size() },
                    .buffers = { bufferBindings.data(), bufferBindings.size() },
				});
			}

			// Delete old bind group.
			ResourceManager::Instance->DeleteBindGroup(mat->MaterialBindGroup);

			ResourceManager::Instance->ReimportMaterial(materialHandle, {
				.debugName = strdup(std::format("{}-material", materialName).c_str()),
				.shader = shaderHandle,
				.drawBindGroup = mat->DrawBindGroup,
				.materialBindGroup = materialBindGroup,
			});
		}

		ioStream.close();
		return materialHandle;
	}

	Handle<Mesh> EditorAssetManager::ReimportMesh(Asset* asset)
	{
		Handle<Mesh> meshHandle = Handle<Mesh>::UnPack(asset->Indentifier);
		Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);

		if (mesh == nullptr)
		{
			return {};
		}

		mesh->MarkAsEmpty();

		const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& workingDirectory = Project::GetAssetDirectory().parent_path().parent_path();
		const std::filesystem::path& meshPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : workingDirectory / asset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

		std::ifstream stream(meshPath.string() + ".hblmesh");
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Mesh"].IsDefined())
		{
			HBL2_CORE_TRACE("Mesh not found: {0}", ss.str());
			stream.close();

			return Handle<Mesh>();
		}

		auto meshProperties = data["Mesh"];
		if (meshProperties)
		{
			MeshUtilities::Get().Reload(asset);
		}

		stream.close();
		return meshHandle;
	}

	Handle<Prefab> EditorAssetManager::ReimportPrefab(Asset* asset)
	{
		return Handle<Prefab>();
	}

	/// Save methods

	void EditorAssetManager::SaveMaterial(Asset* asset)
	{
		Handle<Material> materialHandle = Handle<Material>::UnPack(asset->Indentifier);

		if (!materialHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return;
		}

		Material* mat = ResourceManager::Instance->GetMaterial(materialHandle);

		std::fstream ioStream(Project::GetAssetFileSystemPath(asset->FilePath), std::ios::in | std::ios::out);

		if (!ioStream.is_open())
		{
			HBL2_CORE_ERROR("Material file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath));
			return;
		}

		std::stringstream ss;
		ss << ioStream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Material"].IsDefined())
		{
			HBL2_CORE_TRACE("Material file: {0}, is not in correct format!", ss.str());
			ioStream.close();
			return;
		}

		auto materialProperties = data["Material"];
		if (materialProperties)
		{
			if (materialProperties["RasterState"].IsDefined())
			{
				materialProperties["RasterState"]["Topology"] = (int)mat->VariantHash.topology;
				materialProperties["RasterState"]["PolygonMode"] = (int)mat->VariantHash.polygonMode;
				materialProperties["RasterState"]["CullMode"] = (int)mat->VariantHash.cullMode;
				materialProperties["RasterState"]["FrontFace"] = (int)mat->VariantHash.frontFace;
			}

			materialProperties["BlendState"]["Enabled"] = (bool)mat->VariantHash.blendEnabled;
			materialProperties["BlendState"]["ColorOutputEnabled"] = (bool)mat->VariantHash.colorOutput;

			materialProperties["DepthState"]["Enabled"] = (bool)mat->VariantHash.depthEnabled;
			materialProperties["DepthState"]["WriteEnabled"] = (bool)mat->VariantHash.depthWrite;
			materialProperties["DepthState"]["StencilEnabled"] = (bool)mat->VariantHash.stencilEnabled;
			materialProperties["DepthState"]["DepthTest"] = (int)mat->VariantHash.depthCompare;

			if (materialProperties["ShaderConstantsState"].IsDefined())
			{
				materialProperties["ShaderConstantsState"]["ShaderConstantBool0"] = (bool)mat->VariantHash.shaderConstantBool0;
				materialProperties["ShaderConstantsState"]["ShaderConstantBool1"] = (bool)mat->VariantHash.shaderConstantBool1;
				materialProperties["ShaderConstantsState"]["ShaderConstantBool2"] = (bool)mat->VariantHash.shaderConstantBool2;
				materialProperties["ShaderConstantsState"]["ShaderConstantBool3"] = (bool)mat->VariantHash.shaderConstantBool3;
				materialProperties["ShaderConstantsState"]["ShaderConstantBool4"] = (bool)mat->VariantHash.shaderConstantBool4;
				materialProperties["ShaderConstantsState"]["ShaderConstantBool5"] = (bool)mat->VariantHash.shaderConstantBool5;
				materialProperties["ShaderConstantsState"]["ShaderConstantBool6"] = (bool)mat->VariantHash.shaderConstantBool6;
				materialProperties["ShaderConstantsState"]["ShaderConstantBool7"] = (bool)mat->VariantHash.shaderConstantBool7;
			}

			UUID shaderUUID = materialProperties["Shader"].as<UUID>();
			Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderUUID);

			ResourceManager::Instance->GetOrAddShaderVariant(shaderHandle, mat->VariantHash);
			ShaderUtilities::Get().UpdateShaderVariantMetadataFile(shaderUUID, mat->VariantHash);
		}

		ioStream.seekg(0, std::ios::beg);
		ioStream << data;

		ioStream.close();
	}

	void EditorAssetManager::SaveScene(Asset* asset)
	{
		if (!asset->Loaded)
		{
			HBL2_CORE_WARN(" Scene: {0}, at path: {1} is not loaded, loading it now.", asset->DebugName, asset->FilePath.string());

			const auto& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath);

			try
			{
				std::filesystem::create_directories(filePath.parent_path());
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Project directory creation failed: {0}", e.what());
			}

			SceneDescriptor desc =
			{
				.name = asset->FilePath.filename().stem().string(),
			};
			

			std::ofstream fout(filePath.string() + ".hblscene", std::ios_base::out);

			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Scene" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
			out << YAML::Key << "MaxEntities" << YAML::Value << desc.maxEntities;
			out << YAML::Key << "MaxComponents" << YAML::Value << desc.maxComponents;
			out << YAML::Key << "MaxSystems" << YAML::Value << desc.maxSystems;
			out << YAML::Key << "MaxJobsPerSystem" << YAML::Value << desc.maxJobsPerSystem;
			out << YAML::Key << "MaxStructuralCommandsPerFramePerThread" << YAML::Value << desc.maxStructuralCommandsPerFramePerThread;
			out << YAML::Key << "UseStructuralCommandBuffer" << YAML::Value << desc.useStructuralCommandBuffer;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();

			auto sceneHandle = ResourceManager::Instance->CreateScene(std::move(desc));

			Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Scene asset \"{0}\" is invalid, aborting scene load.", asset->FilePath.filename().stem().string());
				return;
			}

			scene->RegisterSystem<HierachySystem>();
			scene->RegisterSystem<CameraSystem>(SystemType::Runtime);
			scene->RegisterSystem<TerrainSystem>();
			scene->RegisterSystem<RenderingSystem>();
			scene->RegisterSystem<SoundSystem>(SystemType::Runtime);
			scene->RegisterSystem<Physics2dSystem>(SystemType::Runtime);
			scene->RegisterSystem<Physics3dSystem>(SystemType::Runtime);
			scene->RegisterSystem<AnimationCurveSystem>();

			asset->Indentifier = sceneHandle.Pack();
			asset->Loaded = true;
		}

		Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);

		if (!sceneHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return;
		}

		Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);
		SceneSerializer serializer(scene);
		serializer.Serialize(Project::GetAssetFileSystemPath(asset->FilePath));
	}

	void EditorAssetManager::SaveTexture(Asset* asset)
	{
		Handle<Texture> textureHandle = Handle<Texture>::UnPack(asset->Indentifier);

		if (!textureHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return;
		}

		// Open texture metadata file.
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture");

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Texture metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture");
			return;
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Texture"].IsDefined())
		{
			HBL2_CORE_ERROR("Texture not found: {0}", asset->DebugName);
			stream.close();
			return;
		}

		auto textureProperties = data["Texture"];
		if (textureProperties)
		{
			// Load the texture to get current pixel data.
			TextureSettings textureSettings =
			{
				.Flip = textureProperties["Flip"].as<bool>(),
			};
			void* textureData = TextureUtilities::Get().Load(Project::GetAssetFileSystemPath(asset->FilePath).string(), textureSettings);

			// Update gpu texture storage.
			ResourceManager::Instance->UpdateTexture(textureHandle, { (std::byte*)textureData, (size_t)(textureSettings.Width * textureSettings.Height) });

			// Free the cpu side pixel data since they are copied by the driver.
			stbi_image_free(textureData);
		}

		stream.close();
	}

	void EditorAssetManager::SaveScript(Asset* asset)
	{
		Handle<Script> scriptHandle = Handle<Script>::UnPack(asset->Indentifier);

		if (!scriptHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return;
		}

		Script* script = ResourceManager::Instance->GetScript(scriptHandle);

		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		std::vector<std::string> userSystemNames;

		// Store registered user system names.
		for (ISystem* userSystem : activeScene->GetRuntimeSystems())
		{
			if (userSystem->GetType() == SystemType::User)
			{
				userSystemNames.push_back(userSystem->Name);
			}
		}

		std::vector<std::string> userComponentNames;
		Reflect::TypeEntry::ByteStorage data;

		// Store all registered meta types.
		Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
		{
			entry.serialize(&activeScene->GetRegistry(), data, true);
		});

		// Unload unity build dll.
		BuildEngine::Instance->UnloadBuild(activeScene);

		// Build unity build source dll.
		BuildEngine::Instance->Build();

		// Re-register systems.
		for (const auto& userSystemName : userSystemNames)
		{
			BuildEngine::Instance->RegisterSystem(userSystemName, activeScene);
		}

		bool newComponentTobeRegistered = true;

		// Re-register the components.
		for (const auto& userComponentName : userComponentNames)
		{
			if (script->Type == ScriptType::COMPONENT)
			{
				if (userComponentName == script->Name)
				{
					newComponentTobeRegistered = false;
				}
			}

			BuildEngine::Instance->RegisterComponent(userComponentName, activeScene);
		}

		Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
		{
			entry.deserialize(&activeScene->GetRegistry(), data);
		});

		if (newComponentTobeRegistered && script->Type == ScriptType::COMPONENT)
		{
			BuildEngine::Instance->RegisterComponent(script->Name, activeScene);
		}
	}

	void EditorAssetManager::SaveSound(Asset* asset)
	{
		Handle<Sound> soundHandle = Handle<Sound>::UnPack(asset->Indentifier);

		if (!soundHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return;
		}

		Sound* sound = ResourceManager::Instance->GetSound(soundHandle);

		std::ifstream ifStream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblsound", std::ios::in);

		if (!ifStream.is_open())
		{
			HBL2_CORE_ERROR("Sound file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblsound");
			return;
		}

		std::stringstream ss;
		ss << ifStream.rdbuf();
		ifStream.close();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Sound"].IsDefined())
		{
			HBL2_CORE_TRACE("Sound not found: {0}", ss.str());
			ifStream.close();
			return;
		}

		std::ofstream ofStream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblsound", std::ios::out);

		if (!ofStream.is_open())
		{
			HBL2_CORE_ERROR("Sound file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblsound");
			return;
		}

		ofStream << data;
		ofStream.close();
	}

	void EditorAssetManager::SavePrefab(Asset* asset)
	{
		Handle<Prefab> prefabHandle = Handle<Prefab>::UnPack(asset->Indentifier);

		if (!prefabHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return;
		}

		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		PrefabSerializer serializer(prefab);
		serializer.Serialize(Project::GetAssetFileSystemPath(asset->FilePath));
	}

	/// Destroy methods

	bool EditorAssetManager::DestroyTexture(Asset* asset)
	{
		if (asset->Loaded)
		{
			UnloadTexture(asset);
		}

		// Delete files
		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath)))
			{
				HBL2_CORE_INFO("File {} deleted.", asset->FilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", asset->FilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), asset->FilePath);
			return false;
		}

		const std::string& metafilePath = asset->FilePath.string() + ".hbltexture";

		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture"))
			{
				HBL2_CORE_INFO("File {} deleted.", metafilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", metafilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), metafilePath);
			return false;
		}

		return true;
	}

	bool EditorAssetManager::DestroyShader(Asset* asset)
	{
		if (asset->Loaded)
		{
			UnloadShader(asset);
		}

		// Delete files
		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath)))
			{
				HBL2_CORE_INFO("File {} deleted.", asset->FilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", asset->FilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), asset->FilePath);
			return false;
		}

		const std::string& metafilePath = asset->FilePath.string() + ".hblshader";

		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblshader"))
			{
				HBL2_CORE_INFO("File {} deleted.", metafilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", metafilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), metafilePath);
			return false;
		}

		return true;
	}

	bool EditorAssetManager::DestroyMaterial(Asset* asset)
	{
		if (asset->Loaded)
		{
			UnloadMaterial(asset);
		}

		// Delete files
		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath)))
			{
				HBL2_CORE_INFO("File {} deleted.", asset->FilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", asset->FilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), asset->FilePath);
			return false;
		}

		const std::string& metafilePath = asset->FilePath.string() + ".hblmat";

		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblmat"))
			{
				HBL2_CORE_INFO("File {} deleted.", metafilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", metafilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), metafilePath);
			return false;
		}

		return true;
	}

	bool EditorAssetManager::DestroyMesh(Asset* asset)
	{
		if (asset->Loaded)
		{
			UnloadMesh(asset);
		}

		// Delete files
		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath)))
			{
				HBL2_CORE_INFO("File {} deleted.", asset->FilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", asset->FilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), asset->FilePath);
			return false;
		}

		const std::string& metafilePath = asset->FilePath.string() + ".hblmesh";

		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblmesh"))
			{
				HBL2_CORE_INFO("File {} deleted.", metafilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", metafilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), metafilePath);
			return false;
		}

		return true;
	}

	bool EditorAssetManager::DestroyScript(Asset* asset)
	{
		if (asset->Loaded)
		{
			UnloadScript(asset);
		}

		// Delete files
		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath)))
			{
				HBL2_CORE_INFO("File {} deleted.", asset->FilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", asset->FilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), asset->FilePath);
			return false;
		}

		const std::string& metafilePath = asset->FilePath.string() + ".hblscript";

		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblscript"))
			{
				HBL2_CORE_INFO("File {} deleted.", metafilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", metafilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), metafilePath);
			return false;
		}

		return true;
	}

	bool EditorAssetManager::DestroyScene(Asset* asset)
	{
		if (asset->Loaded)
		{
			UnloadScene(asset);
		}

		Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);

		if (sceneHandle == Context::ActiveScene)
		{
			HBL2_CORE_WARN("Scene asset \"{0}\" is currently open, skipping destroy operation. Close it and then destroy.", asset->DebugName);
			return false;
		}

		// Delete files
		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath)))
			{
				HBL2_CORE_INFO("File {} deleted.", asset->FilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", asset->FilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), asset->FilePath);
			return false;
		}

		const std::string& metafilePath = asset->FilePath.string() + ".hblscene";

		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblscene"))
			{
				HBL2_CORE_INFO("File {} deleted.", metafilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", metafilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), metafilePath);
			return false;
		}

		return true;
	}

	bool EditorAssetManager::DestroySound(Asset* asset)
	{
		if (asset->Loaded)
		{
			UnloadSound(asset);
		}

		// Delete files
		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath)))
			{
				HBL2_CORE_INFO("File {} deleted.", asset->FilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", asset->FilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), asset->FilePath);
			return false;
		}

		const std::string& metafilePath = asset->FilePath.string() + ".hblsound";

		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblsound"))
			{
				HBL2_CORE_INFO("File {} deleted.", metafilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", metafilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), metafilePath);
			return false;
		}

		return true;
	}

	bool EditorAssetManager::DestroyPrefab(Asset* asset)
	{
		if (asset->Loaded)
		{
			UnloadPrefab(asset);
		}

		// Delete files
		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath)))
			{
				HBL2_CORE_INFO("File {} deleted.", asset->FilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", asset->FilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), asset->FilePath);
			return false;
		}

		const std::string& metafilePath = asset->FilePath.string() + ".hblprefab";

		try
		{
			if (std::filesystem::remove(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblprefab"))
			{
				HBL2_CORE_INFO("File {} deleted.", metafilePath);
			}
			else
			{
				HBL2_CORE_WARN("File {} not found.", metafilePath);
			}
		}
		catch (const std::filesystem::filesystem_error& err)
		{
			HBL2_CORE_ERROR("Filesystem error: {}, when trying to delete {}.", err.what(), metafilePath);
			return false;
		}

		return true;
	}

	/// Unload methods

	void EditorAssetManager::UnloadTexture(Asset* asset)
	{
		Handle<Texture> textureAssetHandle = Handle<Texture>::UnPack(asset->Indentifier);

		if (!textureAssetHandle.IsValid())
		{
			HBL2_CORE_WARN("Asset \"{0}\" has an invalid handle, skipping unload operation.", asset->DebugName);
			return;
		}

		if (!asset->Loaded)
		{
			HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
			return;
		}

		ResourceManager::Instance->DeleteTexture(textureAssetHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}

	void EditorAssetManager::UnloadShader(Asset* asset)
	{
		Handle<Shader> shaderAssetHandle = Handle<Shader>::UnPack(asset->Indentifier);

		if (!shaderAssetHandle.IsValid())
		{
			HBL2_CORE_WARN("Asset \"{0}\" has an invalid handle, skipping unload operation.", asset->DebugName);
			return;
		}

		if (!asset->Loaded)
		{
			HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
			return;
		}

		ResourceManager::Instance->DeleteShader(shaderAssetHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}

	void EditorAssetManager::UnloadMesh(Asset* asset)
	{
		Handle<Mesh> meshAssetHandle = Handle<Mesh>::UnPack(asset->Indentifier);

		if (!meshAssetHandle.IsValid())
		{
			HBL2_CORE_WARN("Asset \"{0}\" has an invalid handle, skipping unload operation.", asset->DebugName);
			return;
		}

		if (!asset->Loaded)
		{
			HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
			return;
		}

		Mesh* mesh = ResourceManager::Instance->GetMesh(meshAssetHandle);

		if (mesh != nullptr)
		{
			for (auto& meshPart : mesh->Meshes)
			{
				ResourceManager::Instance->DeleteBuffer(meshPart.IndexBuffer);

				for (const auto vertexBuffer : meshPart.VertexBuffers)
				{
					ResourceManager::Instance->DeleteBuffer(vertexBuffer);
				}
			}
		}

		ResourceManager::Instance->DeleteMesh(meshAssetHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}

	void EditorAssetManager::UnloadMaterial(Asset* asset)
	{
		Handle<Material> materialAssetHandle = Handle<Material>::UnPack(asset->Indentifier);

		if (!materialAssetHandle.IsValid())
		{
			HBL2_CORE_WARN("Asset \"{0}\" has an invalid handle, skipping unload operation.", asset->DebugName);
			return;
		}

		if (!asset->Loaded)
		{
			HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
			return;
		}

		Material* material = ResourceManager::Instance->GetMaterial(materialAssetHandle);

		if (material != nullptr)
		{
			ResourceManager::Instance->DeleteBindGroup(material->DrawBindGroup);
			ResourceManager::Instance->DeleteBindGroup(material->MaterialBindGroup);
		}

		ResourceManager::Instance->DeleteMaterial(materialAssetHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}

	void EditorAssetManager::UnloadScript(Asset* asset)
	{
		Handle<Script> scriptAssetHandle = Handle<Script>::UnPack(asset->Indentifier);

		if (!scriptAssetHandle.IsValid())
		{
			HBL2_CORE_WARN("Asset \"{0}\" has an invalid handle, skipping unload operation.", asset->DebugName);
			return;
		}

		if (!asset->Loaded)
		{
			HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
			return;
		}

		Script* script = ResourceManager::Instance->GetScript(scriptAssetHandle);

		if (script != nullptr)
		{
			switch (script->Type)
			{
			case ScriptType::SYSTEM:
			{
				Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

				// If active scene is null, it means it is already unloaded and all the attached scripts are unloaded as well.
				if (activeScene == nullptr)
				{
					break;
				}

				// Delete registered user system.
				for (ISystem* userSystem : activeScene->GetRuntimeSystems())
				{
					if (userSystem->Name == script->Name && userSystem->GetType() == SystemType::User)
					{
						activeScene->DeregisterSystem(userSystem);
						break;
					}
				}
			}
			break;
			case ScriptType::COMPONENT:
			{
				Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

				// If active scene is null, it means it is already unloaded and all the attached scripts are unloaded as well.
				if (activeScene == nullptr)
				{
					break;
				}

				// Remove component from all the entities of the source scene.
				Reflect::TypeEntry* entryToRemove = nullptr;
				Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
				{
					std::string componentName = BuildEngine::Instance->CleanComponentNameO3(std::string(entry.typeName));

					if (script->Name == componentName)
					{
						entry.clearStorage(&activeScene->GetRegistry());
						entryToRemove = (Reflect::TypeEntry*)&entry;
						return;
					}
				});

				if (entryToRemove)
				{
					Reflect::Unregister(entryToRemove);
				}
			}
			break;
			}
		}

		ResourceManager::Instance->DeleteScript(scriptAssetHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}

	void EditorAssetManager::UnloadScene(Asset* asset)
	{
		Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);

		if (!sceneHandle.IsValid())
		{
			HBL2_CORE_WARN("Asset \"{0}\" has an invalid resource handle, skipping unload operation.", asset->DebugName);
			return;
		}

		if (!asset->Loaded)
		{
			HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
			return;
		}

		// Retrieve scene.
		Scene* currentScene = ResourceManager::Instance->GetScene(sceneHandle);

		// Clear entire scene.
		if (currentScene != nullptr)
		{
			currentScene->Clear();
		}

		// Delete from pool.
		ResourceManager::Instance->DeleteScene(sceneHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}

	void EditorAssetManager::UnloadSound(Asset* asset)
	{
		Handle<Sound> soundHandle = Handle<Sound>::UnPack(asset->Indentifier);

		if (!soundHandle.IsValid())
		{
			HBL2_CORE_WARN("Asset \"{0}\" has an invalid resource handle, skipping unload operation.", asset->DebugName);
			return;
		}

		if (!asset->Loaded)
		{
			HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
			return;
		}

		// Retrieve and destroy sound.
		Sound* sound = ResourceManager::Instance->GetSound(soundHandle);
		if (sound != nullptr)
		{
			if (sound->ID != InvalidSoundID)
			{
				SoundEngine::Instance->ReleaseSound(sound->ID);
				sound->ID = InvalidSoundID;
			}
		}

		// Delete from pool.
		ResourceManager::Instance->DeleteSound(soundHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}

	void EditorAssetManager::UnloadPrefab(Asset* asset)
	{
		Handle<Prefab> prefabHandle = Handle<Prefab>::UnPack(asset->Indentifier);

		if (!prefabHandle.IsValid())
		{
			HBL2_CORE_WARN("Asset \"{0}\" has an invalid resource handle, skipping unload operation.", asset->DebugName);
			return;
		}

		if (!asset->Loaded)
		{
			HBL2_CORE_WARN("Asset \"{0}\" is already unloaded, skipping unload operation.", asset->DebugName);
			return;
		}

		// Unload.
		Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

		if (prefab)
		{
			prefab->Unload();
		}

		// Delete from pool.
		ResourceManager::Instance->DeletePrefab(prefabHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}
}
