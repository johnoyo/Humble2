#include "AssetPack.h"

#include "Serializer.h"
#include "Asset/EditorAssetManager.h"
#include "Utilities/Collections/ByteSpan.h"
#include "Utilities/YamlUtilities.h"

namespace HBL2
{
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

		AssetPackFormat::Shader shaderAssetPackFormat;
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
			shaderAssetPackFormat.Data.BindGroupLayoutDescriptors.resize(3);
			for (const auto& descriptorSet : outReflectionData.descriptorSets)
			{
				if (descriptorSet.set >= 3)
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
		stream.WriteRaw<AssetPackFormat::Shader::FileHeader>(shaderAssetPackFormat.Header);

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
			stream.WriteSpan<AssetPackFormat::BindGroupLayoutDescriptor::TextureBinding>(elem.textureBindings);
			stream.WriteSpan<AssetPackFormat::BindGroupLayoutDescriptor::BufferBinding>(elem.bufferBindings);
		}

		// Serialize ShaderBindGroupDescriptor.
		stream.WriteSpan<AssetPackFormat::Shader::Metadata::BindGroupDescriptor::TextureEntry>(shaderAssetPackFormat.Data.ShaderBindGroupDescriptor.textures);

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
		const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
		const auto& workingDirectory = Project::GetProjectDirectory().parent_path();
		const std::filesystem::path& materialPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : workingDirectory / asset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.
				
		AssetPackFormat::Material materialAssetPackFormat;

		// Retrieve material type from metadata file.
		{
			// Open metadata file.
			std::ifstream metaDataStream(materialPath.string() + ".hblmat");

			if (!metaDataStream.is_open())
			{
				HBL2_CORE_ERROR("Material metadata file not found: {0}", Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblmat");
				return false;
			}

			std::stringstream ssMetadata;
			ssMetadata << metaDataStream.rdbuf();

			YAML::Node dataMetadata = YAML::Load(ssMetadata.str());
			if (!dataMetadata["Material"].IsDefined())
			{
				HBL2_CORE_ERROR("Material not found in metadata file: {0}", ssMetadata.str());
				metaDataStream.close();
				return false;
			}

			auto materialMetadataProperties = dataMetadata["Material"];
			if (materialMetadataProperties)
			{
				materialAssetPackFormat.Data.Type = materialMetadataProperties["Type"].as<uint32_t>();
			}

			metaDataStream.close();
		}

		// Open regular material file.
		std::ifstream dataStream(materialPath);

		if (!dataStream.is_open())
		{
			HBL2_CORE_ERROR("Material file not found: {0}", materialPath);
			return false;
		}

		std::stringstream ss;
		ss << dataStream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Material"].IsDefined())
		{
			HBL2_CORE_ERROR("Material not found: {0}", ss.str());
			dataStream.close();
			return false;
		}

		auto materialProperties = data["Material"];

		// Retrieve shader UUID and variant.
		ShaderDescriptor::RenderPipeline::PackedVariant variantDesc = {};
		{
			if (materialProperties)
			{
				materialAssetPackFormat.Data.ShaderUUID = materialProperties["Shader"].as<UUID>();

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
			}
		}

		auto shaderAssetHandle = AssetManager::Instance->GetHandleFromUUID(materialAssetPackFormat.Data.ShaderUUID);
		Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(shaderAssetHandle);

		if (shaderAsset == nullptr)
		{
			HBL2_CORE_ERROR("Shader with UUID: {0}, of material: {1}, not found!", materialAssetPackFormat.Data.ShaderUUID, materialPath);
			dataStream.close();
			return false;
		}

		auto shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderAssetHandle);

		const auto& filesystemPath = Project::GetAssetFileSystemPath(shaderAsset->FilePath);
		const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : workingDirectory / shaderAsset->FilePath;

		// NOTE: This^ is done to handle built in assets that are outside of the project assets folder,
		//		 meaning that the Project::GetAssetFileSystemPath will return an invalid path for them.

