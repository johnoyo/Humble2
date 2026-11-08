#pragma once

#include "FileStreamWriter.h"
#include "FileStreamReader.h"

#include "Resources/TypeDescriptors.h"

namespace HBL2
{
	template<typename T> inline constexpr bool is_always_false_v = false;

	template<typename T>
	struct Serializer
	{
		static bool Serialize(FileStreamWriter*, const T&) { static_assert(is_always_false_v<T>, "No Serialize specialization visible for this T"); }
		static bool Deserialize(FileStreamReader*, T&) { static_assert(is_always_false_v<T>, "No Deserialize specialization visible for this T"); }
	};

	template<>
	struct Serializer<ShaderConstant>
	{
		static bool Serialize(FileStreamWriter* stream, const ShaderConstant& obj)
		{
			stream->WriteRaw((uint32_t)obj.type);
			stream->WriteRaw(obj.stage.GetRaw());

			switch (obj.type)
			{
			case ShaderConstantType::Int:
				stream->WriteRaw<int32_t>(obj.value.i);
				break;
			case ShaderConstantType::UInt:
				stream->WriteRaw<uint32_t>(obj.value.u);
				break;
			case ShaderConstantType::Bool:
				stream->WriteRaw<bool>(obj.value.b);
				break;
			case ShaderConstantType::Float:
				stream->WriteRaw<float>(obj.value.f);
				break;
			}

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, ShaderConstant& obj)
		{
			uint32_t type = 0;
			stream->ReadRaw(type);
			obj.type = (ShaderConstantType)type;

			uint32_t stageFlags = 0;
			stream->ReadRaw(stageFlags);
			obj.stage.SetRaw(stageFlags);

			switch (obj.type)
			{
			case ShaderConstantType::Int:
				stream->ReadRaw<int32_t>(obj.value.i);
				break;
			case ShaderConstantType::UInt:
				stream->ReadRaw<uint32_t>(obj.value.u);
				break;
			case ShaderConstantType::Bool:
				stream->ReadRaw<bool>(obj.value.b);
				break;
			case ShaderConstantType::Float:
				stream->ReadRaw<float>(obj.value.f);
				break;
			}

			return true;
		}
	};

	template<>
	struct Serializer<ShaderDescriptor::RenderPipeline::PackedVariant>
	{
		static bool Serialize(FileStreamWriter* stream, const ShaderDescriptor::RenderPipeline::PackedVariant& obj)
		{
			stream->WriteRaw<uint64_t>(obj.Key());
			return true;
		}

		static bool Deserialize(FileStreamReader* stream, ShaderDescriptor::RenderPipeline::PackedVariant& obj)
		{
			uint64_t key = 0;
			stream->ReadRaw<uint64_t>(key);
			obj = ShaderDescriptor::RenderPipeline::PackedVariant::FromKey(key);
			return true;
		}
	};

	template<>
	struct Serializer<ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute>
	{
		static bool Serialize(FileStreamWriter* stream, const ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute& obj)
		{
			stream->WriteRaw(obj.byteOffset);
			stream->WriteRaw((uint32_t)obj.format);

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute& obj)
		{
			stream->ReadRaw(obj.byteOffset);

			uint32_t format = 0;
			stream->ReadRaw(format);
			obj.format = (VertexFormat)format;

			return true;
		}
	};

	template<>
	struct Serializer<ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::TextureBinding>
	{
		static bool Serialize(FileStreamWriter* stream, const ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::TextureBinding& obj)
		{
			stream->WriteRaw(obj.slot);
			stream->WriteRaw((uint32_t)obj.type);
			stream->WriteRaw(obj.visibility.GetRaw());

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::TextureBinding& obj)
		{
			stream->ReadRaw(obj.slot);

			uint32_t type = 0;
			stream->ReadRaw(type);
			obj.type = (TextureBindingType)type;

			uint32_t visibilityFlags = 0;
			stream->ReadRaw(visibilityFlags);
			obj.visibility.SetRaw(visibilityFlags);

			return true;
		}
	};

	template<>
	struct Serializer<ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::BufferBinding>
	{
		static bool Serialize(FileStreamWriter* stream, const ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::BufferBinding& obj)
		{
			stream->WriteRaw(obj.slot);
			stream->WriteRaw((uint32_t)obj.type);
			stream->WriteRaw(obj.visibility.GetRaw());

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, ShaderAssetPackFormat::Metadata::BindGroupLayoutDescriptor::BufferBinding& obj)
		{
			stream->ReadRaw(obj.slot);

			uint32_t type = 0;
			stream->ReadRaw(type);
			obj.type = (BufferBindingType)type;

			uint32_t visibilityFlags = 0;
			stream->ReadRaw(visibilityFlags);
			obj.visibility.SetRaw(visibilityFlags);

			return true;
		}
	};
}