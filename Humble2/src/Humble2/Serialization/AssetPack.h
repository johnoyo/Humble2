#pragma once

#include "FileStreamWriter.h"

#include "Asset/Asset.h"
#include "Resources/Handle.h"
#include "Resources/TypeDescriptors.h"
#include "Utilities/Collections/StaticDArray.h"
#include "Utilities/Collections/FixedHashMap.h"

#include <cstdint>

namespace HBL2
{
	struct AssetPack
	{
		static constexpr uint32_t CurrentVersion = 0;

		struct AssetInfo
		{
			uint64_t PackedOffset = 0;
			uint64_t PackedSize = 0;
			uint16_t Type = 0;
			uint16_t Flags = 0;
		};

		struct FileHeader
		{
			const char Header[5] = { 'H', 'B', 'L', 'A', 'P' };
			uint32_t Version = CurrentVersion;
			uint64_t BuildVersion = 0;
		};

		FileHeader Header;
		FixedHashMap<UUID, AssetInfo> Assets;

		bool Serialize(UUID assetUUID, FileStreamWriter& stream);

	private:
		bool SerializeTexture(Asset* asset, FileStreamWriter& stream);
		bool SerializeShader(Asset* asset, FileStreamWriter& stream);
		bool SerializeMaterial(Asset* asset, FileStreamWriter& stream);
		bool SerializeMesh(Asset* asset, FileStreamWriter& stream);
		bool SerializeScene(Asset* asset, FileStreamWriter& stream);
		bool SerializePrefab(Asset* asset, FileStreamWriter& stream);
		bool SerializeScript(Asset* asset, FileStreamWriter& stream);
		bool SerializeSound(Asset* asset, FileStreamWriter& stream);
	};

	struct ShaderAssetPackFormat
	{
		static constexpr uint32_t CurrentVersion = 0;

		struct Metadata
		{
			struct BindGroupLayoutDescriptor
			{
				struct TextureBinding
				{
					uint32_t slot = 0;
					BitFlags<ShaderStage> visibility = { ShaderStage::VERTEX };
					TextureBindingType type = TextureBindingType::COMBINED_IMAGE_SAMPLER;
				};
				StaticDArray<TextureBinding, 8> textureBindings;

				struct BufferBinding
				{
					uint32_t slot = 0;
					BitFlags<ShaderStage> visibility = { ShaderStage::VERTEX };
					BufferBindingType type = BufferBindingType::UNIFORM;
				};

				StaticDArray<BufferBinding, 8> bufferBindings;
			};

			struct BindGroupDescriptor
			{
				BindGroupLayoutDescriptor layout;

				struct TextureEntry
				{
					UUID texture;
				};

				StaticDArray<TextureEntry, 8> textures;
				struct BufferEntry
				{
					BufferDescriptor buffer;
					uint32_t byteOffset = 0;
					uint32_t range = 0;
				};
				StaticDArray<BufferEntry, 8> buffers;
			};

			StaticDArray<ShaderDescriptor::RenderPipeline::PackedVariant, 16> ShaderVariants;
			StaticDArray<ShaderDescriptor::RenderPipeline::VertexBufferBinding, 2> VertexBufferBindings;
			StaticDArray<BindGroupLayoutDescriptor, 4> BindGroupLayoutDescriptors;
			BindGroupDescriptor ShaderBindGroupDescriptor;
			StaticDArray<StaticDArray<ShaderConstant, 8>, 16> SpecializationConstantsPerVariant;
			ByteSpan VertexShaderCode;
			ByteSpan FragmentShaderCode;
			ByteSpan ComputeShaderCode;
		};

		struct FileHeader
		{
			const char HEADER[5] = { 'H','B','L','S','D' };
			uint32_t Version = CurrentVersion;
		};

		FileHeader Header;
		Metadata Data;
	};

	struct MeshAssetPackFormat
	{
		static constexpr uint32_t CurrentVersion = 0;

		enum class MeshFlags : uint32_t
		{
			HasMaterials = 1,
			HasAnimation = 2,
			HasSkeleton = 4,
		};

		struct Metadata
		{
			uint32_t Flags;
			// AABB BoundingBox;

			uint64_t NodeArrayOffset;
			uint64_t NodeArraySize;

			uint64_t SubmeshArrayOffset;
			uint64_t SubmeshArraySize;

			uint64_t MaterialArrayOffset;
			uint64_t MaterialArraySize;

			uint64_t VertexBufferOffset;
			uint64_t VertexBufferSize;

			uint64_t IndexBufferOffset;
			uint64_t IndexBufferSize;

			uint64_t AnimationDataOffset;
			uint64_t AnimationDataSize;
		};

		struct FileHeader
		{
			const char HEADER[5] = { 'H','B','L','M','S' };
			uint32_t Version = CurrentVersion;
		};

		FileHeader Header;
		Metadata Data;
	};
}