		ShaderReflectionData shaderReflectionData;
		ShaderUtilities::Get().Reflect(shaderPath.string(), &shaderReflectionData, true);

		// Retrieve material bind group.
		ScratchArena scratch(Allocator::FrameArenaMT);
		{
			for (const auto& descriptorSet : shaderReflectionData.descriptorSets)
			{
				if (descriptorSet.set != 2)
				{
					continue;
				}

				// Retrieve bind group layout description.
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

						materialAssetPackFormat.Data.MaterialBindGroupDescriptor.layout.textureBindings.push_back({
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

						materialAssetPackFormat.Data.MaterialBindGroupDescriptor.layout.bufferBindings.push_back({
							.slot = b.binding,
							.visibility = b.stageMask,
							.type = bufferType,
						});
					}
				}

				// Retrieve bind group description.
				for (const auto& b : descriptorSet.bindings)
				{
					if (b.type == ShaderResourceType::UniformBuffer)
					{
						FixedArray<uint8_t> uniformBufferBytes = FixedArray<uint8_t>(scratch.GetArena(), b.size);
						uniformBufferBytes.resize(b.size);

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

							void* bufferData = scratch.Alloc(uniformBufferBytes.size(), alignof(uint8_t));
							std::memcpy(bufferData, uniformBufferBytes.data(), uniformBufferBytes.size());

							materialAssetPackFormat.Data.MaterialBindGroupDescriptor.buffers.push_back({
								.buffer = {
									.debugName = "material-uniform-buffer",
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
						const auto& textureProp = materialProperties[b.name];

						if (textureProp.IsDefined())
						{
							UUID textureMapUUID = textureProp.as<UUID>();

							materialAssetPackFormat.Data.MaterialBindGroupDescriptor.textures.push_back({
								.texture = textureMapUUID,
							});
						}
					}
				}
			}
		}

		// Serialize to stream.
		uint64_t startPosition = stream.GetStreamPosition();

		// Serialize MaterialAssetPackFormat header.
		stream.WriteRaw<AssetPackFormat::Material::FileHeader>(materialAssetPackFormat.Header);

		// Serialize material type and shader uuid.
		stream.WriteRaw<uint32_t>(materialAssetPackFormat.Data.Type);
		stream.WriteRaw<UUID>(materialAssetPackFormat.Data.ShaderUUID);

		// Serialize MaterialBindGroupDescriptor.
		stream.WriteSpan<AssetPackFormat::BindGroupLayoutDescriptor::TextureBinding>(materialAssetPackFormat.Data.MaterialBindGroupDescriptor.layout.textureBindings);
		stream.WriteSpan<AssetPackFormat::BindGroupLayoutDescriptor::BufferBinding>(materialAssetPackFormat.Data.MaterialBindGroupDescriptor.layout.bufferBindings);

		stream.WriteSpan<AssetPackFormat::Material::Metadata::BindGroupDescriptor::TextureEntry>(materialAssetPackFormat.Data.MaterialBindGroupDescriptor.textures);

		for (auto& buffer : materialAssetPackFormat.Data.MaterialBindGroupDescriptor.buffers)
		{
			stream.WriteRaw<uint32_t>(buffer.byteOffset);
			stream.WriteRaw<uint32_t>(buffer.range);

			stream.WriteRaw<uint32_t>(buffer.buffer.usage.GetRaw());
			stream.WriteRaw<uint32_t>((uint32_t)buffer.buffer.memoryUsage);
			stream.WriteRaw<uint32_t>(buffer.buffer.byteSize);

			ByteSpan bufferBytes(buffer.buffer.initialData, buffer.buffer.byteSize);
			stream.WriteBytes(bufferBytes);
		}

		// Serialize material variant hash.
		stream.WriteRaw<packed_size>(variantDesc.Key());

		uint64_t writtenSize = stream.GetStreamPosition() - startPosition;

		// Store offsets in binary blob.
		Assets[asset->UUID].PackedOffset = startPosition;
		Assets[asset->UUID].PackedSize = writtenSize;

		dataStream.close();
		return true;
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
