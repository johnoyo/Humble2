#include "EditorAssetManager.h"

#include "Script\BuildEngine.h"
#include "Utilities\YamlUtilities.h"
#include "Utilities\Collections\Collections.h"

#include "Systems\HierachySystem.h"
#include "Systems\CameraSystem.h"
#include "Systems\RenderingSystem.h"
#include "Systems\SoundSystem.h"
#include "Systems\Physics2dSystem.h"
#include "Systems\Physics3dSystem.h"
#include "Systems\TerrainSystem.h"
#include "Systems\AnimationCurveSystem.h"

#include <Prefab/PrefabSerializer.h>

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

    bool EditorAssetManager::DestroyAsset(Handle<Asset> handle)
    {
        Asset* asset = GetAssetMetadata(handle);

		if (asset == nullptr)
		{
			return false;
		}

		switch (asset->Type)
		{
		case AssetType::Texture:
			return DestroyTexture(asset);
		case AssetType::Shader:
			return DestroyShader(asset);
		case AssetType::Material:
			return DestroyMaterial(asset);
		case AssetType::Mesh:
			return DestroyMesh(asset);
		case AssetType::Script:
			return DestroyScript(asset);
		case AssetType::Scene:
			return DestroyScene(asset);
		case AssetType::Sound:
			return DestroySound(asset);
		case AssetType::Prefab:
			return DestroyPrefab(asset);
		}

		HBL2_CORE_ASSERT(false, "Unsupported asset type!");
		return false;
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
				.debugName = _strdup(std::format("{}-texture", textureName).c_str()),
				.dimensions = { textureSettings.Width, textureSettings.Height, 1 },
				.format = textureSettings.PixelFormat,
				.internalFormat = textureSettings.PixelFormat,
				.usage = { TextureUsage::SAMPLED, TextureUsage::COPY_DST },
				.aspect = TextureAspect::COLOR,
				.sampler = { .filter = Filter::LINEAR },
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
		const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : asset->FilePath;

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

		auto shaderVariants = MakeDArray<ShaderDescriptor::RenderPipeline::PackedVariant>(Allocator::FrameArena);

		const auto& shaderProperties = data["Shader"];
		if (shaderProperties)
		{
			uint32_t type = shaderProperties["Type"].as<uint32_t>();

			switch (type)
			{
			case 0:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout2D();
				drawBindGroupLayout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::UNLIT);
				break;
			case 1:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout3D();
				drawBindGroupLayout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::BLINN_PHONG);
				break;
			case 2:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout3D();
				drawBindGroupLayout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::PBR);
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

					// Retrieve blend state.
					const auto& blendStateProp = variantNode["BlendState"];

					if (blendStateProp)
					{
						variant.colorOutput = blendStateProp["ColorOutputEnabled"].as<bool>();
						variant.blendEnabled = blendStateProp["Enabled"].as<bool>();
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

					shaderVariants.push_back(variant);
				}
			}
		}

		// Compile Shader.
		const auto& shaderCode = ShaderUtilities::Get().Compile(shaderPath.string());

		if (shaderCode.empty())
		{
			HBL2_CORE_ERROR("Shader asset: {0}, at path: {1}, could not be compiled. Returning invalid shader.", asset->DebugName, shaderPath.string());
			stream.close();
			return ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::INVALID);
		}

		// Reflect shader.
		const auto& reflectionData = ShaderUtilities::Get().GetReflectionData(shaderPath.string());

		// Create resource.
		auto shader = ResourceManager::Instance->CreateShader({
			.debugName = _strdup(std::format("{}-shader", shaderName).c_str()),
			.VS { .code = shaderCode[0], .entryPoint = reflectionData.VertexEntryPoint.c_str() },
			.FS { .code = shaderCode[1], .entryPoint = reflectionData.FragmentEntryPoint.c_str() },
			.bindGroups {
				globalBindGroupLayout,	// Global bind group (0)
				drawBindGroupLayout,	// Material bind group (1)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = reflectionData.ByteStride,
						.attributes = reflectionData.Attributes,
					},
				},
				.variants = { shaderVariants.data(), shaderVariants.size() },
			},
			.renderPass = Renderer::Instance->GetRenderingRenderPass(),
		});

		stream.close();
		return shader;
	}

	Handle<Material> EditorAssetManager::ImportMaterial(Asset* asset)
	{
		// Open metadata file.
		const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
		const std::filesystem::path& materialPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : asset->FilePath;

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
			HBL2_CORE_TRACE("Material not found in metadata file: {0}", ssMetadata.str());
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
			HBL2_CORE_TRACE("Material not found: {0}", ss.str());
			stream.close();
			return Handle<Material>();
		}

		auto materialProperties = data["Material"];
		if (materialProperties)
		{
			UUID shaderUUID = materialProperties["Shader"].as<UUID>();

			ShaderDescriptor::RenderPipeline::PackedVariant variantDesc = {};

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

			UUID albedoMapUUID = materialProperties["AlbedoMap"].as<UUID>();
			UUID normalMapUUID = materialProperties["NormalMap"].as<UUID>();
			UUID metallicMapUUID = materialProperties["MetallicMap"].as<UUID>();
			UUID roughnessMapUUID = materialProperties["RoughnessMap"].as<UUID>();

			glm::vec4 albedoColor = materialProperties["AlbedoColor"].as<glm::vec4>();
			float glossiness = materialProperties["Glossiness"].as<float>();

			JobContext materialJobsCtx;

			auto shaderJobHandle = AssetManager::Instance->GetAssetAsync<Shader>(shaderUUID, &materialJobsCtx);
			auto albedoMapJobHandle = AssetManager::Instance->GetAssetAsync<Texture>(albedoMapUUID, &materialJobsCtx);
			auto normalMapJobHandle = AssetManager::Instance->GetAssetAsync<Texture>(normalMapUUID, &materialJobsCtx);
			auto metallicMapJobHandle = AssetManager::Instance->GetAssetAsync<Texture>(metallicMapUUID, &materialJobsCtx);
			auto roughnessMapJobHandle = AssetManager::Instance->GetAssetAsync<Texture>(roughnessMapUUID, &materialJobsCtx);

			AssetManager::Instance->WaitForAsyncJobs(&materialJobsCtx);

			Handle<Shader> shaderHandle = (shaderJobHandle ? shaderJobHandle->ResourceHandle : Handle<Shader>{});
			Handle<Texture> albedoMapHandle = (albedoMapJobHandle ? albedoMapJobHandle->ResourceHandle : Handle<Texture>{});
			Handle<Texture> normalMapHandle = (normalMapJobHandle ? normalMapJobHandle->ResourceHandle : Handle<Texture>{});
			Handle<Texture> metallicMapHandle = (metallicMapJobHandle ? metallicMapJobHandle->ResourceHandle : Handle<Texture>{});
			Handle<Texture> roughnessMapHandle = (roughnessMapJobHandle ? roughnessMapJobHandle->ResourceHandle : Handle<Texture>{});

			// If shader is not set, get the built in shader depending on material type.
			if (!shaderHandle.IsValid())
			{
				if (type == 0)
				{
					shaderHandle = ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::UNLIT);
				}
				else if (type == 1)
				{
					shaderHandle = ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::BLINN_PHONG);
				}
				else if (type == 2)
				{
					shaderHandle = ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::PBR);
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
			}

			HBL2_CORE_ASSERT(shaderHandle.IsValid(), "Error while trying to load shader of material!");
			HBL2_CORE_ASSERT(shaderUUID != 0, "Error while trying to load shader of material!");

			ResourceManager::Instance->GetOrAddShaderVariant(shaderHandle, variantDesc);
			ShaderUtilities::Get().UpdateShaderVariantMetadataFile(shaderUUID, variantDesc);

			// If albedo map is not set use the built in white texture.
			if (!albedoMapHandle.IsValid())
			{
				albedoMapHandle = TextureUtilities::Get().WhiteTexture;
			}

			const std::string& materialName = asset->FilePath.filename().stem().string();

			Handle<BindGroup> drawBindings;
			uint32_t dynamicUniformBufferRange = (type == 0 ? sizeof(PerDrawDataSprite) : sizeof(PerDrawData));

			if (type == 2) // PBR
			{
				if (!normalMapHandle.IsValid())
				{
					normalMapHandle = TextureUtilities::Get().WhiteTexture;
				}

				drawBindings = ResourceManager::Instance->CreateBindGroup({
					.debugName = _strdup(std::format("{}-bind-group", materialName).c_str()),
					.layout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::PBR),
					.textures = { albedoMapHandle, normalMapHandle, metallicMapHandle, roughnessMapHandle, Renderer::Instance->ShadowAtlasTexture },
					.buffers = {
						{ .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer(), .range = dynamicUniformBufferRange },
					}
				});
			}
			else
			{
				drawBindings = ResourceManager::Instance->CreateBindGroup({
					.debugName = _strdup(std::format("{}-bind-group", materialName).c_str()),
					.layout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::BLINN_PHONG), // BuiltInShader::BLINN_PHONG, UNLIT, INVALID have the same bindgroup layout.
					.textures = { albedoMapHandle, Renderer::Instance->ShadowAtlasTexture },
					.buffers = {
						{ .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer(), .range = dynamicUniformBufferRange },
					}
				});
			}

			auto material = ResourceManager::Instance->CreateMaterial({
				.debugName = _strdup(std::format("{}-material", materialName).c_str()),
				.shader = shaderHandle,
				.bindGroup = drawBindings,
			});

			Material* mat = ResourceManager::Instance->GetMaterial(material);
			mat->AlbedoColor = albedoColor;
			mat->Glossiness = glossiness;
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

		auto sceneHandle = ResourceManager::Instance->CreateScene({
			.name = asset->FilePath.filename().stem().string()
		});

		Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Scene asset \"{0}\" is invalid, aborting scene load.", asset->FilePath.filename().stem().string());
			return sceneHandle;
		}

		scene->RegisterSystem(new HierachySystem);
		scene->RegisterSystem(new CameraSystem, SystemType::Runtime);
		scene->RegisterSystem(new TerrainSystem);
		scene->RegisterSystem(new RenderingSystem);
		scene->RegisterSystem(new SoundSystem, SystemType::Runtime);
		scene->RegisterSystem(new Physics2dSystem, SystemType::Runtime);
		scene->RegisterSystem(new Physics3dSystem, SystemType::Runtime);
		scene->RegisterSystem(new AnimationCurveSystem);

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
				.debugName = _strdup(std::format("{}-sound", soundName).c_str()),
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

			auto prefabHandle = ResourceManager::Instance->CreatePrefab({
				.debugName = prefabName.c_str(),
				.uuid = asset->UUID,
				.baseEntityUUID = baseEntityUUID,
				.version = version,
			});

			Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

			if (prefab == nullptr)
			{
				HBL2_CORE_ERROR("Prefab asset \"{0}\" is invalid, aborting prefab load.", asset->FilePath.filename().stem().string());
				return Handle<Prefab>();
			}

			// NOTE: We do not deserialize here!

			return prefabHandle;
		}

		return Handle<Prefab>();
	}

	Handle<Mesh> EditorAssetManager::ImportMesh(Asset* asset)
	{
		const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
		const std::filesystem::path& meshPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : asset->FilePath;

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
		const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : asset->FilePath;

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

		auto shaderVariants = MakeDArray<ShaderDescriptor::RenderPipeline::PackedVariant>(Allocator::FrameArena);

		const auto& shaderProperties = data["Shader"];
		if (shaderProperties)
		{
			uint32_t type = shaderProperties["Type"].as<uint32_t>();

			switch (type)
			{
			case 0:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout2D();
				drawBindGroupLayout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::UNLIT);
				break;
			case 1:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout3D();
				drawBindGroupLayout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::BLINN_PHONG);
				break;
			case 2:
				globalBindGroupLayout = Renderer::Instance->GetGlobalBindingsLayout3D();
				drawBindGroupLayout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::PBR);
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

					// Retrieve blend state.
					const auto& blendStateProp = variantNode["BlendState"];

					if (blendStateProp)
					{
						variant.colorOutput = blendStateProp["ColorOutputEnabled"].as<bool>();
						variant.blendEnabled = blendStateProp["Enabled"].as<bool>();
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

					shaderVariants.push_back(variant);
				}
			}
		}

		// Compile Shader.
		const auto& shaderCode = ShaderUtilities::Get().Compile(shaderPath.string(), true);

		if (shaderCode.empty())
		{
			HBL2_CORE_ERROR("Shader asset: {0}, at path: {1}, could not be compiled. Returning invalid shader.", asset->DebugName, shaderPath.string());
			stream.close();
			return ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::INVALID);
		}

		// Reflect shader.
		const auto& reflectionData = ShaderUtilities::Get().GetReflectionData(shaderPath.string());

		// Create resource.
		ResourceManager::Instance->RecompileShader(shaderHandle, {
			.debugName = _strdup(std::format("{}-shader", shaderName).c_str()),
			.VS { .code = shaderCode[0], .entryPoint = reflectionData.VertexEntryPoint.c_str() },
			.FS { .code = shaderCode[1], .entryPoint = reflectionData.FragmentEntryPoint.c_str() },
			.bindGroups {
				globalBindGroupLayout,	// Global bind group (0)
				drawBindGroupLayout,	// Material bind group (1)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = reflectionData.ByteStride,
						.attributes = reflectionData.Attributes,
					},
				},
				.variants = { shaderVariants.data(), shaderVariants.size() },
			},
			.renderPass = Renderer::Instance->GetRenderingRenderPass(),
		});

		stream.close();

		return shaderHandle;
	}

	Handle<Material> EditorAssetManager::ReimportMaterial(Asset* asset)
	{
		Handle<Material> materialHandle = Handle<Material>::UnPack(asset->Indentifier);

		if (!materialHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return Handle<Material>();
		}

		Material* mat = ResourceManager::Instance->GetMaterial(materialHandle);

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

			if (shaderHandle == mat->Shader)
			{
				AssetManager::Instance->ReloadAsset<Shader>(shaderUUID);
			}
			else
			{
				// TODO: Handle shader resource change.
			}
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
		const std::filesystem::path& meshPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : asset->FilePath;

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
			materialProperties["AlbedoColor"] = mat->AlbedoColor;
			materialProperties["Glossiness"] = mat->Glossiness;

			materialProperties["BlendState"]["Enabled"] = mat->VariantHash.blendEnabled;
			materialProperties["BlendState"]["ColorOutputEnabled"] = mat->VariantHash.colorOutput;

			materialProperties["DepthState"]["Enabled"] = mat->VariantHash.depthEnabled;
			materialProperties["DepthState"]["WriteEnabled"] = mat->VariantHash.depthWrite;
			materialProperties["DepthState"]["StencilEnabled"] = mat->VariantHash.stencilEnabled;
			materialProperties["DepthState"]["DepthTest"] = (int)mat->VariantHash.depthCompare;

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

			std::ofstream fout(filePath.string() + ".hblscene", 0);

			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Scene" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();

			auto sceneHandle = ResourceManager::Instance->CreateScene({
				.name = asset->FilePath.filename().stem().string()
			});

			Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Scene asset \"{0}\" is invalid, aborting scene load.", asset->FilePath.filename().stem().string());
				return;
			}

			scene->RegisterSystem(new HierachySystem);
			scene->RegisterSystem(new CameraSystem, SystemType::Runtime);
			scene->RegisterSystem(new TerrainSystem);
			scene->RegisterSystem(new RenderingSystem);
			scene->RegisterSystem(new SoundSystem, SystemType::Runtime);
			scene->RegisterSystem(new Physics2dSystem, SystemType::Runtime);
			scene->RegisterSystem(new Physics3dSystem, SystemType::Runtime);
			scene->RegisterSystem(new AnimationCurveSystem);

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

			// Free the cpu side pixel data sice they are copied by the driver.
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
		std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> data;

		// Store all registered meta types.
		for (auto meta_type : entt::resolve(activeScene->GetMetaContext()))
		{
			std::string componentName = meta_type.second.info().name().data();
			componentName = BuildEngine::Instance->CleanComponentNameO3(componentName);
			userComponentNames.push_back(componentName);

			BuildEngine::Instance->SerializeComponents(componentName, activeScene, data);
		}

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

			BuildEngine::Instance->DeserializeComponents(userComponentName, activeScene, data);
		}

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
				for (auto meta_type : entt::resolve(activeScene->GetMetaContext()))
				{
					std::string componentName = meta_type.second.info().name().data();
					componentName = BuildEngine::Instance->CleanComponentNameO3(componentName);

					if (script->Name == componentName)
					{
						BuildEngine::Instance->ClearComponentStorage(componentName, activeScene);
						entt::meta_reset(activeScene->GetMetaContext(), meta_type.first);
						break;
					}
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

		// Delete from pool.
		ResourceManager::Instance->DeletePrefab(prefabHandle);

		asset->Loaded = false;
		asset->Indentifier = 0;
	}
}
