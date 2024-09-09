#include "AssetImporter.h"

#include "AssetManager.h"
#include "Utilities\YamlUtilities.h"

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
		}

		return 0;
	}

    Handle<Texture> AssetImporter::ImportTexture(Asset* asset)
    {
		std::ifstream stream(asset->FilePath.string() + ".hbltexture");
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
			TextureData textureData = TextureUtilities::Get().Load(asset->FilePath.string(), textureSettings);

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
		auto shaderCode = ShaderUtilities::Get().Compile(asset->FilePath.string());

		auto shader = ResourceManager::Instance->CreateShader({
			.debugName = "test-shader",
			.VS { .code = shaderCode[0], .entryPoint = "main" },
			.FS { .code = shaderCode[1], .entryPoint = "main" },
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 12,
						.attributes = {
							{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
						},
					}
				}
			}
		});

		return shader;
    }

	Handle<Material> AssetImporter::ImportMaterial(Asset* asset)
	{
		std::ifstream stream(asset->FilePath);
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

			return material;
		}

		return Handle<Material>();
	}

	Handle<Mesh> AssetImporter::ImportMesh(Asset* asset)
	{
		return Handle<Mesh>();
	}
}

