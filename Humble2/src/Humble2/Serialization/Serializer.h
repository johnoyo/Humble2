#pragma once

#include "AssetPack.h"

#include "FileStreamWriter.h"
#include "FileStreamReader.h"

#include "Scene/Scene.h"
#include "Resources/TypeDescriptors.h"

#include "Utilities/JobSystem.h"

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
	struct Serializer<AssetPackFormat::BindGroupLayoutDescriptor::TextureBinding>
	{
		static bool Serialize(FileStreamWriter* stream, const AssetPackFormat::BindGroupLayoutDescriptor::TextureBinding& obj)
		{
			stream->WriteRaw(obj.slot);
			stream->WriteRaw((uint32_t)obj.type);
			stream->WriteRaw(obj.visibility.GetRaw());

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, AssetPackFormat::BindGroupLayoutDescriptor::TextureBinding& obj)
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
	struct Serializer<AssetPackFormat::BindGroupLayoutDescriptor::BufferBinding>
	{
		static bool Serialize(FileStreamWriter* stream, const AssetPackFormat::BindGroupLayoutDescriptor::BufferBinding& obj)
		{
			stream->WriteRaw(obj.slot);
			stream->WriteRaw((uint32_t)obj.type);
			stream->WriteRaw(obj.visibility.GetRaw());

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, AssetPackFormat::BindGroupLayoutDescriptor::BufferBinding& obj)
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

	template<>
	struct Serializer<AssetPackFormat::Mesh::Metadata::MeshDescriptor>
	{
		static bool Serialize(FileStreamWriter* stream, const AssetPackFormat::Mesh::Metadata::MeshDescriptor& obj)
		{
			// Sub meshes.
			auto& subMeshes = *(FixedArray<AssetPackFormat::Mesh::Metadata::SubMeshDescriptor>*)&obj.subMeshes;
			stream->WriteSpan<AssetPackFormat::Mesh::Metadata::SubMeshDescriptor>(subMeshes);

			// Index buffer.
			stream->WriteRaw(obj.indexBuffer.usage.GetRaw());
			stream->WriteRaw((uint32_t)obj.indexBuffer.memoryUsage);
			ByteSpan indexBufferBytes = { obj.indexBuffer.initialData, obj.indexBuffer.byteSize };
			stream->WriteBytes(indexBufferBytes);

			// Vertex buffer.
			stream->WriteRaw(obj.vertexBuffer.usage.GetRaw());
			stream->WriteRaw((uint32_t)obj.vertexBuffer.memoryUsage);
			ByteSpan vertexBufferBytes = { obj.vertexBuffer.initialData, obj.vertexBuffer.byteSize };
			stream->WriteBytes(vertexBufferBytes);

			// Skin buffer.
			if (obj.skinBuffer.initialData != nullptr && obj.skinBuffer.byteSize != 0)
			{
				stream->WriteRaw(1);

				stream->WriteRaw(obj.skinBuffer.usage.GetRaw());
				stream->WriteRaw((uint32_t)obj.skinBuffer.memoryUsage);
				ByteSpan skinBufferBytes = { obj.skinBuffer.initialData, obj.skinBuffer.byteSize };
				stream->WriteBytes(skinBufferBytes);
			}
			else
			{
				stream->WriteRaw(0);
			}

			// Imported local transform.
			stream->WriteRaw<glm::vec3>(obj.importedLocalTransform.translation);
			stream->WriteRaw<glm::vec3>(obj.importedLocalTransform.rotation);
			stream->WriteRaw<glm::vec3>(obj.importedLocalTransform.scale);

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, AssetPackFormat::Mesh::Metadata::MeshDescriptor& obj)
		{
			// Sub meshes.
			uint32_t subMeshCount = 0;
			stream->ReadRaw(subMeshCount);
			obj.subMeshes = FixedArray<AssetPackFormat::Mesh::Metadata::SubMeshDescriptor>(JobSystem::Get().GetWorkerArena(), subMeshCount);
			obj.subMeshes.resize(subMeshCount);
			stream->ReadSpan<AssetPackFormat::Mesh::Metadata::SubMeshDescriptor>(obj.subMeshes, subMeshCount);

			// Index buffer.
			uint32_t indexBufferUsageFlags = 0;
			stream->ReadRaw(indexBufferUsageFlags);
			obj.indexBuffer.usage.SetRaw(indexBufferUsageFlags);

			uint32_t indexBufferMemoryUsage = 0;
			stream->ReadRaw(indexBufferMemoryUsage);
			obj.indexBuffer.memoryUsage = (MemoryUsage)indexBufferMemoryUsage;

			ByteSpan indexBufferBytes;
			stream->ReadBytes(indexBufferBytes);
			obj.indexBuffer.initialData = indexBufferBytes.data();
			obj.indexBuffer.byteSize = indexBufferBytes.size();

			// Vertex buffer.
			uint32_t vertexBufferUsageFlags = 0;
			stream->ReadRaw(vertexBufferUsageFlags);
			obj.vertexBuffer.usage.SetRaw(vertexBufferUsageFlags);

			uint32_t vertexBufferMemoryUsage = 0;
			stream->ReadRaw(vertexBufferMemoryUsage);
			obj.vertexBuffer.memoryUsage = (MemoryUsage)vertexBufferMemoryUsage;

			ByteSpan vertexBufferBytes;
			stream->ReadBytes(vertexBufferBytes);
			obj.vertexBuffer.initialData = vertexBufferBytes.data();
			obj.vertexBuffer.byteSize = vertexBufferBytes.size();

			// Skin buffer.
			uint32_t isSkinned;
			stream->ReadRaw(isSkinned);
			if (isSkinned)
			{
				uint32_t skinBufferUsageFlags = 0;
				stream->ReadRaw(skinBufferUsageFlags);
				obj.skinBuffer.usage.SetRaw(skinBufferUsageFlags);

				uint32_t skinBufferMemoryUsage = 0;
				stream->ReadRaw(skinBufferMemoryUsage);
				obj.skinBuffer.memoryUsage = (MemoryUsage)skinBufferMemoryUsage;

				ByteSpan skinBufferBytes;
				stream->ReadBytes(skinBufferBytes);
				obj.skinBuffer.initialData = skinBufferBytes.data();
				obj.skinBuffer.byteSize = skinBufferBytes.size();
			}

			// Imported local transform.
			stream->ReadRaw<glm::vec3>(obj.importedLocalTransform.translation);
			stream->ReadRaw<glm::vec3>(obj.importedLocalTransform.rotation);
			stream->ReadRaw<glm::vec3>(obj.importedLocalTransform.scale);

			return true;
		}
	};

	template<>
	struct Serializer<AnimationClipDescriptor>
	{
		static bool Serialize(FileStreamWriter* stream, const AnimationClipDescriptor& obj)
		{
			stream->WriteRaw(obj.samplers.size());

			for (auto& sampler : obj.samplers)
			{
				stream->WriteRaw((uint32_t)sampler.interpolation);

				auto& samplerInputs = *(std::vector<float>*)&sampler.inputs;
				stream->WriteSpan<float>(samplerInputs);

				auto& samplerInTangents = *(std::vector<glm::vec4>*)&sampler.in_tangents;
				stream->WriteSpan<glm::vec4>(samplerInTangents);

				auto& samplerValues = *(std::vector<glm::vec4>*)&sampler.values;
				stream->WriteSpan<glm::vec4>(samplerValues);

				auto& samplerOutTangents = *(std::vector<glm::vec4>*) & sampler.out_tangents;
				stream->WriteSpan<glm::vec4>(samplerOutTangents);
			}

			auto& channels = *(std::vector<AnimationChannel>*)&obj.channels;
			stream->WriteSpan<AnimationChannel>(channels);

			stream->WriteRaw<float>(obj.duration);

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, AnimationClipDescriptor& obj)
		{
			uint32_t samplerCount = 0;
			stream->ReadRaw(samplerCount);
			obj.samplers.resize(samplerCount);

			for (uint32_t i = 0; i < samplerCount; i++)
			{
				uint32_t samplerInterpolation = 0;
				stream->ReadRaw(samplerInterpolation);
				obj.samplers[i].interpolation = (AnimationSampler::InterpolationType)samplerInterpolation;

				uint32_t inputCount = 0;
				stream->ReadRaw(inputCount);
				obj.samplers[i].inputs.resize(inputCount);
				stream->ReadSpan<float>(obj.samplers[i].inputs, inputCount);

				uint32_t inTangentCount = 0;
				stream->ReadRaw(inTangentCount);
				obj.samplers[i].in_tangents.resize(inTangentCount);
				stream->ReadSpan<glm::vec4>(obj.samplers[i].in_tangents, inTangentCount);

				uint32_t valueCount = 0;
				stream->ReadRaw(valueCount);
				obj.samplers[i].values.resize(valueCount);
				stream->ReadSpan<glm::vec4>(obj.samplers[i].values, valueCount);

				uint32_t outTangentCount = 0;
				stream->ReadRaw(outTangentCount);
				obj.samplers[i].out_tangents.resize(outTangentCount);
				stream->ReadSpan<glm::vec4>(obj.samplers[i].out_tangents, outTangentCount);
			}

			uint32_t channelCount = 0;
			stream->ReadRaw(channelCount);
			obj.channels.resize(channelCount);
			stream->ReadSpan<AnimationChannel>(obj.channels, channelCount);

			stream->ReadRaw<float>(obj.duration);

			return true;
		}
	};

	template<>
	struct Serializer<SceneDescriptor>
	{
		static bool Serialize(FileStreamWriter* stream, const SceneDescriptor& obj)
		{
			stream->WriteRaw(obj.maxEntities);
			stream->WriteRaw(obj.maxComponents);
			stream->WriteRaw(obj.maxSystems);
			stream->WriteRaw(obj.maxStructuralCommandsPerFramePerThread);
			stream->WriteRaw(obj.maxJobsPerSystem);
			stream->WriteRaw(obj.useStructuralCommandBuffer);

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, SceneDescriptor& obj)
		{
			stream->ReadRaw(obj.maxEntities);
			stream->ReadRaw(obj.maxComponents);
			stream->ReadRaw(obj.maxSystems);
			stream->ReadRaw(obj.maxStructuralCommandsPerFramePerThread);
			stream->ReadRaw(obj.maxJobsPerSystem);
			stream->ReadRaw(obj.useStructuralCommandBuffer);

			return true;
		}
	};

	template<>
	struct Serializer<Component::AnimationCurve::KeyFrame>
	{
		static bool Serialize(FileStreamWriter* stream, const Component::AnimationCurve::KeyFrame& obj)
		{
			stream->WriteRaw(obj.Time);
			stream->WriteRaw(obj.Value);
			stream->WriteRaw(obj.InTan);
			stream->WriteRaw(obj.OutTan);

			return true;
		}

		static bool Deserialize(FileStreamReader* stream, Component::AnimationCurve::KeyFrame& obj)
		{
			stream->ReadRaw(obj.Time);
			stream->ReadRaw(obj.Value);
			stream->ReadRaw(obj.InTan);
			stream->ReadRaw(obj.OutTan);

			return true;
		}
	};
}