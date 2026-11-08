#include "AssetPack.h"

#include "Serializer.h"
#include "Asset/EditorAssetManager.h"
#include "Utilities/Collections/ByteSpan.h"
#include "Utilities/YamlUtilities.h"

namespace HBL2
{
	using packed_size = ShaderDescriptor::RenderPipeline::packed_size;

    bool AssetPack::Serialize(UUID assetUUID, FileStreamWriter& stream)
    {
        EditorAssetManager* assetManager = (EditorAssetManager*)AssetManager::Instance;

        Handle<Asset> assetHandle = assetManager->GetHandleFromUUID(assetUUID);
        Asset* asset = assetManager->GetAssetMetadata(assetHandle);

		bool result = false;

        switch (asset->Type)
        {
		case AssetType::Texture:
			result = SerializeTexture(asset, stream);
			break;
		case AssetType::Shader:
			result = SerializeShader(asset, stream);
			break;
		case AssetType::Material:
			result = SerializeMaterial(asset, stream);
			break;
		case AssetType::Mesh:
			result = SerializeMesh(asset, stream);
			break;
		case AssetType::Scene:
			result = SerializeScene(asset, stream);
			break;
		case AssetType::Script:
			result = SerializeScript(asset, stream);
			break;
		case AssetType::Sound:
			result = SerializeSound(asset, stream);
			break;
		case AssetType::Prefab:
			result = SerializePrefab(asset, stream);
			break;
		case AssetType::None:
			result = false;
			break;
        }

        return result;
    }

	bool AssetPack::SerializeTexture(Asset* asset, FileStreamWriter& stream)
	{
		// Retrieve texture handle.
		Handle<Texture> handle = Handle<Texture>::UnPack(asset->Indentifier);

		// Load the texture settings.
		TextureSettings textureSettings = TextureUtilities::Get().DeserializeAssetMetadataFile(asset);

		// Load texture data to buffer.
		size_t textureByteSize = TextureUtilities::Get().GetTextureByteSize(textureSettings);
		void* textureBytes = std::malloc(textureByteSize);

		ByteSpan textureData(textureBytes, textureByteSize);
		ResourceManager::Instance->CopyTextureToHostBuffer(handle, textureData);

		// Serialize to stream.
		uint64_t startPosition = stream.GetStreamPosition();

		stream.WriteRaw(textureSettings.Width);
		stream.WriteRaw(textureSettings.Height);
		stream.WriteRaw((uint64_t)textureSettings.PixelFormat);
		stream.WriteBytes(textureData);

		uint64_t writtenSize = stream.GetStreamPosition() - startPosition;

		// Store offsets in binary blob.
		Assets[asset->UUID].PackedOffset = startPosition;
		Assets[asset->UUID].PackedSize = writtenSize;

		std::free(textureBytes);

		return true;
	}

