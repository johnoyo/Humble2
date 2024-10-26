#include "AssetImporter.h"

#include "AssetManager.h"
#include "Utilities\YamlUtilities.h"

#include "Systems\TransformSystem.h"
#include "Systems\LinkSystem.h"
#include "Systems\CameraSystem.h"
#include "Systems\StaticMeshRenderingSystem.h"
#include "Systems\SpriteRenderingSystem.h"

namespace HBL2
{
	uint32_t AssetImporter::ImportAsset(Asset* asset)
	{
		if (asset == nullptr)
		{
			return 0;
		}

		asset->Loaded = true;

		switch (asset->Type)
		{
		case AssetType::Texture:
			asset->Indentifier = ImportTexture(asset).Pack();
			return asset->Indentifier;
		case AssetType::Shader:
			asset->Indentifier = ImportShader(asset).Pack();
			return asset->Indentifier;
		case AssetType::Material:
			asset->Indentifier = ImportMaterial(asset).Pack();
			return asset->Indentifier;
		case AssetType::Mesh:
			asset->Indentifier = ImportMesh(asset).Pack();
			return asset->Indentifier;
		case AssetType::Scene:
			asset->Indentifier = ImportScene(asset).Pack();
			return asset->Indentifier;
		case AssetType::Script:
			// asset->Indentifier = ImportScript(asset).Pack();
			return 0; // asset->Indentifier;
		}

		return 0;
	}

	void AssetImporter::SaveAsset(Asset* asset)
	{
		if (asset == nullptr)
		{
			return;
		}

		switch (asset->Type)
		{
		case AssetType::Scene:
			SaveScene(asset);
			break;
		}
	}

    Handle<Texture> AssetImporter::ImportTexture(Asset* asset)
    {
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture");

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Texture file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hbltexture");
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
			stbi_uc* textureData = TextureUtilities::Get().Load(Project::GetAssetFileSystemPath(asset->FilePath).string(), textureSettings);

			const std::string& textureName = asset->FilePath.filename().stem().string();

			// Create the texture
			auto texture = ResourceManager::Instance->CreateTexture({
				.debugName = _strdup(std::format("{}-texture", textureName).c_str()),
				.dimensions = { textureSettings.Width, textureSettings.Height, 1 },
				.usage = TextureUsage::SAMPLED,
				.aspect = TextureAspect::COLOR,
				.initialData = textureData,
			});

			stream.close();
			return texture;
		}

		HBL2_CORE_ERROR("Texture not found: {0}", asset->DebugName);
		
