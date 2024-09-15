#include "AssetImporter.h"

#include "AssetManager.h"
#include "Utilities\YamlUtilities.h"

#include "Systems\TransformSystem.h"
#include "Systems\LinkSystem.h"
#include "Systems\CameraSystem.h"
#include "Systems\StaticMeshRenderingSystem.h"

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

		if (!asset->Loaded)
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because it is not loaded.", asset->DebugName, asset->FilePath.string());
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
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Texture"].IsDefined())
		{
			HBL2_CORE_TRACE("Texture not found: {0}", ss.str());
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

		// Return the handle
		return Handle<Texture>();
    }

	Handle<Shader> AssetImporter::ImportShader(Asset* asset)
    {
		// Compile Shader.
		std::filesystem::path shaderPath = Project::GetAssetFileSystemPath(asset->FilePath);
		auto shaderCode = ShaderUtilities::Get().Compile(shaderPath.string());

		// Reflect Shader.
		const auto& reflectionData = ShaderUtilities::Get().GetReflectionData(shaderPath.string());

		// Create Resource.
		auto shader = ResourceManager::Instance->CreateShader({
			.debugName = "test-shader",
			.VS { .code = shaderCode[0], .entryPoint = reflectionData.VertexEntryPoint.c_str() },
			.FS {.code = shaderCode[1], .entryPoint = reflectionData.FragmentEntryPoint.c_str() },
			.bindGroups {
				{}, // Global bind group (0)
				{}, // Material bind group (1)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = reflectionData.ByteStride,
						.attributes = reflectionData.Attributes,
					}
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
			glm::vec4 albedoColor = materialProperties["AlbedoColor"].as<glm::vec4>();
			float metalicness = materialProperties["Metalicness"].as<float>();
			float roughness = materialProperties["Roughness"].as<float>();

			auto shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderUUID);

			auto drawBindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
				.debugName = "unlit-colored-layout",
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
		scene->RegisterSystem(new CameraSystem);
		scene->RegisterSystem(new StaticMeshRenderingSystem);

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

		}

		return Handle<Mesh>();
	}

	void AssetImporter::SaveScene(Asset* asset)
	{
		Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);

		if (!sceneHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not save asset {0} at path: {1}, because the handle is invalid.", asset->DebugName, asset->FilePath.string());
			return;
		}

		Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);
		SceneSerializer serializer(scene);
		serializer.Serialize(asset->FilePath);
	}
}