	bool AssetPack::SerializeShader(Asset* asset, FileStreamWriter& stream)
	{
		const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& workingDirectory = Project::GetAssetDirectory().parent_path().parent_path();
		const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : workingDirectory / asset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

		std::ifstream metadataStream(shaderPath.string() + ".hblshader");

		if (!metadataStream.is_open())
		{
			HBL2_CORE_ERROR("Shader metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblshader");
			return false;
		}

		std::stringstream ss;
		ss << metadataStream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Shader"].IsDefined())
		{
			HBL2_CORE_ERROR("Shader not found: {0}", asset->DebugName);
			metadataStream.close();
			return false;
		}

		const std::string& shaderName = asset->FilePath.filename().stem().string();

		ShaderAssetPackFormat shaderAssetPackFormat;
		uint32_t type = 0;

		// Retrieve shader variants.
		const auto& shaderProperties = data["Shader"];
		{
			if (shaderProperties)
			{
				type = shaderProperties["Type"].as<uint32_t>();

				// Get shader variants property.
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

						shaderAssetPackFormat.Data.ShaderVariants.push_back(variant);
					}
				}
			}
		}
		
		// Compile shader and retrieve compilation and reflection data.
		ShaderReflectionData outReflectionData;
		bool attachLightingModules = (type == 1 || type == 2);
		const auto& compilationData = ShaderUtilities::Get().Compile(shaderPath.string(), &outReflectionData, attachLightingModules);

		if (!compilationData.IsValid())
		{
			HBL2_CORE_ERROR("Shader asset: {0}, at path: {1}, could not be compiled.", asset->DebugName, shaderPath.string());
			metadataStream.close();
			return false;
		}

		// Retrieve shader vertex buffer bindings.
		{
			for (auto& vertexBufferBinding : outReflectionData.vertexBufferBindings)
			{
				shaderAssetPackFormat.Data.VertexBufferBindings.push_back(vertexBufferBinding);
			}
		}

		// Retrieve shader bind group layouts.
		{
			shaderAssetPackFormat.Data.BindGroupLayoutDescriptors.resize(4);
			for (const auto& descriptorSet : outReflectionData.descriptorSets)
			{
				if (descriptorSet.set > 3)
				{
					continue;
				}

				for (const auto& b : descriptorSet.bindings)
				{
					if (b.type == ShaderResourceType::SampledTexture || b.type == ShaderResourceType::CombinedTextureSampler ||
						b.type == ShaderResourceType::StorageTexture || b.type == ShaderResourceType::Sampler)
					{
						TextureBindingType textureType;

						switch (b.type)
						{
						case ShaderResourceType::SampledTexture:
							textureType = TextureBindingType::SAMPLED_IMAGE;
							break;
						case ShaderResourceType::StorageTexture:
							textureType = TextureBindingType::STORAGE_IMAGE;
							break;
						case ShaderResourceType::Sampler:
							textureType = TextureBindingType::SAMPLER;
							break;
						case ShaderResourceType::CombinedTextureSampler:
							textureType = TextureBindingType::COMBINED_IMAGE_SAMPLER;
							break;
						}

						shaderAssetPackFormat.Data.BindGroupLayoutDescriptors[descriptorSet.set].textureBindings.push_back({
							.slot = b.binding,
							.visibility = b.stageMask,
							.type = textureType,
						});
					}
					else if (b.type == ShaderResourceType::UniformBuffer || b.type == ShaderResourceType::StorageBuffer || b.type == ShaderResourceType::StorageBufferReadOnly)
					{
						BufferBindingType bufferType;

						switch (b.type)
						{
						case ShaderResourceType::UniformBuffer:
							bufferType = BufferBindingType::UNIFORM;
							break;
						case ShaderResourceType::StorageBuffer:
							bufferType = BufferBindingType::STORAGE;
							break;
						case ShaderResourceType::StorageBufferReadOnly:
							bufferType = BufferBindingType::READ_ONLY_STORAGE;
							break;
						}

						shaderAssetPackFormat.Data.BindGroupLayoutDescriptors[descriptorSet.set].bufferBindings.push_back({
							.slot = b.binding,
							.visibility = b.stageMask,
							.type = bufferType,
						});
					}
				}
			}
		}

		// Retrieve shader bind group.
		ScratchArena scratch(Allocator::FrameArenaMT);
		{
			if (shaderProperties && shaderProperties["BindGroup"].IsDefined())
			{
				for (const auto& descriptorSet : outReflectionData.descriptorSets)
				{
					if (descriptorSet.set != 1)
					{
						continue;
					}

					uint32_t bindingIndex = 0;

					for (const auto& b : descriptorSet.bindings)
					{
						if (b.type == ShaderResourceType::UniformBuffer)
						{
							FixedArray<uint8_t> uniformBufferBytes = FixedArray<uint8_t>(scratch.GetArena(), b.size);
							uniformBufferBytes.resize(b.size);

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

								void* bufferData = scratch.Alloc(uniformBufferBytes.size(), alignof(uint8_t));
								std::memcpy(bufferData, uniformBufferBytes.data(), uniformBufferBytes.size());

								shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.buffers.push_back({
									.buffer = {
										.debugName = "shader-uniform-buffer",
										.usage = BufferUsage::UNIFORM,
										.memoryUsage = MemoryUsage::GPU_CPU,
										.byteSize = (uint32_t)b.size,
										.initialData = bufferData,
									}
								});
							}
						}
						else if (b.type == ShaderResourceType::SampledTexture || b.type == ShaderResourceType::CombinedTextureSampler || b.type == ShaderResourceType::StorageTexture)
						{
							const auto& textureProp = shaderProperties["BindGroup"][bindingIndex];

							if (textureProp.IsDefined())
							{
								UUID textureMapUUID = textureProp[b.name].as<UUID>();
								shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.textures.push_back({
									.texture = textureMapUUID,
								});
							}
						}

						bindingIndex++;
					}
				}
			}
		}

		// Retrieve SpecializationConstantsPerVariant.
		{
			const auto& specializationConstantsPerVariant = outReflectionData.GetSpecializationConstantsPerVariant(shaderAssetPackFormat.Data.ShaderVariants);
			for (auto& variantSpecConstants : specializationConstantsPerVariant)
			{
				shaderAssetPackFormat.Data.SpecializationConstantsPerVariant.emplace_back();
				auto& variantSpecializationConstants = shaderAssetPackFormat.Data.SpecializationConstantsPerVariant.back();

				for (auto& specConstant : variantSpecConstants)
				{
					variantSpecializationConstants.push_back(specConstant);
				}
			}
		}

		// Retrieve shader code.
		{
			shaderAssetPackFormat.Data.VertexShaderCode = ByteSpan(compilationData.vertexShaderCode.ptr, compilationData.vertexShaderCode.size);
			shaderAssetPackFormat.Data.FragmentShaderCode = ByteSpan(compilationData.fragmentShaderCode.ptr, compilationData.fragmentShaderCode.size);
		}

		// Serialize to stream.
		uint64_t startPosition = stream.GetStreamPosition();

		// Serialize ShaderAssetPackFormat header.
		stream.WriteRaw<ShaderAssetPackFormat::FileHeader>(shaderAssetPackFormat.Header);

		// Serialize ShaderVariants.
		stream.WriteSpan<ShaderDescriptor::RenderPipeline::PackedVariant>(shaderAssetPackFormat.Data.ShaderVariants);

		// Serialize VertexBufferBindings.
		stream.WriteRaw<uint32_t>((uint32_t)shaderAssetPackFormat.Data.VertexBufferBindings.size());
		for (auto& elem : shaderAssetPackFormat.Data.VertexBufferBindings)
		{
			stream.WriteRaw<uint32_t>(elem.bufferBinding);
			stream.WriteRaw<uint32_t>(elem.byteStride);
			stream.WriteSpan<ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute>(elem.attributes);
		}

		// Serialize BindGroupLayoutDescriptors.
		stream.WriteRaw<uint32_t>((uint32_t)shaderAssetPackFormat.Data.BindGroupLayoutDescriptors.size());
		for (auto& elem : shaderAssetPackFormat.Data.BindGroupLayoutDescriptors)
		{
			stream.WriteSpan<ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::TextureBinding>(elem.textureBindings);
			stream.WriteSpan<ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::BufferBinding>(elem.bufferBindings);
		}

		// Serialize ShaderBindGroupDescriptor.
		stream.WriteSpan<ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::TextureBinding>(shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.layout.textureBindings);
		stream.WriteSpan<ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::BufferBinding>(shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.layout.bufferBindings);

		stream.WriteSpan<ShaderAssetPackFormat::Metadata::BindGroupDescriptor::TextureEntry>(shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.textures);

		for (auto& buffer : shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.buffers)
		{
			stream.WriteRaw<uint32_t>(buffer.byteOffset);
			stream.WriteRaw<uint32_t>(buffer.range);

			stream.WriteRaw<uint32_t>(buffer.buffer.usage.GetRaw());
			stream.WriteRaw<uint32_t>((uint32_t)buffer.buffer.memoryUsage);
			stream.WriteRaw<uint32_t>(buffer.buffer.byteSize);

			ByteSpan bufferBytes(buffer.buffer.initialData, buffer.buffer.byteSize);
			stream.WriteBytes(bufferBytes);
		}

		// Serialize SpecializationConstantsPerVariant.
		stream.WriteRaw<uint32_t>(shaderAssetPackFormat.Data.SpecializationConstantsPerVariant.size());
		for (auto& variantSpecializationConstants : shaderAssetPackFormat.Data.SpecializationConstantsPerVariant)
		{
			stream.WriteSpan<ShaderConstant>(variantSpecializationConstants);
		}

		// Serialize VertexShaderCode and FragmentShaderCode.
		stream.WriteBytes(shaderAssetPackFormat.Data.VertexShaderCode);
		stream.WriteBytes(shaderAssetPackFormat.Data.FragmentShaderCode);

		uint64_t writtenSize = stream.GetStreamPosition() - startPosition;

		// Store offsets in binary blob.
		Assets[asset->UUID].PackedOffset = startPosition;
		Assets[asset->UUID].PackedSize = writtenSize;

		metadataStream.close();
		return true;
	}

	bool AssetPack::SerializeMaterial(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeMesh(Asset* asset, FileStreamWriter& stream)
	{
		// Retrieve mesh handle.
		Handle<Mesh> handle = Handle<Mesh>::UnPack(asset->Indentifier);
		Mesh* mesh = ResourceManager::Instance->GetMesh(handle);

		return false;
	}

	bool AssetPack::SerializeScene(Asset* asset, FileStreamWriter& stream)
	{
		// Retrieve scene handle.
		Handle<Scene> handle = Handle<Scene>::UnPack(asset->Indentifier);
		Scene* scene = ResourceManager::Instance->GetScene(handle);

		return false;
	}

	bool AssetPack::SerializePrefab(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeScript(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeSound(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}
}