		stream.close();
		return Handle<Texture>();
    }

	Handle<Shader> AssetImporter::ImportShader(Asset* asset)
    {
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblshader");

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Shader file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblshader");
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

		auto shaderProperties = data["Shader"];
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
		}

		// Compile Shader.
		std::filesystem::path shaderPath = Project::GetAssetFileSystemPath(asset->FilePath);
		auto shaderCode = ShaderUtilities::Get().Compile(shaderPath.string());

		if (shaderCode.empty())
		{
			HBL2_CORE_ERROR("Shader asset: {0}, at path: {1}, could be compiled. Returning invalid shader.", asset->DebugName, shaderPath.string());
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
				.blend = {
					.colorOp = BlendOperation::ADD,
					.srcColorFactor = BlendFactor::SRC_ALPHA,
					.dstColorFactor = BlendFactor::ONE_MINUS_SRC_ALPHA, // TODO: get them from the shader file.
					.alphaOp = BlendOperation::ADD,
					.srcAlphaFactor = BlendFactor::ONE,
					.dstAlphaFactor = BlendFactor::ZERO,
					.enabled = true,
				},
				.depthTest = {
					.enabled = true,
					.depthTest = Compare::LESS,
				},
				.vertexBufferBindings = {
					{
						.byteStride = reflectionData.ByteStride,
						.attributes = reflectionData.Attributes,
					},
				},
			},
			.renderPass = Renderer::Instance->GetMainRenderPass(),
		});

		stream.close();
		return shader;
    }

	Handle<Material> AssetImporter::ImportMaterial(Asset* asset)
	{
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath));
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Material"].IsDefined())
		{
			HBL2_CORE_TRACE("Material not found: {0}", ss.str());
			return Handle<Material>();
		}

		auto materialProperties = data["Material"];
		if (materialProperties)
		{
			uint32_t type = materialProperties["Type"].as<uint32_t>();

			UUID shaderUUID = materialProperties["Shader"].as<UUID>();

			UUID albedoMapUUID = materialProperties["AlbedoMap"].as<UUID>();
			UUID normalMapUUID = materialProperties["NormalMap"].as<UUID>();
			UUID metallicMapUUID = materialProperties["MetallicMap"].as<UUID>();
			UUID roughnessMapUUID = materialProperties["RoughnessMap"].as<UUID>();

			glm::vec4 albedoColor = materialProperties["AlbedoColor"].as<glm::vec4>();
			float glossiness = materialProperties["Metalicness"].as<float>();

			auto shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderUUID);
			auto albedoMapHandle = AssetManager::Instance->GetAsset<Texture>(albedoMapUUID);
			auto normalMapHandle = AssetManager::Instance->GetAsset<Texture>(normalMapUUID);
			auto metallicMapHandle = AssetManager::Instance->GetAsset<Texture>(metallicMapUUID);
			auto roughnessMapHandle = AssetManager::Instance->GetAsset<Texture>(roughnessMapUUID);

			if (!albedoMapHandle.IsValid())
			{
				albedoMapHandle = TextureUtilities::Get().WhiteTexture;
			}

			const std::string& materialName = asset->FilePath.filename().stem().string();

			Handle<BindGroup> drawBindings;
			uint32_t dynamicUniformBufferRange = type == 0 ? sizeof(PerDrawDataSprite) : sizeof(PerDrawData);

			if (normalMapHandle.IsValid() && metallicMapHandle.IsValid() && roughnessMapHandle.IsValid())
			{
				drawBindings = ResourceManager::Instance->CreateBindGroup({
					.debugName = _strdup(std::format("{}-bind-group", materialName).c_str()),
					.layout = ShaderUtilities::Get().GetBuiltInShaderLayout(BuiltInShader::PBR),
					.textures = { albedoMapHandle, normalMapHandle, metallicMapHandle, roughnessMapHandle },
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
					.textures = { albedoMapHandle },
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

			stream.close();
			return material;
		}

		stream.close();
		return Handle<Material>();
	}

	Handle<Scene> AssetImporter::ImportScene(Asset* asset)
	{
		auto sceneHandle = ResourceManager::Instance->CreateScene({
			.name = asset->FilePath.filename().stem().string()
		});

		Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Scene asset \"{0}\" is invalid, aborting scene load.", asset->FilePath.filename().stem().string());
			return sceneHandle;
		}

		SceneSerializer sceneSerializer(scene);
		sceneSerializer.Deserialize(Project::GetAssetFileSystemPath(asset->FilePath));

		scene->RegisterSystem(new TransformSystem);
		scene->RegisterSystem(new LinkSystem);
		scene->RegisterSystem(new CameraSystem, SystemType::Runtime);
		// scene->RegisterSystem(new StaticMeshRenderingSystem);
		scene->RegisterSystem(new SpriteRenderingSystem);

		return sceneHandle;
	}

	Handle<Mesh> AssetImporter::ImportMesh(Asset* asset)
	{
		std::ifstream stream(Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblmesh");
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Mesh"].IsDefined())
		{
			HBL2_CORE_TRACE("Mesh not found: {0}", ss.str());
			return Handle<Mesh>();
		}

		auto meshProperties = data["Mesh"];
		if (meshProperties)
		{
			MeshData* meshData = MeshUtilities::Get().Load(Project::GetAssetFileSystemPath(asset->FilePath));

			const std::string& meshName = asset->FilePath.filename().stem().string();

			auto vertexBuffer = ResourceManager::Instance->CreateBuffer({
				.debugName = _strdup(std::format("{}-vertex-buffer", meshName).c_str()),
				.usage = BufferUsage::VERTEX,
				.memoryUsage = MemoryUsage::GPU_ONLY,
				.byteSize = (uint32_t)(meshData->VertexBuffer.size() * sizeof(Vertex)),
				.initialData = meshData->VertexBuffer.data(),
			});

			Handle<Buffer> indexBuffer;
			if (meshData->IndexBuffer.size() > 0)
			{
				indexBuffer = ResourceManager::Instance->CreateBuffer({
					.debugName = _strdup(std::format("{}-index-buffer", meshName).c_str()),
					.usage = BufferUsage::INDEX,
					.memoryUsage = MemoryUsage::GPU_ONLY,
					.byteSize = (uint32_t)(meshData->IndexBuffer.size() * sizeof(uint32_t)),
					.initialData = meshData->IndexBuffer.data(),
				});
			}

			// Create the mesh
			auto mesh = ResourceManager::Instance->CreateMesh({
				.debugName = _strdup(std::format("{}-mesh", meshName).c_str()),
				.indexOffset = 0,
				.indexCount = (uint32_t)meshData->IndexBuffer.size(),
				.vertexOffset = 0,
				.vertexCount = (uint32_t)meshData->VertexBuffer.size(),
				.indexBuffer = indexBuffer,
				.vertexBuffers = { vertexBuffer },
			});

			return mesh;
		}

		return Handle<Mesh>();
	}

	void AssetImporter::SaveScene(Asset* asset)
	{
		if (!asset->Loaded)
		{
			HBL2_CORE_ERROR(" Scene: {0}, at path: {1}, loading it now.", asset->DebugName, asset->FilePath.string());

			auto sceneHandle = ResourceManager::Instance->CreateScene({
				.name = asset->FilePath.filename().stem().string()
			});

			Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Scene asset \"{0}\" is invalid, aborting scene load.", asset->FilePath.filename().stem().string());
				return;
			}

			scene->RegisterSystem(new TransformSystem);
			scene->RegisterSystem(new LinkSystem);
			scene->RegisterSystem(new CameraSystem, SystemType::Runtime);
			// scene->RegisterSystem(new StaticMeshRenderingSystem);
			scene->RegisterSystem(new SpriteRenderingSystem);

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
}
