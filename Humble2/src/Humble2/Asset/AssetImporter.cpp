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
			TextureData textureData = TextureUtilities::Get().Load(Project::GetAssetFileSystemPath(asset->FilePath).string(), textureSettings);

			// Create the texture
			auto texture = ResourceManager::Instance->CreateTexture({
				.debugName = "test-texture",
				.dimensions = { textureData.Width, textureData.Height, 0 },
				.initialData = textureData.Data,
			});

			return texture;
		}

		HBL2_CORE_ERROR("Texture not found: {0}", asset->DebugName);

		return Handle<Texture>();
    }

	Handle<Shader> AssetImporter::ImportShader(Asset* asset)
    {
		// Compile Shader.
		std::filesystem::path shaderPath = Project::GetAssetFileSystemPath(asset->FilePath);
		auto shaderCode = ShaderUtilities::Get().Compile(shaderPath.string());

		if (shaderCode.empty())
		{
			HBL2_CORE_ERROR("Shader asset: {0}, at path: {1}, could be compiled. Returning invalid shader.", asset->DebugName, shaderPath.string());
			return ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::INVALID);
		}

		// Reflect Shader.
		const auto& reflectionData = ShaderUtilities::Get().GetReflectionData(shaderPath.string());

		// Create Resource.
		auto shader = ResourceManager::Instance->CreateShader({
			.debugName = "test-shader",
			.VS {.code = shaderCode[0], .entryPoint = reflectionData.VertexEntryPoint.c_str() },
			.FS {.code = shaderCode[1], .entryPoint = reflectionData.FragmentEntryPoint.c_str() },
			.bindGroups {
				{}, // Global bind group (0)
				{}, // Material bind group (1)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 20,
						.attributes = {
							{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
							{ .byteOffset = 12, .format = VertexFormat::FLOAT32x2 },
						},
					},
				}
			},
		});

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
			UUID shaderUUID = materialProperties["Shader"].as<UUID>();
			UUID albedoMapUUID = materialProperties["AlbedoMap"].as<UUID>();
			glm::vec4 albedoColor = materialProperties["AlbedoColor"].as<glm::vec4>();
			float metalicness = materialProperties["Metalicness"].as<float>();
			float roughness = materialProperties["Roughness"].as<float>();

			auto shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderUUID);
			auto albedoMapHandle = AssetManager::Instance->GetAsset<Texture>(albedoMapUUID);

			if (!albedoMapHandle.IsValid())
			{
				albedoMapHandle = TextureUtilities::Get().WhiteTexture;
			}

			auto drawBindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
				.debugName = "unlit-colored-layout",
				.textureBindings = {
					{
						.slot = 0,
						.visibility = ShaderStage::FRAGMENT,
					}
				},
				.bufferBindings = {
					{
						.slot = 1,
						.visibility = ShaderStage::VERTEX,
						.type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
					},
				},
			});

			auto drawBindings = ResourceManager::Instance->CreateBindGroup({
				.debugName = "unlit-colored-bind-group",
				.layout = drawBindGroupLayout,
				.textures = {
					albedoMapHandle,
				},
				.buffers = {
					{ .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer() },
				}
			});

			auto material = ResourceManager::Instance->CreateMaterial({
				.debugName = "test-material",
				.shader = shaderHandle,
				.bindGroup = drawBindings,
			});

			Material* mat = ResourceManager::Instance->GetMaterial(material);
			mat->AlbedoColor = albedoColor;
			mat->Metalicness = metalicness;
			mat->Roughness = roughness;

			return material;
		}

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
		scene->RegisterSystem(new StaticMeshRenderingSystem);
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
			MeshData meshData = MeshUtilities::Get().Load(Project::GetAssetFileSystemPath(asset->FilePath));

			auto vertexBuffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "vertex-buffer",
				.byteSize = (uint32_t)(meshData.VertexBuffer.size() * sizeof(Vertex)),
				.initialData = meshData.VertexBuffer.data(),
			});

			auto indexBuffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "index-buffer",
				.byteSize = (uint32_t)(meshData.IndexBuffer.size() * sizeof(uint32_t)),
				.initialData = meshData.IndexBuffer.data(),
			});

			// Create the texture
			auto mesh = ResourceManager::Instance->CreateMesh({
				.debugName = "test-mesh",
				.indexOffset = 0,
				.indexCount = (uint32_t)meshData.IndexBuffer.size(),
				.vertexOffset = 0,
				.vertexCount = (uint32_t)meshData.VertexBuffer.size(),
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
